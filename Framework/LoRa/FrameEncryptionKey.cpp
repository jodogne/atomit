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


#include "FrameEncryptionKey.h"

#include "LoRaToolbox.h"
#include "MACPayload.h"

#include <Core/Endianness.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace LoRa
  {
    void FrameEncryptionKey::PrepareMainBlock(UnsignedInteger128& block,
                                              MessageDirection direction,
                                              const uint32_t deviceAddress,
                                              uint32_t frameCounter,  // WARNING: Not uint16_t!
                                              uint8_t headerByte,
                                              uint8_t trailerByte) const
    {
      block.AssignZero();
      block.SetByte(0, headerByte);
      block.SetByte(5, (direction == MessageDirection_Uplink ? 0 : 1));
      block.Copy(6, htole32(deviceAddress));
      block.Copy(10, htole32(frameCounter));
      block.SetByte(15, trailerByte);
    }
                          

    void FrameEncryptionKey::PrepareSessionKey(std::string& result,
                                               MessageDirection direction,
                                               const uint32_t deviceAddress,
                                               uint32_t frameCounter,  // WARNING: Not uint16_t!
                                               size_t frameSize) const
    {
      size_t blocks = LoRaToolbox::CeilingDivision(frameSize, 16);

      UnsignedInteger128 key(key_);  // For const-correctness

      // Create the main block that will be copied "blocks" time after encryption
      UnsignedInteger128 mainBlock;
      PrepareMainBlock(mainBlock, direction, deviceAddress, frameCounter, 0x01, 0);

      // Encrypt the main block until filling the target buffer
      result.resize(blocks * 16);

      uint8_t* target = reinterpret_cast<uint8_t*>(&result[0]);
      for (size_t i = 0; i < blocks; i++)
      {
        mainBlock.SetByte(15, i + 1);

        UnsignedInteger128 encrypted = UnsignedInteger128::EncryptAES(key, mainBlock);

        memcpy(target, encrypted.GetBuffer(), 16);
        target += 16;
      }
    }


    FrameEncryptionKey FrameEncryptionKey::ParseHexadecimal(const std::string& key)
    {
      return FrameEncryptionKey(UnsignedInteger128::ParseHexadecimal(key));
    }


    void FrameEncryptionKey::Apply(std::string& target,
                                   const std::string& source,
                                   MessageDirection direction,
                                   const uint32_t deviceAddress,
                                   uint32_t frameCounter) const
    {
      std::string session;
      PrepareSessionKey(session, direction, deviceAddress, frameCounter, source.size());

      target.resize(source.size());
      for (size_t i = 0; i < source.size(); i++)
      {
        target[i] = source[i] ^ session[i];
      }
    }


    void FrameEncryptionKey::Apply(std::string& target,
                                   const PHYPayload& payload,
                                   uint16_t highFrameCounter) const
    {
      MACPayload mac(payload);
      uint32_t frameCounter = (static_cast<uint32_t>(mac.GetFrameCounter()) +
                               (static_cast<uint32_t>(highFrameCounter) << 16));

      std::string source;
      mac.GetFramePayload(source);
      
      Apply(target, source,
            payload.GetMessageDirection(),
            mac.GetDeviceAddress(),
            frameCounter);            
    }


    uint32_t FrameEncryptionKey::ComputeMIC(const PHYPayload& payload,
                                            uint16_t highFrameCounter)
    {
      MACPayload mac(payload);
      uint32_t frameCounter = (static_cast<uint32_t>(mac.GetFrameCounter()) +
                               (static_cast<uint32_t>(highFrameCounter) << 16));

      uint8_t mhdr = payload.GetMHDR();
      uint8_t fport = htole16(mac.GetFPort());      
      
      std::string fhdr, frame;
      mac.GetFHDR(fhdr);
      mac.GetFramePayload(frame);

      // msg = MHDR | FHDR | FPort | FRMPayload
      size_t msgSize = sizeof(mhdr) + fhdr.size() + sizeof(fport) + frame.size();
      if (msgSize > 255)
      {
        LOG(ERROR) << "Too long message: " << msgSize << " bytes";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      UnsignedInteger128 b0;
      PrepareMainBlock(b0, payload.GetMessageDirection(), mac.GetDeviceAddress(),
                       frameCounter, 0x49, msgSize);

      std::string msg;
      msg.resize(msgSize + 16);

      uint8_t* buffer = reinterpret_cast<uint8_t*>(&msg[0]);
      memcpy(buffer, b0.GetBuffer(), 16);
      memcpy(buffer + 16, &mhdr, sizeof(mhdr));
      memcpy(buffer + 16 + sizeof(mhdr), fhdr.c_str(), fhdr.size());
      memcpy(buffer + 16 + sizeof(mhdr) + fhdr.size(), &fport, sizeof(fport));
      memcpy(buffer + 16 + sizeof(mhdr) + fhdr.size() + sizeof(fport),
             frame.c_str(), frame.size());

      UnsignedInteger128 cmac = key_.ComputeCMAC(msg);

      uint32_t mic;
      memcpy(&mic, cmac.GetBuffer(), sizeof(mic));
      return le32toh(mic);
    }


    bool FrameEncryptionKey::CheckMIC(const PHYPayload& payload,
                                      uint16_t highFrameCounter)
    {
      return payload.GetMIC() == ComputeMIC(payload, highFrameCounter);
    }
  }
}
