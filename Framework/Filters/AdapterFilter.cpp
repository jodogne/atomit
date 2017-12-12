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


#include "AdapterFilter.h"

#include <Core/OrthancException.h>
#include <Core/Logging.h>

namespace AtomIT
{
  AdapterFilter::AdapterFilter(const std::string& name,
                               ITimeSeriesManager& manager,
                               const std::string& timeSeries) :
    name_(name),
    manager_(manager),
    timeSeries_(timeSeries),
    reader_(manager, timeSeries, true),
    replayHistory_(false),
    isValid_(false),
    timestamp_(0)  // Dummy initialization
  {
  }


  void AdapterFilter::SetPopInput(bool pop)
  {
    if (pop)
    {
      inputPopper_.reset(new TimeSeriesWriter(manager_, timeSeries_));
    }
    else
    {
      inputPopper_.reset(NULL);
    }
  }


  bool AdapterFilter::IsPopInput() const
  {
    return inputPopper_.get() != NULL;
  }
    

  void AdapterFilter::Start()
  {
    isValid_ = false;
      
    if (!replayHistory_)
    {
      // Ignore the history, only take new incoming messages into
      // consideration
      TimeSeriesReader::Transaction transaction(reader_);

      int64_t last;     
      if (transaction.SeekLast() &&
          transaction.GetTimestamp(last))
      {
        // The time series is not empty, set the reading head over
        // the last item in the time series (*)
        isValid_ = true;
        timestamp_ = last;
      }
    }
  }

  
  bool AdapterFilter::Step()
  {
    bool ok = false;
    int64_t timestamp = 0;  // Dummy initialization
    std::string metadata, value;

    {
      // Lock the input series as few as possible
      TimeSeriesReader::Transaction transaction(reader_);

      if (isValid_)
      {
        // Lookup for the item in the time series that is just after
        // the last-consumed item
        ok = transaction.SeekNearest(timestamp_ + 1);
      }
      else
      {
        // The input time series was empty at the time "Start()" was
        // called (*), or the source is asked to replay the history of
        // the time series (replayHistory_ is true).
        ok = transaction.SeekFirst();
      }

      if (ok)
      {
        ok = (transaction.GetTimestamp(timestamp) &&
              transaction.Read(metadata, value));
      }
    }

    if (ok)
    {
      Message message;
      message.SetTimestamp(timestamp);
      message.SwapMetadata(metadata);
      message.SwapValue(value);

      PushStatus status = Push(message);

      switch (status)
      {
        case PushStatus_Success:
        case PushStatus_Failure:
          // Success or failure. In both cases, advance the reading
          // head to the next message in the time series.
          isValid_ = true;
          timestamp_ = timestamp;
          break;

        case PushStatus_Retry:
          // Will retry with the same message in the time series
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
        
      if (status == PushStatus_Success)
      {
        if (inputPopper_.get() != NULL)
        {
          LOG(INFO) << "Removing timestamp " << timestamp
                    << " from time series \"" << timeSeries_ << "\"";
            
          // Pop the incoming message from the input time series
          TimeSeriesWriter::Transaction transaction(*inputPopper_);
          transaction.DeleteRange(timestamp, timestamp + 1);
        }
      }
    }
    else
    {
      // The input time series is empty, wait a bit for new messages
      reader_.WaitModification(500);
    }

    return true;
  }
}
