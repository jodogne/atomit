/**
 * Atom-IT - A Lightweight, RESTful microservice for IoT
 * Copyright (C) 2017 Sebastien Jodogne, WSL S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "SQLiteDatabase.h"

#include "SQLiteTimeSeriesTransaction.h"

#include <EmbeddedResources.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/filesystem.hpp>

namespace AtomIT
{
  SQLiteDatabase::Transaction::Transaction(SQLiteDatabase& database) :
    lock_(database.mutex_),
    connection_(database.connection_)
  {
    transaction_.reset(new Orthanc::SQLite::Transaction(connection_));
    transaction_->Begin();
  }

  
  SQLiteDatabase::Transaction::~Transaction()
  {
    if (transaction_.get() != NULL)
    {
      transaction_->Rollback();
      transaction_.reset(NULL);
    }
  }

  
  void SQLiteDatabase::Transaction::Commit()
  {
    if (transaction_.get() == NULL)
    {
      LOG(ERROR) << "Cannot Commit() without a transaction";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      transaction_->Commit();
      transaction_.reset(NULL);
    }
  }


  bool SQLiteDatabase::Transaction::HasTimeSeries(const std::string& name)
  {
    Orthanc::SQLite::Statement s
      (connection_, SQLITE_FROM_HERE, "SELECT internalId FROM TimeSeries WHERE publicId=?");
    s.BindString(0, name);
    return s.Step();
  }

    
  void SQLiteDatabase::FlushWorker(SQLiteDatabase* that)
  {
    unsigned int count = 0;

    while (that->continue_)
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));

      count++;

      // Flush every 10 seconds
      if (count == 100)
      {
        {
          boost::mutex::scoped_lock lock(that->mutex_);
          that->connection_.FlushToDisk();
        }
          
        count = 0;
      }          
    }
  }
    

  void SQLiteDatabase::SetupDatabase()
  {
    if (!connection_.DoesTableExist("GlobalProperties"))
    {
      LOG(WARNING) << "Creating SQLite database";
      std::string query;
      Orthanc::EmbeddedResources::GetFileResource
        (query, Orthanc::EmbeddedResources::PREPARE_SQLITE_DATABASE);
      connection_.Execute(query);
    }

    // Performance tuning of SQLite with PRAGMAs
    // http://www.sqlite.org/pragma.html
    connection_.Execute("PRAGMA SYNCHRONOUS=OFF;");
    connection_.Execute("PRAGMA JOURNAL_MODE=WAL;");
    connection_.Execute("PRAGMA LOCKING_MODE=EXCLUSIVE;");

    continue_ = true;
    flushThread_ = boost::thread(FlushWorker, this);

    LOG(INFO) << "SQLite database is ready";
  }


  SQLiteDatabase::SQLiteDatabase(const std::string& path)
  {
    boost::filesystem::path p(path);
    LOG(WARNING) << "Opening SQLite database from: " << p.string();

    connection_.Open(p.string());
    SetupDatabase();
  }

    
  SQLiteDatabase::SQLiteDatabase()
  {
    LOG(WARNING) << "Opening a transient SQLite database in memory";
    connection_.OpenInMemory();
    SetupDatabase();
  }
    

  SQLiteDatabase::~SQLiteDatabase()
  {
    LOG(INFO) << "Closing SQLite database";

    continue_ = false;

    if (flushThread_.joinable())
    {
      flushThread_.join();
    }
  }

    
  void SQLiteDatabase::DeleteTimeSeries(const std::string& name)
  {
    Transaction transaction(*this);

    Orthanc::SQLite::Statement t(connection_, SQLITE_FROM_HERE,
                                 "DELETE FROM TimeSeries WHERE publicId=?");
    t.BindString(0, name);
    t.Run();

    transaction.Commit();
  }
    

  void SQLiteDatabase::CreateTimeSeries(const std::string& name,
                                        uint64_t maxLength,
                                        uint64_t maxSize)
  {
    std::auto_ptr<Transaction> transaction(new Transaction(*this));

    if (!transaction->HasTimeSeries(name))
    {
      LOG(WARNING) << "Creating a new time series in SQLite database: " << name;
        
      Orthanc::SQLite::Statement t(connection_, SQLITE_FROM_HERE,
                                   "INSERT INTO TimeSeries VALUES(NULL, ?, ?, ?, 0, 0, NULL)");
      t.BindString(0, name);
      t.BindInt64(1, maxLength);
      t.BindInt64(2, maxSize);
      if (!t.Run())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      transaction->Commit();
    }
    else
    {
      {
        Orthanc::SQLite::Statement t(
          connection_, SQLITE_FROM_HERE,
          "UPDATE TimeSeries SET maxLength=?, maxSize=? WHERE publicId=?");
        t.BindInt64(0, maxLength);
        t.BindInt64(1, maxSize);
        t.BindString(2, name);

        if (!t.Run())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        transaction->Commit();
        transaction.reset(NULL);  // Unlock the transaction to be able to update quota at (*)
      }
      
      SQLiteTimeSeriesTransaction::UpdateQuota(*this, name);  // (*)
    }
  }
}
