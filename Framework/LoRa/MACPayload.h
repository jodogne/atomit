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

#include "PHYPayload.h"

namespace AtomIT
{
  namespace LoRa
  {
    class MACPayload
    {
    private:
      std::string  buffer_;
      uint32_t     deviceAddress_;
      uint8_t      fctrl_;
      uint16_t     frameCounter_;
      size_t       foptsLength_;
      uint8_t      fport_;
      size_t       frameOffset_;
      size_t       frameSize_;

      void Parse();
    
      MACPayload()
      {
      }

    public:
      explicit MACPayload(const PHYPayload& physical);

      static MACPayload ParseHexadecimal(const std::string& message);
    
      static MACPayload FromBuffer(const std::string& buffer);

      const std::string& GetBuffer() const
      {
        return buffer_;
      }

      uint32_t GetDeviceAddress() const
      {
        return deviceAddress_;
      }

      uint16_t GetFrameCounter() const
      {
        return frameCounter_;
      }

      uint8_t GetFCtrl() const
      {
        return fctrl_;
      }

      size_t GetFOptsLength() const
      {
        return foptsLength_;
      }

      void GetFOpts(std::string& result) const;

      uint8_t GetFPort() const
      {
        return fport_;
      }
    
      size_t GetFrameSize() const
      {
        return frameSize_;
      }

      void GetFramePayload(std::string& result) const;

      bool HasADR() const;

      bool HasACK() const;

      bool HasRFU(MessageDirection direction) const;

      bool HasRFU(MessageType type) const;

      bool GetFPending(MessageDirection direction) const;

      bool GetFPending(MessageType type) const;

      bool HasADRACKReq(MessageDirection direction) const;

      bool HasADRACKReq(MessageType type) const;

      void GetFHDR(std::string& result) const;
    };
  }
}
