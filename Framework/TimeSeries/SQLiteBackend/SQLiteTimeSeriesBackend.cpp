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


#include "SQLiteTimeSeriesBackend.h"

#include <Core/Logging.h>

namespace AtomIT
{
  class SQLiteTimeSeriesBackend::Transaction : public ITimeSeriesBackend::ITransaction
  {
  private:
    SQLiteTimeSeriesTransaction  transaction_;

  public:
    explicit Transaction(SQLiteTimeSeriesBackend& backend) :
      transaction_(backend.database_, backend.name_)
    {
    }

    virtual void ClearContent()
    {
      transaction_.ClearContent();
    }

    virtual void DeleteRange(int64_t start,
                             int64_t end)
    {
      transaction_.DeleteRange(start, end);
    }

    virtual bool SeekFirst(int64_t& result)
    {
      return transaction_.SeekFirst(result);
    }

    virtual bool SeekLast(int64_t& result)
    {
      return transaction_.SeekLast(result);
    }

    virtual bool SeekNearest(int64_t& result,
                             int64_t timestamp)
    {
      return transaction_.SeekNearest(result, timestamp);
    }

    virtual bool SeekNext(int64_t& result,
                          int64_t timestamp)
    {
      return transaction_.SeekNext(result, timestamp);
    }

    virtual bool SeekPrevious(int64_t& result,
                              int64_t timestamp)
    {
      return transaction_.SeekPrevious(result, timestamp);
    }

    virtual bool Read(std::string& metadata,
                      std::string& value,
                      int64_t timestamp)
    {
      return transaction_.Read(metadata, value, timestamp);
    }

    virtual bool Append(int64_t timestamp,
                        const std::string& metadata,
                        const std::string& value)
    {
      return transaction_.Append(timestamp, metadata, value);
    }

    virtual void GetStatistics(uint64_t& length,
                               uint64_t& size)
    {
      transaction_.GetStatistics(length, size);
    }

    virtual bool GetLastTimestamp(int64_t& result)
    {
      return transaction_.GetLastTimestamp(result);
    }
  };


  SQLiteTimeSeriesBackend::SQLiteTimeSeriesBackend(SQLiteDatabase& database,
                                                   const std::string& name) :
    database_(database),
    name_(name)
  {
    LOG(INFO) << "Accessing SQLite time series: " << name;
  }

  
  ITimeSeriesBackend::ITransaction* SQLiteTimeSeriesBackend::CreateTransaction(bool isReadOnly)
  {
    return new Transaction(*this);
  }
}
