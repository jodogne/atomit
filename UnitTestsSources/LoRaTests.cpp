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


#include <gtest/gtest.h>

#include "../Framework/LoRa/FrameEncryptionKey.h"
#include "../Framework/LoRa/MACPayload.h"
#include "../Framework/LoRa/LoRaToolbox.h"

#include <Core/Endianness.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

TEST(Toolbox, CeilingDivision)
{
  ASSERT_EQ(5, AtomIT::LoRa::LoRaToolbox::CeilingDivision(17, 4));
  ASSERT_EQ(4, AtomIT::LoRa::LoRaToolbox::CeilingDivision(16, 4));
  ASSERT_EQ(4, AtomIT::LoRa::LoRaToolbox::CeilingDivision(15, 4));
}


TEST(LoRa, PHYPayload1)
{
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40C51C012600010001FC98BDB0D4C6");

  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(phy.GetBuffer(), true), "40C51C012600010001FC98BDB0D4C6");
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(phy.GetBuffer(), false), "40c51c012600010001fc98bdb0d4c6");
  ASSERT_EQ(phy.GetMHDR(), 0x40);
  ASSERT_EQ(phy.GetMIC(), be32toh(0xBDB0D4C6));
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_EQ(phy.GetRFU(), 0);
  ASSERT_EQ(phy.GetMajor(), 0);
  ASSERT_TRUE(phy.HasMACPayload()); 
  ASSERT_EQ(phy.GetMACPayloadSize(), 10u);

  std::string s;
  phy.GetMACPayload(s);
  ASSERT_EQ(s.size(), phy.GetMACPayloadSize());

  AtomIT::LoRa::MACPayload mac(phy);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(mac.GetBuffer(), true), "C51C012600010001FC98");
  ASSERT_EQ(mac.GetDeviceAddress(), 0x26011CC5u);  // Should also work on big-endian
  ASSERT_EQ(mac.GetFCtrl(), 0);
  ASSERT_EQ(mac.GetFrameCounter(), 1);
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFrameSize(), 2u);
  ASSERT_EQ(mac.GetFPort(), 1);
  ASSERT_FALSE(mac.HasRFU(phy.GetMessageType()));
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
  ASSERT_FALSE(mac.HasADRACKReq(phy.GetMessageType()));

  mac.GetFOpts(s);
  ASSERT_EQ(s.size(), 0u);

  mac.GetFHDR(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "C51C0126000100");
  
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "FC98");

  {
    // Take AppSKey, as mac.GetFPort() is not zero
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("b4661c6bf2dd3920e3a256f760aacc69");
    ASSERT_EQ(encryption.FormatKey(true), "B4661C6BF2DD3920E3A256F760AACC69");

    std::string t;
    encryption.Apply(t, s, phy.GetMessageDirection(), mac.GetDeviceAddress(), mac.GetFrameCounter());
    ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(t, true), "F700");

    t.clear();
    encryption.Apply(t, phy, 0);
    ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(t, true), "F700");

    ASSERT_FALSE(encryption.CheckMIC(phy, 0));
  }

  {
    // Take NwkSKey
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("C980917342CB4AF14E9EBB07BE792031");
    ASSERT_EQ(encryption.ComputeMIC(phy, 0), phy.GetMIC());
    ASSERT_TRUE(encryption.CheckMIC(phy, 0));
  }
}


TEST(LoRa, PHYPayload2)
{
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40C51C01260000000178A6E7257065");

  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(phy.GetBuffer(), true), "40C51C01260000000178A6E7257065");
  ASSERT_EQ(phy.GetMHDR(), 0x40);
  ASSERT_EQ(phy.GetMIC(), 0x657025E7u);
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_EQ(phy.GetRFU(), 0);
  ASSERT_EQ(phy.GetMajor(), 0);
  ASSERT_TRUE(phy.HasMACPayload());
  ASSERT_EQ(phy.GetMACPayloadSize(), 10u);

  std::string s;
  phy.GetMACPayload(s);
  ASSERT_EQ(s.size(), phy.GetMACPayloadSize());

  AtomIT::LoRa::MACPayload mac(phy);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(mac.GetBuffer(), true), "C51C01260000000178A6");
  ASSERT_EQ(mac.GetDeviceAddress(), 0x26011CC5u);  // Should also work on big-endian
  ASSERT_EQ(mac.GetFCtrl(), 0);
  ASSERT_EQ(mac.GetFrameCounter(), 0);
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFrameSize(), 2u);
  ASSERT_EQ(mac.GetFPort(), 1);
  ASSERT_FALSE(mac.HasRFU(phy.GetMessageType()));
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
  ASSERT_FALSE(mac.HasADRACKReq(phy.GetMessageType()));

  mac.GetFOpts(s);
  ASSERT_EQ(s.size(), 0u);
  
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "78A6");

  {
    // Take AppSKey, as mac.GetFPort() is not zero
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("b4661c6bf2dd3920e3a256f760aacc69");
      
    std::string t;
    encryption.Apply(t, s, phy.GetMessageDirection(), mac.GetDeviceAddress(), mac.GetFrameCounter());
    ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(t, true), "F700");

    t.clear();
    encryption.Apply(t, phy, 0);
    ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(t, true), "F700");

    ASSERT_FALSE(encryption.CheckMIC(phy, 0));
  }

  {
    // Take NwkSKey
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("C980917342CB4AF14E9EBB07BE792031");
    ASSERT_EQ(encryption.ComputeMIC(phy, 0), phy.GetMIC());
    ASSERT_TRUE(encryption.CheckMIC(phy, 0));
  }
}


