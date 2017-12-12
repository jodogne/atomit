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


#include "TimeSeriesReader.h"

#include <Core/OrthancException.h>

#include <cassert>

namespace AtomIT
{
  TimeSeriesReader::Transaction::Transaction(TimeSeriesReader& reader) :
    lock_(reader.accessor_->Lock()),
    valid_(false),
    timestamp_(0)  // Dummy initialization
  {
    if (lock_->HasBackend())
    {
      transaction_.reset(lock_->GetBackend().CreateTransaction(true));

      if (transaction_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }

    
  bool TimeSeriesReader::Transaction::GetTimestamp(int64_t& result) const
  {
    if (valid_)
    {
      result = timestamp_;
      return true;
    }
    else
    {
      return false;
    }
  }
    
      
  bool TimeSeriesReader::Transaction::SeekFirst()
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());

      int64_t first;
      if (transaction_->SeekFirst(first))
      {
        valid_ = true;
        timestamp_ = first;
        return true;
      }
      else
      {
        return false;
      }
    }
  }

    
  bool TimeSeriesReader::Transaction::SeekLast()
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());
          
      int64_t last;
      if (transaction_->SeekLast(last))
      {
        valid_ = true;
        timestamp_ = last;
        return true;
      }
      else
      {
        return false;
      }
    }
  }
    

  void TimeSeriesReader::Transaction::Seek(int64_t timestamp)
  {
    valid_ = true;
    timestamp_ = timestamp;
  }
    

  bool TimeSeriesReader::Transaction::SeekNearest(int64_t timestamp)
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());

      int64_t nearest;
      if (transaction_->SeekNearest(nearest, timestamp))
      {
        valid_ = true;
        timestamp_ = nearest;
        return true;
      }
      else
      {
        return false;
      }
    }
  }

    
  bool TimeSeriesReader::Transaction::SeekNext()
  {
    if (!valid_ ||
        transaction_.get() == NULL)
    {
      return false;
    }
    else
    {
      assert(lock_->HasBackend());

      int64_t next;
      if (transaction_->SeekNext(next, timestamp_))
      {
        valid_ = true;
        timestamp_ = next;
        return true;
      }
      else
      {
        return false;
      }
    }
  }

    
  bool TimeSeriesReader::Transaction::SeekPrevious()
  {
    if (!valid_ ||
        transaction_.get() == NULL)
    {
      return false;
    }
    else
    {
      assert(lock_->HasBackend());

      int64_t previous;
      if (transaction_->SeekPrevious(previous, timestamp_))
      {
        valid_ = true;
        timestamp_ = previous;
        return true;
      }
      else
      {
        return false;
      }
    }
  }
    

  bool TimeSeriesReader::Transaction::Read(std::string& metadata,
                                           std::string& value)
  {
    if (!valid_ ||
        transaction_.get() == NULL)
    {
      return false;
    }
    else
    {
      return transaction_->Read(metadata, value, timestamp_);
    }
  }
  

  void TimeSeriesReader::Transaction::GetStatistics(uint64_t& length,
                                                    uint64_t& size)
  {
    if (transaction_.get() == NULL)
    {
      length = 0;
      size = 0;
    }
    else
    {
      transaction_->GetStatistics(length, size);
    }
  }


  TimeSeriesReader::TimeSeriesReader(ITimeSeriesManager& manager,
                                     const std::string& name,
                                     bool hasSynchronousWait) :
    accessor_(manager.CreateAccessor(name, hasSynchronousWait))
  {
    if (accessor_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
  }
  

  bool TimeSeriesReader::WaitModification(unsigned int milliseconds)
  {
    return accessor_->WaitModification(milliseconds);
  }
}
