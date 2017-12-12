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


#pragma once

#include "SQLiteDatabase.h"

namespace AtomIT
{
  class SQLiteTimeSeriesTransaction : public boost::noncopyable
  {
  private:
    SQLiteDatabase::Transaction  transaction_;

    int64_t   id_;
    uint64_t  maxLength_;
    uint64_t  maxSize_;
    uint64_t  currentLength_;
    uint64_t  currentSize_;
    bool      hasLastTimestamp_;
    int64_t   lastTimestamp_;

    bool SanityCheck();

    void UpdateTimeSeriesTable();
    
    void RemoveOldest();
    
  public:
    SQLiteTimeSeriesTransaction(SQLiteDatabase& database,
                                const std::string& name);

    ~SQLiteTimeSeriesTransaction();

    void DeleteRange(int64_t start,
                     int64_t end);
    
    bool SeekFirst(int64_t& result);

    bool SeekLast(int64_t& result);
    
    bool SeekNearest(int64_t& result,
                     int64_t timestamp);

    bool SeekNext(int64_t& result,
                  int64_t timestamp);
    
    bool SeekPrevious(int64_t& result,
                      int64_t timestamp);

    bool Read(std::string& metadata,
              std::string& value,
              int64_t timestamp);

    bool Append(int64_t timestamp,
                const std::string& metadata,
                const std::string& value);

    void GetStatistics(uint64_t& length,
                       uint64_t& size);

    void ClearContent();

    bool GetLastTimestamp(int64_t& result);

    static void UpdateQuota(SQLiteDatabase& database,
                            const std::string& name);
  };
}
