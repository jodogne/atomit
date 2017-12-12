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


#include "SQLiteTimeSeriesTransaction.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  bool SQLiteTimeSeriesTransaction::SanityCheck()
  {
    // WARNING: This sanity check is very time-consuming, and is enabled in Debug builds

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "SELECT currentLength, currentSize FROM TimeSeries WHERE internalId=?");
      s.BindInt64(0, id_);

      if (!s.Step() ||
          currentLength_ != static_cast<uint64_t>(s.ColumnInt64(0)) ||
          currentSize_ != static_cast<uint64_t>(s.ColumnInt64(1)) ||
          (maxLength_ != 0 && currentLength_ > maxLength_) ||
          (maxSize_ != 0 && currentSize_ > maxSize_))
      {
        return false;
      }
    }

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "SELECT COUNT(*), SUM(size) FROM Content WHERE id=?");
      s.BindInt64(0, id_);

      if (!s.Step() ||
          currentLength_ != static_cast<uint64_t>(s.ColumnInt64(0)) ||
          currentSize_ != static_cast<uint64_t>(s.ColumnInt64(1)))
      {
        return false;
      }
    }

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "SELECT timestamp FROM Content WHERE id=? ORDER BY timestamp DESC LIMIT 1");
        
      s.BindInt64(0, id_);

      if (s.Step())
      {
        if (!hasLastTimestamp_ ||
            s.ColumnInt64(0) > lastTimestamp_)
        {
          return false;
        }
      }
    }
      
    return true;
  }


  void SQLiteTimeSeriesTransaction::UpdateTimeSeriesTable()
  {
    Orthanc::SQLite::Statement s(
      transaction_.GetConnection(), SQLITE_FROM_HERE,
      "UPDATE TimeSeries SET currentLength=?, currentSize=?, lastTimestamp=? WHERE internalId=?");
    s.BindInt64(0, currentLength_);
    s.BindInt64(1, currentSize_);

    if (hasLastTimestamp_)
    {
      s.BindInt64(2, lastTimestamp_);
    }
    else
    {
      s.BindNull(2);
    }       

    s.BindInt64(3, id_);

    if (!s.Run())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }
      
    
  void SQLiteTimeSeriesTransaction::RemoveOldest()
  {
    int64_t timestamp;

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "SELECT timestamp, size FROM Content WHERE id=? ORDER BY timestamp ASC LIMIT 1");
      s.BindInt64(0, id_);

      if (!s.Step())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      uint64_t size = static_cast<uint64_t>(s.ColumnInt64(1));

      if (currentLength_ == 0 ||
          currentSize_ < size)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      currentSize_ -= size;
      currentLength_--;
      timestamp = s.ColumnInt64(0);
    }

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "DELETE FROM Content WHERE id=? AND timestamp=?");
      s.BindInt64(0, id_);
      s.BindInt64(1, timestamp);

      if (!s.Run())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }
    

  SQLiteTimeSeriesTransaction::SQLiteTimeSeriesTransaction(SQLiteDatabase& database,
                                                           const std::string& name) :
    transaction_(database)
  {
    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE, "SELECT internalId, maxLength, "
       "maxSize, currentLength, currentSize, lastTimestamp FROM TimeSeries WHERE publicId=?");
    s.BindString(0, name);
        
    if (!s.Step())
    {
      LOG(ERROR) << "Unknown time series: " << name;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }

    id_ = s.ColumnInt64(0);
    maxLength_ = static_cast<uint64_t>(s.ColumnInt64(1));
    maxSize_ = static_cast<uint64_t>(s.ColumnInt64(2));
    currentLength_ = static_cast<uint64_t>(s.ColumnInt64(3));
    currentSize_ = static_cast<uint64_t>(s.ColumnInt64(4));

    if (s.ColumnIsNull(5))
    {
      hasLastTimestamp_ = false;
    }
    else
    {
      lastTimestamp_ = s.ColumnInt64(5);
      hasLastTimestamp_ = true;
    }
  }


  SQLiteTimeSeriesTransaction::~SQLiteTimeSeriesTransaction()
  {
    assert(SanityCheck());
    transaction_.Commit();
  }

    
  void SQLiteTimeSeriesTransaction::DeleteRange(int64_t start,
                                                int64_t end)
  {
    assert(SanityCheck());

    LOG(INFO) << "Removing range [" << start << ", "
              << end << "[ in time series \"" << id_ << "\"";

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "SELECT COUNT(*), SUM(size) FROM Content WHERE id=? AND timestamp>=? AND timestamp<?");
      s.BindInt64(0, id_);
      s.BindInt64(1, start);
      s.BindInt64(2, end);

      if (!s.Step())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      uint64_t length = s.ColumnInt64(0);
      uint64_t size = s.ColumnInt64(1);

      if (currentLength_ < length ||
          currentSize_ < size)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
        
      currentLength_ -= length;
      currentSize_ -= size;
    }

    {
      Orthanc::SQLite::Statement s
        (transaction_.GetConnection(), SQLITE_FROM_HERE,
         "DELETE FROM Content WHERE id=? AND timestamp>=? AND timestamp<?");
      s.BindInt64(0, id_);
      s.BindInt64(1, start);
      s.BindInt64(2, end);

      if (!s.Run())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    UpdateTimeSeriesTable();
  }
    
    
  bool SQLiteTimeSeriesTransaction::SeekFirst(int64_t& result)
  {
    assert(SanityCheck());

    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT timestamp FROM Content WHERE id=? ORDER BY timestamp ASC LIMIT 1");
    s.BindInt64(0, id_);

    if (s.Step())
    {
      result = s.ColumnInt64(0);
      return true;
    }
    else
    {
      return false;
    }
  }
    

  bool SQLiteTimeSeriesTransaction::SeekLast(int64_t& result)
  {
    assert(SanityCheck());

    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT timestamp FROM Content WHERE id=? ORDER BY timestamp DESC LIMIT 1");
    s.BindInt64(0, id_);

    if (s.Step())
    {
      result = s.ColumnInt64(0);
      return true;
    }
    else
    {
      return false;
    }
  }

    
  bool SQLiteTimeSeriesTransaction::SeekNearest(int64_t& result,
                                                int64_t timestamp)
  {
    assert(SanityCheck());

    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT timestamp FROM Content WHERE id=? AND timestamp>=? ORDER BY timestamp ASC LIMIT 1");
    s.BindInt64(0, id_);
    s.BindInt64(1, timestamp);

    if (s.Step())
    {
      result = s.ColumnInt64(0);
      return true;
    }
    else
    {
      return false;
    }
  }
    

  bool SQLiteTimeSeriesTransaction::SeekNext(int64_t& result,
                                             int64_t timestamp)
  {
    assert(SanityCheck());

    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT timestamp FROM Content WHERE id=? AND timestamp>? ORDER BY timestamp ASC LIMIT 1");
    s.BindInt64(0, id_);
    s.BindInt64(1, timestamp);

    if (s.Step())
    {
      result = s.ColumnInt64(0);
      return true;
    }
    else
    {
      return false;
    }
  }

    
  bool SQLiteTimeSeriesTransaction::SeekPrevious(int64_t& result,
                                                 int64_t timestamp)
  {
    assert(SanityCheck());
 
    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT timestamp FROM Content WHERE id=? AND timestamp<? ORDER BY timestamp DESC LIMIT 1");
    s.BindInt64(0, id_);
    s.BindInt64(1, timestamp);

    if (s.Step())
    {
      result = s.ColumnInt64(0);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool SQLiteTimeSeriesTransaction::Read(std::string& metadata,
                                         std::string& value,
                                         int64_t timestamp)
  {
    assert(SanityCheck());
      
    Orthanc::SQLite::Statement s
      (transaction_.GetConnection(), SQLITE_FROM_HERE,
       "SELECT metadata, value FROM Content WHERE id=? AND timestamp=?");
    s.BindInt64(0, id_);
    s.BindInt64(1, timestamp);

    if (s.Step())
    {
      metadata = s.ColumnString(0);
      value = s.ColumnString(1);
      return true;
    }
    else
    {
      return false;
    }
  }
    

  bool SQLiteTimeSeriesTransaction::Append(int64_t timestamp,
                                           const std::string& metadata,
                                           const std::string& value)
  {
    assert(SanityCheck());

    if (maxSize_ != 0 &&
        value.size() > maxSize_)
    {
      LOG(ERROR) << "Cannot append an observation whose size (" << value.size()
                 << " bytes) is above the max size of the time series (" << maxSize_
                 << " bytes)";
      return false;
    }

    // Make sure the new observation is after the last timestamp
    if (hasLastTimestamp_ &&
        timestamp <= lastTimestamp_)
    {
      return false;
    }
    
    if (maxLength_ != 0)
    {
      while (currentLength_ + 1 > maxLength_)
      {
        RemoveOldest();
      }
    }

    if (maxSize_ != 0)
    {
      while (currentSize_ + value.size() > maxSize_)
      {
        RemoveOldest();
      }
    }

    {
      Orthanc::SQLite::Statement s(transaction_.GetConnection(), SQLITE_FROM_HERE,
                                   "INSERT INTO Content VALUES(?, ?, ?, ?, ?)");
      s.BindInt64(0, id_);
      s.BindInt64(1, timestamp);
      s.BindInt64(2, value.size());
      s.BindString(3, metadata);
      s.BindString(4, value);

      if (!s.Run())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    currentLength_++;
    currentSize_ += value.size();

    if (hasLastTimestamp_)
    {
      assert(lastTimestamp_ < timestamp);
      lastTimestamp_ = timestamp;
    }
    else
    {
      hasLastTimestamp_ = true;
      lastTimestamp_ = timestamp;
    }

    UpdateTimeSeriesTable();

    return true;
  }


  void SQLiteTimeSeriesTransaction::GetStatistics(uint64_t& length,
                                                  uint64_t& size)
  {
    length = currentLength_;
    size = currentSize_;
  }


  void SQLiteTimeSeriesTransaction::ClearContent()
  {
    assert(SanityCheck());

    {
      Orthanc::SQLite::Statement s(transaction_.GetConnection(), SQLITE_FROM_HERE,
                                   "DELETE FROM Content WHERE id=?");
      s.BindInt64(0, id_);

      if (!s.Run())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    currentLength_ = 0;
    currentSize_ = 0;
    
    UpdateTimeSeriesTable();
  }


  bool SQLiteTimeSeriesTransaction::GetLastTimestamp(int64_t& result)
  {
    if (hasLastTimestamp_)
    {
      result = lastTimestamp_;
      return true;
    }
    else
    {
      return false;
    }
  }

  
  void SQLiteTimeSeriesTransaction::UpdateQuota(SQLiteDatabase& database,
                                                const std::string& name)
  {
    SQLiteTimeSeriesTransaction t(database, name);
    
    if (t.maxLength_ != 0)
    {
      while (t.currentLength_ > t.maxLength_)
      {
        t.RemoveOldest();
      }
    }

    if (t.maxSize_ != 0)
    {
      while (t.currentSize_ > t.maxSize_)
      {
        t.RemoveOldest();
      }
    }    

    t.UpdateTimeSeriesTable();

    assert(t.SanityCheck());
  }
}
