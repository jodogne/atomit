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


#include "FilterFactory.h"

#include "../Framework/Filters/CSVFileSinkFilter.h"
#include "../Framework/Filters/CSVFileSourceFilter.h"
#include "../Framework/Filters/CounterSourceFilter.h"
#include "../Framework/Filters/FileLinesSourceFilter.h"
#include "../Framework/Filters/HttpPostSinkFilter.h"
#include "../Framework/Filters/IMSTSourceFilter.h"
#include "../Framework/Filters/LoRaPacketFilter.h"
#include "../Framework/Filters/LuaFilter.h"
#include "../Framework/Filters/MQTTSinkFilter.h"
#include "../Framework/Filters/MQTTSourceFilter.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  static void SetCommonAdapterParameters(AdapterFilter& filter,
                                         const ConfigurationSection& config)
  {
    bool b;
    
    if (config.GetBooleanParameter(b, "ReplayHistory"))
    {
      filter.SetReplayHistory(b);
    }
    else
    {
      filter.SetReplayHistory(false);
    }

    if (config.GetBooleanParameter(b, "PopInput"))
    {
      filter.SetPopInput(b);
    }
    else
    {
      filter.SetPopInput(false);
    }
  }
  

  static void SetCommonFileSinkParameters(SharedFileSinkFilter& filter,
                                          const ConfigurationSection& config)
  {
    bool b;
    
    if (config.GetBooleanParameter(b, "Binary"))
    {
      filter.SetBinary(b);
    }

    if (config.GetBooleanParameter(b, "Append"))
    {
      filter.SetAppend(b);
    }
    
    SetCommonAdapterParameters(filter, config);
  }
  

  static IFilter* LoadCSVFileSourceFilter(const std::string& name,
                                          ITimeSeriesManager& manager,
                                          const ConfigurationSection& config)
  {
    std::auto_ptr<CSVFileSourceFilter> filter
      (new CSVFileSourceFilter(name, manager,
                               config.GetMandatoryStringParameter("Output"),
                               config.GetMandatoryStringParameter("Path")));

    unsigned int v;
    if (config.GetUnsignedIntegerParameter(v, "MaxPendingMessages"))
    {
      filter->SetMaxPendingMessages(v);
    }

    return filter.release();
  }


  static IFilter* LoadCSVFileSinkFilter(const std::string& name,
                                        ITimeSeriesManager& manager,
                                        FileWritersPool& writers,
                                        const ConfigurationSection& config)
  {
    std::auto_ptr<CSVFileSinkFilter> filter
      (new CSVFileSinkFilter(name, manager,
                             config.GetMandatoryStringParameter("Input"), writers,
                             config.GetMandatoryStringParameter("Path")));

    SetCommonFileSinkParameters(*filter, config);

    bool b;
    if (config.GetBooleanParameter(b, "Append"))
    {
      filter->SetAppend(b);
    }
    else
    {
      filter->SetAppend(true);
    }

    if (config.GetBooleanParameter(b, "Header"))
    {
      filter->SetHeaderAdded(b);
    }
    else
    {
      filter->SetHeaderAdded(false);
    }

    if (config.GetBooleanParameter(b, "Base64"))
    {
      filter->SetBase64Encoded(b);
    }
    else
    {
      filter->SetBase64Encoded(true);
    }

    return filter.release();
  }


  static IFilter* LoadFileLinesSourceFilter(const std::string& name,
                                            ITimeSeriesManager& manager,
                                            const ConfigurationSection& config)
  {
    std::auto_ptr<FileLinesSourceFilter> filter
      (new FileLinesSourceFilter(name, manager,
                                 config.GetMandatoryStringParameter("Output"),
                                 config.GetMandatoryStringParameter("Path")));

    unsigned int v;
    if (config.GetUnsignedIntegerParameter(v, "MaxPendingMessages"))
    {
      filter->SetMaxPendingMessages(v);
    }

    std::string s;
    if (config.GetStringParameter(s, "Metadata"))
    {
      filter->SetMetadata(s);
    }

    return filter.release();
  }


#if ATOMIT_ENABLE_IMST_GATEWAY == 1
  static IFilter* LoadIMSTSourceFilter(const std::string& name,
                                       ITimeSeriesManager& manager,
                                       const ConfigurationSection& config)
  {
    std::auto_ptr<IMSTSourceFilter> filter
      (new IMSTSourceFilter(name, manager,
                            config.GetMandatoryStringParameter("Output")));

    std::string s;
    if (config.GetStringParameter(s, "Metadata"))
    {
      filter->SetMetadata(s);
    }

    return filter.release();
  }
