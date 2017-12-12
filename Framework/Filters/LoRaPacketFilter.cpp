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


#include "LoRaPacketFilter.h"

#include "../LoRa/LoRaToolbox.h"
#include "../LoRa/MACPayload.h"

#include <stdio.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  AdapterFilter::PushStatus LoRaPacketFilter::Push(const Message& message)
  {
    try
    {
      LoRa::PHYPayload phy = LoRa::PHYPayload::FromBuffer(message.GetValue());
      //LOG(INFO) << "Raw packet: " << LoRa::FormatHexadecimal(phy.GetBuffer(), true);
        
      LoRa::MACPayload mac(phy);
      char buf[32];
      sprintf(buf, "%04X", mac.GetDeviceAddress());
      std::string payload;
      mac.GetFramePayload(payload);
      LOG(INFO) << "Decoded packet from device "
                << std::string(buf)
                << ": " << LoRa::LoRaToolbox::FormatHexadecimal(payload, true);

      if (nwkSKey_.CheckMIC(phy, 0))
      {
        std::string value;
        appSKey_.Apply(value, phy, 0);
        LOG(INFO) << "Decrypted: " << LoRa::LoRaToolbox::FormatHexadecimal(value, true);

        {
          Message output;
          output.SetTimestamp(message.GetTimestamp());
          output.SetMetadata(std::string(buf));  // Use device address as metadata
          output.SetValue(value);
          writer_.Append(output);
        }
          
        return PushStatus_Success;
      }
      else
      {
        LOG(INFO) << "Bad MIC for packet";
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(INFO) << "Cannot decode packet: " << e.What();
    }

    return PushStatus_Failure;  // TODO
  }

  
  LoRaPacketFilter::LoRaPacketFilter(const std::string& name,
                                     ITimeSeriesManager& manager,
                                     const std::string& inputTimeSeries,
                                     const std::string& outputTimeSeries,
                                     const std::string& nwkSKey,
                                     const std::string& appSKey) :
    AdapterFilter(name, manager, inputTimeSeries),
    writer_(manager, outputTimeSeries),
    nwkSKey_(LoRa::FrameEncryptionKey::ParseHexadecimal(nwkSKey)),
    appSKey_(LoRa::FrameEncryptionKey::ParseHexadecimal(appSKey))
  {
  }
}