TEST(LoRa, PHYPayload3)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/demo/demo1.js
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954378762B11FF0D");

  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(phy.GetBuffer(), true), "40F17DBE4900020001954378762B11FF0D");
  ASSERT_EQ(phy.GetMHDR(), 0x40);
  ASSERT_EQ(phy.GetMIC(), 0x0DFF112Bu);
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_EQ(phy.GetRFU(), 0);
  ASSERT_EQ(phy.GetMajor(), 0);
  ASSERT_TRUE(phy.HasMACPayload());
  ASSERT_EQ(phy.GetMACPayloadSize(), 12u);

  std::string s;
  phy.GetMACPayload(s);
  ASSERT_EQ(s.size(), phy.GetMACPayloadSize());

  AtomIT::LoRa::MACPayload mac(phy);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(mac.GetBuffer(), true), "F17DBE490002000195437876");
  ASSERT_EQ(mac.GetDeviceAddress(), 0x49BE7DF1u);  // Should also work on big-endian
  ASSERT_EQ(mac.GetFCtrl(), 0);
  ASSERT_EQ(mac.GetFrameCounter(), 2);
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFrameSize(), 4u);
  ASSERT_EQ(mac.GetFPort(), 1);
  ASSERT_FALSE(mac.HasRFU(phy.GetMessageType()));
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
  ASSERT_FALSE(mac.HasADRACKReq(phy.GetMessageType()));

  mac.GetFOpts(s);
  ASSERT_EQ(s.size(), 0u);
  
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "95437876");

  {
    // Take AppSKey, as mac.GetFPort() is not zero
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("ec925802ae430ca77fd3dd73cb2cc588");
      
    std::string t;
    encryption.Apply(t, s, phy.GetMessageDirection(), mac.GetDeviceAddress(), mac.GetFrameCounter());
    ASSERT_EQ(t, "test");

    t.clear();
    encryption.Apply(t, phy, 0);
    ASSERT_EQ(t, "test");
  }

  {
    // Take NwkSKey
    AtomIT::LoRa::FrameEncryptionKey encryption =
      AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("44024241ed4ce9a68c6a8bc055233fd3");
    ASSERT_EQ(encryption.ComputeMIC(phy, 0), phy.GetMIC());
  }
}


TEST(LoRa, PHYPayload4)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_parse.js
  // parse example packet
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954378762B11FF0D");
  AtomIT::LoRa::MACPayload mac(phy);

  std::string s;

  ASSERT_EQ(phy.GetMHDR(), 0x40);
  phy.GetMACPayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE490002000195437876");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x2b11ff0d));
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFCtrl(), 0);
  mac.GetFHDR(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE49000200");
  ASSERT_EQ(mac.GetDeviceAddress(), be32toh(0xF17DBE49));
  ASSERT_EQ(mac.GetFrameCounter(), 2);
  ASSERT_EQ(mac.GetFPort(), 1);
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "95437876");
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
}


TEST(LoRa, PHYPayload5)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_parse.js
  // should parse packet with empty payload
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE49000300012A3518AF");
  AtomIT::LoRa::MACPayload mac(phy);

  std::string s;

  ASSERT_EQ(phy.GetMHDR(), 0x40);
  phy.GetMACPayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE4900030001");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x2A3518AF));
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFCtrl(), 0);
  mac.GetFHDR(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE49000300");
  ASSERT_EQ(mac.GetDeviceAddress(), be32toh(0xF17DBE49));
  ASSERT_EQ(mac.GetFrameCounter(), 3);
  ASSERT_EQ(mac.GetFPort(), 1);
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "");
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
}