#endif


  static IFilter* LoadCounterSourceFilter(const std::string& name,
                                          ITimeSeriesManager& manager,
                                          const ConfigurationSection& config)
  {
    int start, stop;
    unsigned int delay, increment;

    if (!config.GetIntegerParameter(start, "Start"))
    {
      start = 0;
    }

    if (!config.GetIntegerParameter(stop, "Stop"))
    {
      stop = 100;
    }

    if (!config.GetUnsignedIntegerParameter(increment, "Increment"))
    {
      increment = 1;
    }

    if (!config.GetUnsignedIntegerParameter(delay, "Delay"))
    {
      delay = 100;
    }

    std::auto_ptr<CounterSourceFilter> filter
      (new CounterSourceFilter(name, manager,
                               config.GetMandatoryStringParameter("Output")));

    std::string s;
    if (config.GetStringParameter(s, "Metadata"))
    {
      filter->SetMetadata(s);
    }

    filter->SetRange(start, stop);
    filter->SetIncrement(increment);
    filter->SetDelay(delay);

    return filter.release();
  }


  static IFilter* LoadMQTTSourceFilter(const std::string& name,
                                       ITimeSeriesManager& manager,
                                       const ConfigurationSection& config)
  {
    static const char* BROKER = "Broker";
    static const char* TOPICS = "Topics";
    
    std::auto_ptr<MQTTSourceFilter> filter
      (new MQTTSourceFilter(name, manager,
                            config.GetMandatoryStringParameter("Output")));

    if (config.HasItem(BROKER))
    {
      filter->SetBroker(MQTT::Broker::Parse(ConfigurationSection(config, BROKER)));
    }
    
    if (config.HasItem(TOPICS))
    {
      size_t size = config.GetSize(TOPICS);

      for (size_t i = 0; i < size; i++)
      {
        filter->AddTopic(config.GetStringArrayItem(TOPICS, i));
      }
    }
    
    std::string s;
    if (config.GetStringParameter(s, "ClientID"))
    {
      filter->SetClientId(s);
    }

    return filter.release();
  }


  static IFilter* LoadMQTTSinkFilter(const std::string& name,
                                     ITimeSeriesManager& manager,
                                     const ConfigurationSection& config)
  {
    static const char* BROKER = "Broker";
    
    std::auto_ptr<MQTTSinkFilter> filter
      (new MQTTSinkFilter(name, manager,
                          config.GetMandatoryStringParameter("Input")));

    SetCommonAdapterParameters(*filter, config);

    if (config.HasItem(BROKER))
    {
      filter->SetBroker(MQTT::Broker::Parse(ConfigurationSection(config, BROKER)));
    }
        
    std::string s;
    if (config.GetStringParameter(s, "ClientID"))
    {
      filter->SetClientId(s);
    }

    return filter.release();
  }


  static IFilter* LoadLuaFilter(const std::string& name,
                                ITimeSeriesManager& manager,
                                const ConfigurationSection& config)
  {
    std::auto_ptr<LuaFilter> filter
      (new LuaFilter(name, manager,
                     config.GetMandatoryStringParameter("Input")));

    filter->ExecuteFile(config.GetMandatoryStringParameter("Path"));

    std::string s;
    if (config.GetStringParameter(s, "Output"))
    {
      filter->SetDefaultOutputTimeSeries(s);
    }

    SetCommonAdapterParameters(*filter, config);

    return filter.release();
  }


  static IFilter* LoadLoRaPacketFilter(const std::string& name,
                                       ITimeSeriesManager& manager,
                                       const ConfigurationSection& config)
  {
    std::auto_ptr<LoRaPacketFilter> filter
      (new LoRaPacketFilter(name, manager,
                            config.GetMandatoryStringParameter("Input"),
                            config.GetMandatoryStringParameter("Output"),
                            config.GetMandatoryStringParameter("nwkSKey"),
                            config.GetMandatoryStringParameter("appSKey")));

    SetCommonAdapterParameters(*filter, config);

    return filter.release();
  }


  static IFilter* LoadHttpPostSinkFilter(const std::string& name,
                                         ITimeSeriesManager& manager,
                                         const ConfigurationSection& config)
  {
    std::auto_ptr<HttpPostSinkFilter> filter
      (new HttpPostSinkFilter(name, manager,
                              config.GetMandatoryStringParameter("Input"),
                              config.GetMandatoryStringParameter("Url")));

    SetCommonAdapterParameters(*filter, config);

    unsigned int v;
    if (config.GetUnsignedIntegerParameter(v, "Timeout"))
    {
      filter->SetTimeout(v);
    }
    else
    {
      filter->SetTimeout(10);
    }

    std::string username, password;
    if (config.GetStringParameter(username, "Username") &&
        config.GetStringParameter(password, "Password"))
    {
      filter->SetCredentials(username.c_str(), password.c_str());
    }

    return filter.release();
  }


  IFilter* CreateFilter(ITimeSeriesManager& manager,
                        FileWritersPool& writers,
                        const ConfigurationSection& config)
  {
    LOG(INFO) << "Creating filter with parameters: " << config.Format();

    std::string name;
    if (!config.GetStringParameter(name, "Name"))
    {
      name = "(no name)";
    }
    
    std::string type;
    if (!config.GetStringParameter(type, "Type"))
    {
      LOG(ERROR) << "Configuration of filter \"" << name << "\" has no type";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    std::auto_ptr<IFilter> filter;

    if (type == "CSVSource")
    {
      filter.reset(LoadCSVFileSourceFilter(name, manager, config));
    }
    else if (type == "CSVSink")
    {
      filter.reset(LoadCSVFileSinkFilter(name, manager, writers, config));
    }
    else if (type == "FileLines")
    {
      filter.reset(LoadFileLinesSourceFilter(name, manager, config));
    }
#if ATOMIT_ENABLE_IMST_GATEWAY == 1
    else if (type == "IMST")
    {
      filter.reset(LoadIMSTSourceFilter(name, manager, config));
    }
#endif
    else if (type == "MQTTSource")
    {
      filter.reset(LoadMQTTSourceFilter(name, manager, config));
    }
    else if (type == "MQTTSink")
    {
      filter.reset(LoadMQTTSinkFilter(name, manager, config));
    }
    else if (type == "Counter")
    {
      filter.reset(LoadCounterSourceFilter(name, manager, config));
    }
    else if (type == "Lua")
    {
      filter.reset(LoadLuaFilter(name, manager, config));
    }
    else if (type == "LoRaDecoder")
    {
      filter.reset(LoadLoRaPacketFilter(name, manager, config));
    }
    else if (type == "HttpPost")
    {
      filter.reset(LoadHttpPostSinkFilter(name, manager, config));
    }
    else
    {
      LOG(ERROR) << "Unknown type for filter \"" << name << "\": " << type;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    if (filter.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    return filter.release();
  }
}
