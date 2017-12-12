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


#include "PHYPayload.h"

#include "LoRaToolbox.h"

#include <Core/Endianness.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace LoRa
  {
    void PHYPayload::Parse()
    {
      if (buffer_.size() < 5)
      {
        LOG(ERROR) << "Too short size of physical payload: " << buffer_.size();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer_.c_str());
      mhdr_ = data[0];

      switch (mhdr_ >> 5)
      {
        case 0:
          type_ = MessageType_JoinRequest;
          break;

        case 1:
          type_ = MessageType_JoinAccept;
          break;

        case 2:
          type_ = MessageType_UnconfirmedDataUp;
          break;

        case 3:
          type_ = MessageType_UnconfirmedDataDown;
          break;

        case 4:
          type_ = MessageType_ConfirmedDataUp;
          break;

        case 5:
          type_ = MessageType_ConfirmedDataDown;
          break;

        case 6:
          type_ = MessageType_Reserved;
          break;

        case 7:
          type_ = MessageType_Proprietary;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      rfu_ = (mhdr_ >> 2) & 0x07;
      major_ = mhdr_ & 0x03;
      mic_ = le32toh(*reinterpret_cast<const uint32_t*>(data + buffer_.size() - 4));
    }
    

    PHYPayload PHYPayload::ParseHexadecimal(const std::string& message)
    {
      PHYPayload result;
      LoRaToolbox::ParseHexadecimal(result.buffer_, message);
      result.Parse();
      return result;
    }

  
    PHYPayload PHYPayload::FromBuffer(const std::string& buffer)
    {
      PHYPayload result;
      result.buffer_ = buffer;
      result.Parse();
      return result;
    }


    PHYPayload PHYPayload::FromBuffer(const void* buffer,
                                      size_t size)
    {
      PHYPayload result;

      result.buffer_.resize(size);
      if (size > 0)
      {
        memcpy(&result.buffer_[0], buffer, size);
      }

      result.Parse();
      return result;
    }


    bool PHYPayload::HasMACPayload() const
    {
      return (type_ == MessageType_UnconfirmedDataUp ||
              type_ == MessageType_UnconfirmedDataDown ||
              type_ == MessageType_ConfirmedDataUp ||
              type_ == MessageType_ConfirmedDataDown);
    }

  
    size_t PHYPayload::GetMACPayloadSize() const
    {
      if (HasMACPayload())
      {
        return buffer_.size() - 5;
      }
      else
      {
        LOG(ERROR) << "No MAC payload";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }
    }


    void PHYPayload::GetMACPayload(std::string& result) const
    {
      if (HasMACPayload())
      {
        result = buffer_.substr(1, buffer_.size() - 5);
      }
      else
      {
        LOG(ERROR) << "No MAC payload";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }
    }
  }
}