TEST(LoRa, PHYPayload6)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_parse.js
  // should parse large packet
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40f17dbe490004000155332de41a11adc072553544429ce7787707d1c316e027e7e5e334263376affb8aa17ad30075293f28dea8a20af3c5e7");
  AtomIT::LoRa::MACPayload mac(phy);

  std::string s;

  ASSERT_EQ(phy.GetMHDR(), 0x40);
  phy.GetMACPayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE490004000155332DE41A11ADC072553544429CE7787707D1C316E027E7E5E334263376AFFB8AA17AD30075293F28DEA8A2");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x0af3c5e7));
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFCtrl(), 0);
  mac.GetFHDR(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE49000400");
  ASSERT_EQ(mac.GetDeviceAddress(), be32toh(0xf17dbe49));
  ASSERT_EQ(mac.GetFrameCounter(), 4);
  ASSERT_EQ(mac.GetFPort(), 1);
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "55332DE41A11ADC072553544429CE7787707D1C316E027E7E5E334263376AFFB8AA17AD30075293F28DEA8A2");
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataUp);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Uplink);
  ASSERT_FALSE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
}


TEST(LoRa, PHYPayload7)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_parse.js
  // should parse ack
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("60f17dbe4920020001f9d65d27");
  AtomIT::LoRa::MACPayload mac(phy);

  std::string s;

  ASSERT_EQ(phy.GetMHDR(), 0x60);
  phy.GetMACPayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE4920020001");
  ASSERT_EQ(phy.GetMIC(), be32toh(0xf9d65d27));
  ASSERT_EQ(mac.GetFOptsLength(), 0u);
  ASSERT_EQ(mac.GetFCtrl(), 0x20);
  mac.GetFHDR(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "F17DBE49200200");
  ASSERT_EQ(mac.GetDeviceAddress(), be32toh(0xf17dbe49));
  ASSERT_EQ(mac.GetFrameCounter(), 2);
  ASSERT_EQ(mac.GetFPort(), 1);
  mac.GetFramePayload(s);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "");
  ASSERT_EQ(phy.GetMessageType(), AtomIT::LoRa::MessageType_UnconfirmedDataDown);
  ASSERT_EQ(phy.GetMessageDirection(), AtomIT::LoRa::MessageDirection_Downlink);
  ASSERT_TRUE(mac.HasACK());
  ASSERT_FALSE(mac.HasADR());
}


TEST(LoRa, Decrypt1)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_decrypt.js
  // should decrypt test payload
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954378762B11FF0D");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("ec925802ae430ca77fd3dd73cb2cc588");
  std::string s;
  key.Apply(s, phy, 0);
  ASSERT_EQ(s, "test");
}


TEST(LoRa, Decrypt2)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_decrypt.js
  // should decrypt large payload
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40f17dbe490004000155332de41a11adc072553544429ce7787707d1c316e027e7e5e334263376affb8aa17ad30075293f28dea8a20af3c5e7");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("ec925802ae430ca77fd3dd73cb2cc588");
  std::string s;
  key.Apply(s, phy, 0);
  ASSERT_EQ(s, "The quick brown fox jumps over the lazy dog.");
}


TEST(LoRa, Decrypt3)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_decrypt.js
  // bad key scrambles payload
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954378762B11FF0D");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("ec925802ae430ca77fd3dd73cb2cc580");
  std::string s;
  key.Apply(s, phy, 0);
  ASSERT_EQ(AtomIT::LoRa::LoRaToolbox::FormatHexadecimal(s, true), "5999FC3F");
}


TEST(LoRa, Decrypt4)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_decrypt.js
  // bad data lightly scrambles payload
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954478762B11FF0D");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("ec925802ae430ca77fd3dd73cb2cc588");
  std::string s;
  key.Apply(s, phy, 0);
  ASSERT_EQ(s, "tbst");
}

 

TEST(LoRa, Decrypt5)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_mic.js
  // should calculate & verify correct MIC
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE4900020001954378762B11FF0D");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("44024241ed4ce9a68c6a8bc055233fd3");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x2B11FF0D));
  ASSERT_EQ(phy.GetMIC(), key.ComputeMIC(phy, 0));
  ASSERT_TRUE(key.CheckMIC(phy, 0));
}


