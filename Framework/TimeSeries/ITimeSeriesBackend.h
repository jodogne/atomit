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

#include <stdint.h>
#include <string>
#include <boost/noncopyable.hpp>

namespace AtomIT
{
  class ITimeSeriesBackend : public boost::noncopyable
  {
  public:
    class ITransaction : public boost::noncopyable
    {
    public:
      virtual ~ITransaction()
      {
      }

      virtual void ClearContent() = 0;
      
      virtual void DeleteRange(int64_t start,
                               int64_t end) = 0;

      virtual bool SeekFirst(int64_t& result) = 0;

      virtual bool SeekLast(int64_t& result) = 0;

      // Returns the first timestamp that is after or equal to "timestamp"
      virtual bool SeekNearest(int64_t& result,
                               int64_t timestamp) = 0;

      virtual bool SeekNext(int64_t& result,
                            int64_t timestamp) = 0;

      virtual bool SeekPrevious(int64_t& result,
                                int64_t timestamp) = 0;

      virtual bool Read(std::string& metadata,
                        std::string& value,
                        int64_t timestamp) = 0;

      // Returns "false" if not enough space, or if "timestamp <= SeekLast()"
      virtual bool Append(int64_t timestamp,
                          const std::string& metadata,
                          const std::string& value) = 0;

      virtual void GetStatistics(uint64_t& length,
                                 uint64_t& size) = 0;

      virtual bool GetLastTimestamp(int64_t& timestamp) = 0;
    };
    
    virtual ~ITimeSeriesBackend()
    {
    }

    virtual ITransaction* CreateTransaction(bool isReadOnly) = 0;
  };
}
