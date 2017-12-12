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

#include "LoRaEnumerations.h"

#include <stdint.h>
#include <string>

namespace AtomIT
{
  namespace LoRa
  {
    class PHYPayload
    {
    private:
      std::string   buffer_;
      MessageType   type_;
      uint8_t       mhdr_;
      uint8_t       rfu_;
      uint8_t       major_;
      uint32_t      mic_;

      PHYPayload()
      {
      }

      void Parse();
    
    public:
      static PHYPayload ParseHexadecimal(const std::string& message);
    
      static PHYPayload FromBuffer(const std::string& buffer);

      static PHYPayload FromBuffer(const void* buffer,
                                   size_t size);

      const std::string& GetBuffer() const
      {
        return buffer_;
      }

      MessageType GetMessageType() const
      {
        return type_;
      }

      MessageDirection GetMessageDirection() const
      {
        return ::AtomIT::LoRa::GetMessageDirection(type_);
      }

      uint8_t GetMHDR() const
      {
        return mhdr_;
      }

      uint8_t GetMajor() const
      {
        return major_;
      }

      uint8_t GetRFU() const
      {
        return rfu_;
      }

      uint32_t GetMIC() const
      {
        return mic_;
      }

      bool HasMACPayload() const;

      size_t GetMACPayloadSize() const;

      void GetMACPayload(std::string& result) const;
    };
  }
}
