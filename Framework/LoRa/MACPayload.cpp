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


#include "MACPayload.h"

#include "LoRaToolbox.h"

#include <Core/Endianness.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace LoRa
  {
    static bool HasBit(uint8_t value,
                       unsigned int bit)
    {
      assert(bit <= 7);
      return value & (1 << bit);
    }


    void MACPayload::Parse()
    {
      if (buffer_.size() < 7)
      {
        // DevAddress (4 bytes) + FCtrl (1 byte) + FCnt (2 bytes) are mandatory
        LOG(ERROR) << "Too short MAC payload: " << buffer_.size() << " bytes";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      const uint8_t* data = reinterpret_cast<const uint8_t*>(buffer_.c_str());

      deviceAddress_ = le32toh(*reinterpret_cast<const uint32_t*>(data));
      frameCounter_ = le16toh(*reinterpret_cast<const uint16_t*>(data + 5));

      fctrl_ = data[4];
      foptsLength_ = fctrl_ & 0x0f;

      frameOffset_ = 7 + foptsLength_;

      if (buffer_.size() < frameOffset_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }
      else if (buffer_.size() == frameOffset_)
      {
        // No frame payload (this is allowed by the standard)
        frameSize_ = 0;
        fport_ = 0;
      }
      else
      {
        fport_ = data[frameOffset_];
        frameOffset_ += 1;  // Skip the FPort field

        if (buffer_.size() == frameOffset_ + 1)
        {
          // In theory, this should not be allowed: If the payload is
          // non-empty, the FPort field should be present
          frameSize_ = 0;
        }
        else
        {
          frameSize_ = buffer_.size() - frameOffset_;
        }
      }
    }


    MACPayload::MACPayload(const PHYPayload& physical)
    {
      physical.GetMACPayload(buffer_);
      Parse();
    }

  
    MACPayload MACPayload::ParseHexadecimal(const std::string& message)
    {
      MACPayload result;
      LoRaToolbox::ParseHexadecimal(result.buffer_, message);
      result.Parse();
      return result;
    }

  
    MACPayload MACPayload::FromBuffer(const std::string& buffer)
    {
      MACPayload result;
      result.buffer_ = buffer;
      result.Parse();
      return result;
    }


    void MACPayload::GetFOpts(std::string& result) const
    {
      result = buffer_.substr(7, foptsLength_);
    }


    void MACPayload::GetFramePayload(std::string& result) const
    {
      result = buffer_.substr(frameOffset_, frameSize_);
    }


    bool MACPayload::HasADR() const
    {
      return HasBit(fctrl_, 7);  // Same bit location for uplink and downlink messages
    }

  
    bool MACPayload::HasACK() const
    {
      return HasBit(fctrl_, 5);  // Same bit location for uplink and downlink messages
    }

  
    bool MACPayload::HasRFU(MessageDirection direction) const
    {
      switch (direction)
      {
        case MessageDirection_Downlink:
          return HasBit(fctrl_, 6);

        case MessageDirection_Uplink:
          return HasBit(fctrl_, 4);

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

  
    bool MACPayload::HasRFU(MessageType type) const
    {
      return HasRFU(GetMessageDirection(type));
    }

  
    bool MACPayload::GetFPending(MessageDirection direction) const
    {
      switch (direction)
      {
        case MessageDirection_Downlink:
          return HasBit(fctrl_, 4);

        default:
          LOG(ERROR) << "Only available for downlink messages";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

  
    bool MACPayload::GetFPending(MessageType type) const
    {
      return GetFPending(GetMessageDirection(type));
    }

  
    bool MACPayload::HasADRACKReq(MessageDirection direction) const
    {
      switch (direction)
      {
        case MessageDirection_Uplink:
          return HasBit(fctrl_, 6);

        default:
          LOG(ERROR) << "Only available for downlink messages";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

  
    bool MACPayload::HasADRACKReq(MessageType type) const
    {
      return HasADRACKReq(GetMessageDirection(type));
    }

  
    void MACPayload::GetFHDR(std::string& result) const
    {
      assert(frameOffset_ > 0 &&
             frameOffset_ - 1 <= buffer_.size());
      result = buffer_.substr(0, frameOffset_ - 1);
    }
  }
}
