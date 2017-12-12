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


#include "SourceFilter.h"

#include "../TimeSeries/TimeSeriesReader.h"

#include <Core/OrthancException.h>
#include <Core/Logging.h>

namespace AtomIT
{
  bool SourceFilter::WaitForRoom()
  {
    if (maxMessages_ != 0)
    {
      TimeSeriesReader reader(manager_, timeSeries_, true);

      {
        uint64_t length, size;

        TimeSeriesReader::Transaction transaction(reader);
        transaction.GetStatistics(length, size);

        if (length < maxMessages_)
        {
          return true;
        }
      }

      // Too many pending messages in the output stream, wait a bit
      reader.WaitModification(100);
      return false;
    }
    else
    {
      return true;
    }
  }


  SourceFilter::SourceFilter(const std::string& name,
                             ITimeSeriesManager& manager,
                             const std::string& timeSeries) :
    name_(name),
    manager_(manager),
    timeSeries_(timeSeries),
    writer_(manager, timeSeries),
    maxMessages_(0),
    defaultTimestampType_(TimestampType_Default)
  {
  }

  
  void SourceFilter::SetDefaultTimestampType(TimestampType type)
  {
    if (type != TimestampType_Default &&
        type != TimestampType_Clock &&
        type != TimestampType_Sequence)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    defaultTimestampType_ = type;
  }


  bool SourceFilter::Step()
  {
    if (WaitForRoom())
    {
      Message message;
      message.SetTimestampType(defaultTimestampType_);

      FetchStatus status = Fetch(message);

      switch (status)
      {
        case FetchStatus_Success:
          LOG(INFO) << "Message received by filter " << GetName() << ": \""
                    << message.FormatValue()
                    << "\" (metadata \"" << message.GetMetadata() << "\")";
            
          writer_.Append(message);
          break;

        case FetchStatus_Invalid:
          break;

        case FetchStatus_Done:
          return false;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          break;
      }
    }

    return true;
  }
}
