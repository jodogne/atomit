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

#include "AtomITEnumerations.h"

#include <stdint.h>
#include <string>
#include <json/value.h>

namespace AtomIT
{
  class Message
  {
  private:
    TimestampType   type_;
    int64_t         timestamp_;
    std::string     metadata_;
    std::string     value_;

  public:
    Message();

    void SetTimestampType(TimestampType type);

    TimestampType GetTimestampType() const
    {
      return type_;
    }

    int64_t GetTimestamp() const;

    void SetTimestamp(int64_t timestamp);

    const std::string& GetMetadata() const
    {
      return metadata_;
    }

    void SetMetadata(const std::string& metadata)
    {
      metadata_ = metadata;
    }

    void SwapMetadata(std::string& metadata)
    {
      metadata_.swap(metadata);
    }

    const std::string& GetValue() const
    {
      return value_;
    }

    void SetValue(const std::string& value)
    {
      value_ = value;
    }

    void SwapValue(std::string& value)
    {
      value_.swap(value);
    }

    std::string FormatValue() const;

    void Format(Json::Value& result) const;
  };
}
