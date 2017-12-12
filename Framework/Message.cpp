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


#include "Message.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace AtomIT
{
  Message::Message() :
    type_(TimestampType_Default),
    timestamp_(0)
  {
  }

  
  void Message::SetTimestampType(TimestampType type)
  {
    if (type == TimestampType_Fixed)
    {
      // Use "SetTimestamp()" to fix the timestamp
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
    }
    else
    {
      type_ = type;
    }
  }

    
  int64_t Message::GetTimestamp() const
  {
    if (type_ != TimestampType_Fixed)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
    }
    else
    {
      return timestamp_;
    }
  }


  void Message::SetTimestamp(int64_t timestamp)
  {
    type_ = TimestampType_Fixed;
    timestamp_ = timestamp;
  }

    
  std::string Message::FormatValue() const
  {
    if (Orthanc::Toolbox::IsAsciiString(value_.c_str(), value_.size()))
    {
      return value_;
    }
    else
    {
      return "(binary)";
    }
  }


  void Message::Format(Json::Value& result) const
  {
    result = Json::objectValue;

    if (type_ == TimestampType_Fixed)
    {
      result["timestamp"] = static_cast<Json::Value::UInt64>(timestamp_);
    }

    result["metadata"] = metadata_;

    if (Orthanc::Toolbox::IsAsciiString(value_.c_str(), value_.size()))
    {
      result["value"] = value_;
      result["base64"] = false;
    }
    else
    {
      std::string s;
      Orthanc::Toolbox::EncodeBase64(s, value_);
      result["value"] = s;
      result["base64"] = true;
    }
  }
}