TEST(LoRa, Decrypt6)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_mic.js
  // should calculate & verify correct MIC
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE49000300012A3518AF");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("44024241ed4ce9a68c6a8bc055233fd3");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x2A3518AF));
  ASSERT_EQ(phy.GetMIC(), key.ComputeMIC(phy, 0));
  ASSERT_TRUE(key.CheckMIC(phy, 0));
}


TEST(LoRa, Decrypt7)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_mic.js
  // should detect incorrect MIC
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("40F17DBE49000300012A3518AA"); // aa not af
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("44024241ed4ce9a68c6a8bc055233fd3");
  ASSERT_EQ(phy.GetMIC(), be32toh(0x2A3518AA));
  ASSERT_NE(phy.GetMIC(), key.ComputeMIC(phy, 0));
  ASSERT_FALSE(key.CheckMIC(phy, 0));
}


TEST(LoRa, Decrypt8)
{
  // https://github.com/anthonykirby/lora-packet/blob/master/test/test_mic.js
  // should calculate & verify correct MIC for ACK
  AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal("60f17dbe4920020001f9d65d27");
  AtomIT::LoRa::FrameEncryptionKey key = AtomIT::LoRa::FrameEncryptionKey::ParseHexadecimal("44024241ed4ce9a68c6a8bc055233fd3");
  ASSERT_EQ(phy.GetMIC(), be32toh(0xf9d65d27));
  ASSERT_EQ(phy.GetMIC(), key.ComputeMIC(phy, 0));
  ASSERT_TRUE(key.CheckMIC(phy, 0));
}


TEST(AES, RFC4493)
{
  // Tests vectors from RFC4493
    
  AtomIT::LoRa::UnsignedInteger128 k, k1, k2, zero;
  k.Assign(AtomIT::LoRa::UnsignedInteger128::ParseHexadecimal("2b7e151628aed2a6abf7158809cf4f3c"));

  zero.AssignZero();
  ASSERT_EQ(AtomIT::LoRa::UnsignedInteger128::EncryptAES(k, zero).Format(true), "7DF76B0C1AB899B33E42F047B91B546F");

  k.GenerateCMACSubkey(k1, k2);
  ASSERT_EQ(k1.Format(true), "FBEED618357133667C85E08F7236A8DE");
  ASSERT_EQ(k2.Format(true), "F7DDAC306AE266CCF90BC11EE46D513B");

  // Length, 0
  ASSERT_EQ(k.ComputeCMAC("").Format(true), "BB1D6929E95937287FA37D129B756746");

  // Length, 16
  std::string tmp;
  AtomIT::LoRa::LoRaToolbox::ParseHexadecimal(tmp, "6bc1bee22e409f96e93d7e117393172a");
  ASSERT_EQ(k.ComputeCMAC(tmp).Format(true), "070A16B46B4D4144F79BDD9DD04A287C");

  // Length, 40
  AtomIT::LoRa::LoRaToolbox::ParseHexadecimal(tmp, "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411");
  ASSERT_EQ(k.ComputeCMAC(tmp).Format(true), "DFA66747DE9AE63030CA32611497C827");

  // Length, 64
  AtomIT::LoRa::LoRaToolbox::ParseHexadecimal(tmp, "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
  ASSERT_EQ(k.ComputeCMAC(tmp).Format(true), "51F0BEBF7E3B9D92FC49741779363CFE");
}


static void TestInvalidPacket(const std::string& packet)
{
  try
  {
    AtomIT::LoRa::PHYPayload phy = AtomIT::LoRa::PHYPayload::ParseHexadecimal(packet);
    AtomIT::LoRa::MACPayload mac(phy);
    std::string payload;
    mac.GetFramePayload(payload);
    mac.GetFHDR(payload);
  }
  catch (Orthanc::OrthancException&)
  {
  }
}


TEST(LoRa, Invalid1)
{
  TestInvalidPacket("F51852DD1AF7D359B24C1BADB082AA4D7827A15E8707C9F684BD");
}


TEST(LoRa, Invalid2)
{
  TestInvalidPacket("0F658D91A5B8349DFB8E243263B6572EC4C4A31ADCED66C6F189E4A1B8134A2D057184E8CF526D535A3D89986CB1996B87D9EF48AD4F585D544B561276B2F5E48DBE9F8B844297BAC4BBE792020AED89");
}


TEST(LoRa, Invalid3)
{
  TestInvalidPacket("5F0306E0745D1C275B6BC4B9AAACFE765225");
}


TEST(LoRa, Invalid4)
{
  TestInvalidPacket("8508900D17D3BE05614BE411E0F44B");
}
