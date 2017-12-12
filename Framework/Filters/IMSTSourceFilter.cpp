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


#include "IMSTSourceFilter.h"

#if ATOMIT_ENABLE_IMST_GATEWAY == 1

#include <Core/OrthancException.h>
#include <Core/Logging.h>

extern "C"
{
#include <loragw_hal.h>
}

#include <boost/thread.hpp>

namespace AtomIT
{
  static void InitializeBoard()
  {
    struct lgw_conf_board_s boardconf;
    memset(&boardconf, 0, sizeof(boardconf));

    // Enable ONLY for *public* networks using the LoRa MAC protocol
    boardconf.lorawan_public = true;

    // Index of RF chain which provides clock to concentrator
    boardconf.clksrc = 1;  // radio_1 provides clock to concentrator
  
    if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS)
    {
      LOG(WARNING) << "Failed to configure IMST board";
    }
  }

    
  static void SetupRFChain(unsigned int index,
                           bool enable,
                           enum lgw_radio_type_e type,
                           uint32_t freq,
                           float rssi_offset,
                           bool tx_enable)
  {
    struct lgw_conf_rxrf_s rfconf;
    memset(&rfconf, 0, sizeof(rfconf));

    rfconf.enable = enable;

    if (enable)
    {
      if (type != LGW_RADIO_TYPE_SX1255 &&
          type != LGW_RADIO_TYPE_SX1257)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      rfconf.freq_hz = freq;
      rfconf.rssi_offset = rssi_offset;
      rfconf.tx_enable = tx_enable;
      rfconf.type = type;
    }

    if (lgw_rxrf_setconf(index, rfconf) != LGW_HAL_SUCCESS)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);    
    }
  }

    
  static void SetupMultiSF(unsigned int index,
                           bool enable,
                           uint8_t rf_chain,
                           int32_t freq_hz)
  {
    struct lgw_conf_rxif_s ifconf;
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.enable = enable;

    if (enable)
    {
      ifconf.rf_chain = rf_chain;
      ifconf.freq_hz = freq_hz;
    }

    if (lgw_rxif_setconf(index, ifconf) != LGW_HAL_SUCCESS)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);    
    }
  }


  static void SetLoRaChannel(bool enable,
                             uint8_t radio,
                             int32_t freq_hz,
                             uint32_t bandwidth,
                             unsigned int spread_factor)
  {
    struct lgw_conf_rxif_s ifconf;
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.enable = enable;

    if (enable)
    {
      ifconf.rf_chain = radio;
      ifconf.freq_hz = freq_hz;

      switch (bandwidth)
      {
        case 500000:
          ifconf.bandwidth = BW_500KHZ;
          break;

        case 250000:
          ifconf.bandwidth = BW_250KHZ;
          break;

        case 125000:
          ifconf.bandwidth = BW_125KHZ;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);    
      }

      switch (spread_factor)
      {
        case  7:
          ifconf.datarate = DR_LORA_SF7;
          break;

        case  8:
          ifconf.datarate = DR_LORA_SF8;
          break;

        case  9:
          ifconf.datarate = DR_LORA_SF9;
          break;

        case 10:
          ifconf.datarate = DR_LORA_SF10;
          break;

        case 11:
          ifconf.datarate = DR_LORA_SF11;
          break;

        case 12:
          ifconf.datarate = DR_LORA_SF12;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);    
      }
    }

    if (lgw_rxif_setconf(8, ifconf) != LGW_HAL_SUCCESS)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);    
    }  
  }


  static void SetupFSKChannel(bool enable,
                              uint8_t radio,
                              int32_t freq_hz,
                              uint32_t bandwidth,
                              uint32_t datarate)
  {
    struct lgw_conf_rxif_s ifconf;
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.enable = enable;

    if (enable)
    {
      ifconf.rf_chain = radio;
      ifconf.freq_hz = freq_hz;
      ifconf.datarate = datarate;

      if (bandwidth <= 7800)
      {
        ifconf.bandwidth = BW_7K8HZ;
      }
      else if (bandwidth <= 15600)
      {
        ifconf.bandwidth = BW_15K6HZ;
      }
      else if (bandwidth <= 31200)
      {
        ifconf.bandwidth = BW_31K2HZ;
      }
      else if (bandwidth <= 62500)
      {
        ifconf.bandwidth = BW_62K5HZ;
      }
      else if (bandwidth <= 125000)
      {
        ifconf.bandwidth = BW_125KHZ;
      }
      else if (bandwidth <= 250000)
      {
        ifconf.bandwidth = BW_250KHZ;
      }
      else if (bandwidth <= 500000)
      {
        ifconf.bandwidth = BW_500KHZ;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    if (lgw_rxif_setconf(9, ifconf) != LGW_HAL_SUCCESS)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);    
    }  
  }


  static void SetupBoard()
  {
    InitializeBoard();

    // Statically configure the SX1301 parameters, as specified in
    // lora_gateway-2017-09-13/util_pkt_logger/global_conf.json

    // Set configuration for RF chains
    SetupRFChain(0, true, LGW_RADIO_TYPE_SX1257, 867500000, -166.0, true);
    SetupRFChain(1, true, LGW_RADIO_TYPE_SX1257, 868500000, -166.0, false);

    // Set configuration for LoRa multi-SF channels (bandwidth cannot be set)
    SetupMultiSF(0, true, 1, -400000);  // Lora MAC channel, 125kHz, all SF, 868.1 MHz
    SetupMultiSF(1, true, 1, -200000);  // Lora MAC channel, 125kHz, all SF, 868.3 MHz
    SetupMultiSF(2, true, 1, 0);        // Lora MAC channel, 125kHz, all SF, 868.5 MHz
    SetupMultiSF(3, true, 0, -400000);  // Lora MAC channel, 125kHz, all SF, 867.1 MHz
    SetupMultiSF(4, true, 0, -200000);  // Lora MAC channel, 125kHz, all SF, 867.3 MHz
    SetupMultiSF(5, true, 0, 0);        // Lora MAC channel, 125kHz, all SF, 867.5 MHz
    SetupMultiSF(6, true, 0, 200000);   // Lora MAC channel, 125kHz, all SF, 867.7 MHz
    SetupMultiSF(7, true, 0, 400000);   // Lora MAC channel, 125kHz, all SF, 867.9 MHz

    // Lora MAC channel, 250kHz, SF7, 868.3 MHz 
    SetLoRaChannel(true, 1, -200000, 250000, 7);

    // Set configuration for FSK channel
    SetupFSKChannel(true, 1, 300000, 125000, 50000);
  
    int code = lgw_start();
    if (code == LGW_HAL_SUCCESS)
    {
      LOG(WARNING) << "IMST LoRa concentrator started, packets can now be received";
    }
    else
    {
      LOG(ERROR) << "Failed to start the IMST LoRa concentrator";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);    
    }
  }


  static void UpdateReferenceCounter(int offset)
  {
    static boost::mutex  mutex_;

    {
      boost::mutex::scoped_lock lock(mutex_);

      static int counter_ = 0;
      counter_ += offset;

      if (counter_ < 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else if (counter_ > 1)
      {
        LOG(ERROR) << "Cannot instantiate more than one IMST source filter";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }
  }
    
    
  SourceFilter::FetchStatus IMSTSourceFilter::Fetch(Message& message)
  {
    struct lgw_pkt_rx_s packet;
    int count = lgw_receive(1, &packet);

    assert(count == 0 || count == 1);
      
    if (count == 1)
    {
      LOG(INFO) << "Received one packet from IMST LoRa concentrator";

      std::string value;
      value.assign(reinterpret_cast<const char*>(packet.payload), packet.size);

      message.SetMetadata(metadata_);
      message.SwapValue(value);  
      return FetchStatus_Success;
    }
    else
    {
      // No packet received, wait a bit
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      return FetchStatus_Invalid;
    }
  }

  
  IMSTSourceFilter::IMSTSourceFilter(const std::string& name,
                                     ITimeSeriesManager& manager,
                                     const std::string& timeSeries) :
    SourceFilter(name, manager, timeSeries),
    metadata_("application/octet-stream")
  {
    UpdateReferenceCounter(1);
  }


  IMSTSourceFilter::~IMSTSourceFilter()
  {
    UpdateReferenceCounter(-1);
  }


  void IMSTSourceFilter::Start()
  {
    SetupBoard();
      
    SourceFilter::Start();
  }

  
  void IMSTSourceFilter::Stop()
  {
    SourceFilter::Stop();

    if (lgw_stop() == LGW_HAL_SUCCESS)
    {
      LOG(WARNING) << "IMST LoRa concentrator stopped successfully";
    }
    else
    {
      LOG(ERROR) << "Failed to cleanly stop the IMST LoRa concentrator";
    }
  }
}

#endif
