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

#include <stdint.h>
#include <string>
#include <boost/noncopyable.hpp>

namespace AtomIT
{
  namespace LoRa
  {
    class UnsignedInteger128 : public boost::noncopyable
    {
    public:
      static const size_t SIZE = 16;
    
    private:
      uint8_t  buffer_[SIZE];

    public:
      UnsignedInteger128()
      {
      }
    
      UnsignedInteger128(const UnsignedInteger128& other);
    
      explicit UnsignedInteger128(const uint8_t buffer[SIZE]);

      void Assign(const UnsignedInteger128& other);
      
      void AssignZero();

      void ShiftLeftOneBit();

      uint8_t GetByte(unsigned int pos) const;

      void SetByte(unsigned int pos,
                   uint8_t value);

      void Copy(unsigned int offset,
                const uint8_t* value,
                unsigned int size);

      void Copy(unsigned int offset,
                uint32_t value);
    
      static UnsignedInteger128 ParseHexadecimal(const std::string& buffer);

      std::string Format(bool upcase) const;
    
      const void* GetBuffer() const
      {
        return buffer_;
      }

      void ApplyXOR(const UnsignedInteger128& key);
    
      static UnsignedInteger128 EncryptAES(const UnsignedInteger128& key,
                                           const UnsignedInteger128& data);

      static UnsignedInteger128 DecryptAES(const UnsignedInteger128& key,
                                           const UnsignedInteger128& data);

      void GenerateCMACSubkey(UnsignedInteger128& k1,
                              UnsignedInteger128& k2) const;

      UnsignedInteger128 ComputeCMAC(const std::string& message) const;  };
  }
}
