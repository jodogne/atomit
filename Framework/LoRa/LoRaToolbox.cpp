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


#include "LoRaToolbox.h"

#include <stdint.h>

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace LoRa
  {
    namespace LoRaToolbox
    {
      static uint8_t GetHexadecimalValue(char c)
      {
        if (c >= 'a' && c <= 'f')
        {
          return static_cast<uint8_t>(c - 'a') + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
          return static_cast<uint8_t>(c - 'A') + 10;
        }
        else if (c >= '0' && c <= '9')
        {
          return static_cast<uint8_t>(c - '0');
        }
        else
        {
          LOG(ERROR) << "Not an hexadecimal character: " << c;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
      }

      static uint8_t GetHexadecimalValue(char high,
                                         char low)
      {
        return (GetHexadecimalValue(high) * 16 +
                GetHexadecimalValue(low));
      }

      static void ToHexadecimalCharacters(char& a,
                                          char& b,
                                          uint8_t value,
                                          bool upcase)
      {
        const uint8_t low = value & 0x0f;
        const uint8_t high = value >> 4;
        const char base = (upcase ? 'A' : 'a');

        assert(low <= 15 && high <= 15);

        if (high < 10)
        {
          a = '0' + high;
        }
        else
        {
          a = base + (high - 10);
        }

        if (low < 10)
        {
          b = '0' + low;
        }
        else
        {
          b = base + (low - 10);
        }
      }

      void ParseHexadecimal(std::string& buffer,
                            const std::string& message)
      {
        if (message.size() % 2 != 0)
        {
          LOG(ERROR) << "The number of hexadecimal characters in a message must be even";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        buffer.resize(message.size() / 2);

        for (size_t i = 0; i < buffer.size(); i++)
        {
          buffer[i] = GetHexadecimalValue(message[2 * i], message[2 * i + 1]);
        }
      }


      std::string FormatHexadecimal(const void* buffer,
                                    size_t size,
                                    bool upcase)
      {
        std::string s;
        s.resize(size * 2);

        const uint8_t* p = reinterpret_cast<const uint8_t*>(buffer);
  
        for (size_t i = 0; i < size; i++, p++)
        {
          ToHexadecimalCharacters(s[2 * i], s[2 * i + 1], *p, upcase);
        }

        return s;
      }

      std::string FormatHexadecimal(const std::string& buffer,
                                    bool upcase)
      {
        return FormatHexadecimal(buffer.c_str(), buffer.size(), upcase);
      }
    }
  }
}
