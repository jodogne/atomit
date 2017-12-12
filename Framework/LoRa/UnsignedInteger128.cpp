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


#include "UnsignedInteger128.h"

#include "LoRaToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

extern "C"
{
#include "../../Resources/ThirdParty/tiny-AES-c-master/aes.h"
}

#include <string.h>


namespace AtomIT
{
  namespace LoRa
  {
    UnsignedInteger128::UnsignedInteger128(const UnsignedInteger128& other)
    {
      memcpy(buffer_, other.buffer_, SIZE);
    }

  
    UnsignedInteger128::UnsignedInteger128(const uint8_t buffer[SIZE])
    {
      memcpy(buffer_, buffer, SIZE);
    }

  
    void UnsignedInteger128::Assign(const UnsignedInteger128& other)
    {
      memcpy(&buffer_, &other.buffer_, SIZE);
    }
      

    void UnsignedInteger128::AssignZero()
    {
      memset(buffer_, 0, SIZE);
    }

  
    void UnsignedInteger128::ShiftLeftOneBit()
    {
      bool overflow = false;
      
      for (unsigned int i = SIZE; i > 0; i--)
      {
        bool nextOverflow = (buffer_[i - 1] & 0x80);
        buffer_[i - 1] = (buffer_[i - 1] << 1) + (overflow ? 1 : 0);
        overflow = nextOverflow;
      }
    }

  
    uint8_t UnsignedInteger128::GetByte(unsigned int pos) const
    {
      if (pos < SIZE)
      {
        return buffer_[pos];
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

  
    void UnsignedInteger128::SetByte(unsigned int pos,
                                     uint8_t value)
    {
      if (pos < SIZE)
      {
        buffer_[pos] = value;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

  
    void UnsignedInteger128::Copy(unsigned int offset,
                                  const uint8_t* value,
                                  unsigned int size)
    {
      if (offset + size > SIZE)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        memcpy(buffer_ + offset, value, size);
      }
    }

  
    void UnsignedInteger128::Copy(unsigned int offset,
                                  uint32_t value)
    {
      Copy(offset, reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    } 

    
    UnsignedInteger128 UnsignedInteger128::ParseHexadecimal(const std::string& buffer)
    {
      std::string parsed;
      LoRaToolbox::ParseHexadecimal(parsed, buffer);

      if (parsed.size() != SIZE)
      {
        LOG(ERROR) << "Encryption keys must have 128 bits";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      
      UnsignedInteger128 result;
      memcpy(&result.buffer_, parsed.c_str(), SIZE);
      return result;
    }

    
    std::string UnsignedInteger128::Format(bool upcase) const
    {
      return LoRaToolbox::FormatHexadecimal(buffer_, SIZE, upcase);
    }
    

    void UnsignedInteger128::ApplyXOR(const UnsignedInteger128& key)
    {
      for (unsigned int i = 0; i < SIZE; i++)
      {
        buffer_[i] = buffer_[i] ^ key.buffer_[i];
      }
    }

    
    UnsignedInteger128 UnsignedInteger128::EncryptAES(const UnsignedInteger128& key,
                                                      const UnsignedInteger128& data)
    {
      UnsignedInteger128 result;
      AES_ECB_encrypt(data.buffer_, key.buffer_, result.buffer_, 16);
      return result;
    }


    UnsignedInteger128 UnsignedInteger128::DecryptAES(const UnsignedInteger128& key,
                                                      const UnsignedInteger128& data)
    {
      UnsignedInteger128 result;
      AES_ECB_decrypt(data.buffer_, key.buffer_, result.buffer_, 16);
      return result;
    }


    void UnsignedInteger128::GenerateCMACSubkey(UnsignedInteger128& k1,
                                                UnsignedInteger128& k2) const
    {
      UnsignedInteger128 zero, rb;
      zero.AssignZero();
      rb.AssignZero();
      rb.SetByte(15, 0x87);

      const UnsignedInteger128 l = EncryptAES(*this, zero);

      k1.Assign(l);
      k1.ShiftLeftOneBit();

      if (l.GetByte(0) & 0x80)
      {
        k1.ApplyXOR(rb);
      }

      k2.Assign(k1);
      k2.ShiftLeftOneBit();

      if (k1.GetByte(0) & 0x80)
      {
        k2.ApplyXOR(rb);
      }
    }


    UnsignedInteger128 UnsignedInteger128::ComputeCMAC(const std::string& message) const
    {
      // This is RFC4493
      // https://tools.ietf.org/html/rfc4493
      
      UnsignedInteger128 k1, k2;
      GenerateCMACSubkey(k1, k2);

      const uint8_t* data = reinterpret_cast<const uint8_t*>(message.c_str());

      size_t n = LoRaToolbox::CeilingDivision(message.size(), SIZE);
      bool flag;

      if (n == 0)
      {
        n = 1;
        flag = false;
      }
      else
      {
        flag = (message.size() % SIZE == 0);
      }

      UnsignedInteger128 lastBlock;
      size_t lastOffset = SIZE * (n - 1);

      if (flag)
      {
        assert(lastOffset + SIZE == message.size());
        lastBlock.Copy(0, data + lastOffset, SIZE);

        lastBlock.ApplyXOR(k1);
      }
      else
      {
        assert(lastOffset <= message.size());
        
        size_t length = message.size() - lastOffset;
        assert(length < SIZE);

        lastBlock.AssignZero();
        lastBlock.Copy(0, data + lastOffset, length);
        lastBlock.SetByte(length, 0x80);

        lastBlock.ApplyXOR(k2);
      }

      UnsignedInteger128 x;
      x.AssignZero();

      for (size_t i = 0; i + 1 < n; i++)
      {
        UnsignedInteger128 block;
        block.Copy(0, data + SIZE * i, SIZE);
        x.ApplyXOR(block);
        x.Assign(EncryptAES(*this, x));
      }

      x.ApplyXOR(lastBlock);
      return EncryptAES(*this, x);
    }
  }
}
