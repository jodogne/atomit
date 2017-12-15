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


#include "TimeSeriesWriter.h"

#include "../AtomITToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <cassert>

namespace AtomIT
{
  TimeSeriesWriter::Transaction::Transaction(TimeSeriesWriter& writer) :
    lock_(writer.accessor_->Lock()),
    modified_(false)
  {
    if (lock_->HasBackend())
    {
      transaction_.reset(lock_->GetBackend().CreateTransaction(false));

      if (transaction_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }

  
  TimeSeriesWriter::Transaction::~Transaction()
  {
    // Unlock the write transaction so that readers can be
    // notified about modification
    transaction_.reset(NULL);
        
    if (modified_)
    {
      lock_->NotifyModification();
    }
  }

      
  bool TimeSeriesWriter::Transaction::GetLastTimestamp(int64_t& timestamp) const
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());
      return transaction_->GetLastTimestamp(timestamp);
    }
  }
      

  bool TimeSeriesWriter::Transaction::Append(int64_t timestamp,
                                             const std::string& metadata,
                                             const std::string& value)
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());
      
      if (transaction_->Append(timestamp, metadata, value))
      {
        modified_ = true;
        return true;
      }
      else
      {
        return false;
      }
    }
  }
      

  bool TimeSeriesWriter::Transaction::DeleteRange(int64_t start,
                                                  int64_t end)
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
      return false;
    }
    else
    {
      assert(lock_->HasBackend());
      transaction_->DeleteRange(start, end);
      modified_ = true;
      return true;
    }
  }

  
  void TimeSeriesWriter::Transaction::ClearContent()
  {
    if (transaction_.get() == NULL)
    {
      assert(!lock_->HasBackend());
    }
    else
    {
      assert(lock_->HasBackend());
      transaction_->ClearContent();
      modified_ = true;
    }
  }


  TimestampType TimeSeriesWriter::Transaction::GetDefaultTimestampType() const
  {
    return lock_->GetDefaultTimestampType();
  }

  
  bool TimeSeriesWriter::Append(const Message& message)
  {
    Transaction transaction(*this);

    TimestampType type = message.GetTimestampType();
    if (type == TimestampType_Default)
    {
      type = transaction.GetDefaultTimestampType();
    }

    int64_t timestamp;

    switch (type)
    {
      case TimestampType_Fixed:
        timestamp = message.GetTimestamp();
        break;

      case TimestampType_NanosecondsClock:
        timestamp = Toolbox::GetNanosecondsClockTimestamp();
        break;

      case TimestampType_MillisecondsClock:
        timestamp = Toolbox::GetMillisecondsClockTimestamp();
        break;

      case TimestampType_SecondsClock:
        timestamp = Toolbox::GetSecondsClockTimestamp();
        break;

      case TimestampType_Sequence:
        if (transaction.GetLastTimestamp(timestamp))
        {
          timestamp += 1;
        }
        else
        {
          timestamp = 0;   // The sequence is empty
        }

        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (transaction.Append(timestamp, message.GetMetadata(), message.GetValue()))
    {
      return true;
    }
    else
    {
      LOG(ERROR) << "Adding an item whose timestamp (" << timestamp
                 << ") is not after the last item of the time series";
      return false;
    }
  }
  
    
  TimeSeriesWriter::TimeSeriesWriter(ITimeSeriesManager& manager,
                                     const std::string& name) :
    accessor_(manager.CreateAccessor(name, false))
  {
    if (accessor_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
  }
}
