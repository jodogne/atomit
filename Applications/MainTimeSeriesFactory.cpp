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


#include "MainTimeSeriesFactory.h"

#include "../Framework/TimeSeries/SQLiteBackend/SQLiteTimeSeriesBackend.h"
#include "../Framework/TimeSeries/MemoryBackend/MemoryTimeSeriesBackend.h"

#include <Core/OrthancException.h>
#include <Core/Logging.h>

#include <boost/lexical_cast.hpp>

namespace AtomIT
{
  MainTimeSeriesFactory::TimeSeriesConfiguration::TimeSeriesConfiguration() :
    backend_(Backend_None),
    maxLength_(0),
    maxSize_(0),
    timestampType_(TimestampType_Default),
    sqlite_(NULL)
  {
  }

      
  MainTimeSeriesFactory::TimeSeriesConfiguration::
  TimeSeriesConfiguration(SQLiteDatabase& sqlite,
                          uint64_t maxLength,
                          uint64_t maxSize,
                          TimestampType timestampType) :
    backend_(Backend_SQLite),
    maxLength_(maxLength),
    maxSize_(maxSize),
    timestampType_(timestampType),
    sqlite_(&sqlite)
  {
  }


  MainTimeSeriesFactory::TimeSeriesConfiguration::
  TimeSeriesConfiguration(Backend type,
                          uint64_t maxLength,
                          uint64_t maxSize,
                          TimestampType timestampType) :
    backend_(type),
    maxLength_(maxLength),
    maxSize_(maxSize),
    timestampType_(timestampType),
    sqlite_(NULL)
  {
    if (type == Backend_SQLite)
    {
      // The other constructor should have been called
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  ITimeSeriesBackend* MainTimeSeriesFactory::TimeSeriesConfiguration::
  CreateTimeSeries(const std::string& name)
  {
    switch (backend_)
    {
      case Backend_None:
        return NULL;

      case Backend_SQLite:
        assert(sqlite_ != NULL);
        sqlite_->CreateTimeSeries(name, maxLength_, maxSize_);
        return new SQLiteTimeSeriesBackend(*sqlite_, name);

      case Backend_Memory:
        return new MemoryTimeSeriesBackend(maxLength_, maxSize_);

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  std::string MainTimeSeriesFactory::TimeSeriesConfiguration::Format() const
  {
    std::string s;
        
    switch (backend_)
    {
      case Backend_Memory:
        s = "Memory backend ";
        break;

      case Backend_SQLite:
        s = "SQLite backend ";
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    if (maxLength_ == 0)
    {
      s += "with unlimited length, ";
    }
    else
    {
      s += "with maximum " + boost::lexical_cast<std::string>(maxLength_) + " items, ";
    }

    if (maxSize_ == 0)
    {
      s += "unlimited size, and ";
    }
    else
    {
      s += "maximum " + boost::lexical_cast<std::string>(maxSize_) + " bytes, and ";
    }

    switch (timestampType_)
    {
      case TimestampType_Sequence:
        s += "sequential timestamps";
        break;
            
      case TimestampType_NanosecondsClock:
        s += "clock timestamps (ns)";
        break;

      case TimestampType_MillisecondsClock:
        s += "clock timestamps (ms)";
        break;

      case TimestampType_SecondsClock:
        s += "clock timestamps (s)";
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);            
    }

    return s;
  }


  MainTimeSeriesFactory::TimeSeriesConfiguration
  MainTimeSeriesFactory::TimeSeriesConfiguration::Parse(const ConfigurationSection& section,
                                                        MainTimeSeriesFactory& that)
  {
    TimeSeriesConfiguration config;
        
    std::string s;
    if (section.GetStringParameter(s, "Backend"))
    {
      if (s == "Memory")
      {
        config.backend_ = Backend_Memory;
      }
      else if (s == "SQLite")
      {
        config.backend_ = Backend_SQLite;
      }
      else
      {
        LOG(ERROR) << "Unsupported value for a time series backend: " << s;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      config.backend_ = Backend_Memory;
    }

    if (config.backend_ == Backend_SQLite)
    {
      if (section.GetStringParameter(s, "Path"))
      {
        config.sqlite_ = &that.GetSQLiteDatabase(s);
      }
      else
      {
        LOG(ERROR) << "The \"Path\" parameter must be provided for a SQLite backend";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }

    {
      unsigned int v;
      if (section.GetUnsignedIntegerParameter(v, "MaxLength"))
      {
        config.maxLength_ = v;
      }
      else
      {
        config.maxLength_ = 0;
      }
    }

    {
      unsigned int v;
      if (section.GetUnsignedIntegerParameter(v, "MaxSize"))
      {
        config.maxSize_ = v;
      }
      else
      {
        config.maxSize_ = 0;
      }
    }

    if (section.GetStringParameter(s, "Timestamp"))
    {
      if (s == "Sequence")
      {
        config.timestampType_ = TimestampType_Sequence;
      }
      else if (s == "NanosecondsClock")
      {
        config.timestampType_ = TimestampType_NanosecondsClock;
      }
      else if (s == "MillisecondsClock")
      {
        config.timestampType_ = TimestampType_MillisecondsClock;
      }
      else if (s == "SecondsClock")
      {
        config.timestampType_ = TimestampType_SecondsClock;
      }
      else
      {
        LOG(ERROR) << "Unsupported value for a timestamp type: " << s;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      config.timestampType_ = TimestampType_Sequence;
    }
 
    return config;
  }


  SQLiteDatabase& MainTimeSeriesFactory::GetSQLiteDatabase(const boost::filesystem::path& path)
  {
    SQLiteDatabases::iterator found = databases_.find(path);

    if (found != databases_.end())
    {
      assert(found->second != NULL);
      return *found->second;
    }
    else
    {
      SQLiteDatabase* db = new SQLiteDatabase(path.string());
      databases_[path] = db;
      return *db;
    }
  }


  void MainTimeSeriesFactory::SetAutoConfiguration(const TimeSeriesConfiguration& timeSeries)
  {
    LOG(WARNING) << "Enabling auto-creation of time series: " << timeSeries.Format();
      
    if (autoTimeSeries_.GetBackend() != Backend_None)
    {
      LOG(ERROR) << "Cannot set two different auto time series";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      autoTimeSeries_ = timeSeries;
    }
  }
    

  void MainTimeSeriesFactory::RegisterTimeSeries(const std::string& name,
                                                 const TimeSeriesConfiguration& timeSeries)
  {
    LOG(WARNING) << "Registering time series \"" << name << "\": " << timeSeries.Format();
      
    if (manualTimeSeries_.find(name) != manualTimeSeries_.end())
    {
      LOG(ERROR) << "Cannot add twice the time series \"" << name << "\"";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      manualTimeSeries_[name] = timeSeries;
    }
  }
    

  MainTimeSeriesFactory::~MainTimeSeriesFactory()
  {
    for (SQLiteDatabases::iterator
           it = databases_.begin(); it != databases_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }


  void MainTimeSeriesFactory::RegisterMemoryTimeSeries(const std::string& name,
                                                       uint64_t maxLength,
                                                       uint64_t maxSize,
                                                       TimestampType timestampType)
  {
    boost::mutex::scoped_lock lock(mutex_);
    TimeSeriesConfiguration config(Backend_Memory, maxLength, maxSize, timestampType);
    RegisterTimeSeries(name, config);
  }


  void MainTimeSeriesFactory::RegisterSQLiteTimeSeries(const std::string& name,
                                                       const boost::filesystem::path& path,
                                                       uint64_t maxLength,
                                                       uint64_t maxSize,
                                                       TimestampType timestampType)
  {
    boost::mutex::scoped_lock lock(mutex_);
    TimeSeriesConfiguration config(GetSQLiteDatabase(path), maxLength, maxSize, timestampType);
    RegisterTimeSeries(name, config);
  }


  void MainTimeSeriesFactory::SetAutoMemory(uint64_t maxLength,
                                            uint64_t maxSize,
                                            TimestampType timestampType)
  {
    boost::mutex::scoped_lock lock(mutex_);
    TimeSeriesConfiguration config(Backend_Memory, maxLength, maxSize, timestampType);
    SetAutoConfiguration(config);
  }


  void MainTimeSeriesFactory::SetAutoSQLite(const boost::filesystem::path& path,
                                            uint64_t maxLength,
                                            uint64_t maxSize,
                                            TimestampType timestampType)
  {
    boost::mutex::scoped_lock lock(mutex_);
    TimeSeriesConfiguration config(GetSQLiteDatabase(path), maxLength, maxSize, timestampType);
    SetAutoConfiguration(config);
  }


  ITimeSeriesBackend* MainTimeSeriesFactory::CreateManualTimeSeries(const std::string& name)
  {
    boost::mutex::scoped_lock lock(mutex_);

    ManualTimeSeries::iterator timeSeries = manualTimeSeries_.find(name);

    if (timeSeries == manualTimeSeries_.end())
    {
      return NULL;
    }
    else
    {
      return timeSeries->second.CreateTimeSeries(name);
    }
  }


  ITimeSeriesBackend* MainTimeSeriesFactory::
  CreateAutoTimeSeries(TimestampType& timestampType /* out */,
                       const std::string& name)
  {
    boost::mutex::scoped_lock lock(mutex_);

    timestampType = autoTimeSeries_.GetTimestampType();
    return autoTimeSeries_.CreateTimeSeries(name);
  }


  void MainTimeSeriesFactory::ListManualTimeSeries(std::map<std::string, TimestampType>& target)
  {
    boost::mutex::scoped_lock lock(mutex_);

    target.clear();

    for (ManualTimeSeries::iterator 
           it = manualTimeSeries_.begin(); it != manualTimeSeries_.end(); ++it)
    {
      target[it->first] = it->second.GetTimestampType();
    }      
  }


  void MainTimeSeriesFactory::LoadConfiguration(const ConfigurationSection& config)
  {
    boost::mutex::scoped_lock lock(mutex_);

    static const char* AUTO = "AutoTimeSeries";
    static const char* MANUAL = "TimeSeries";      
      
    if (config.HasItem(AUTO))
    {
      ConfigurationSection section(config, AUTO);

      LOG(INFO) << "Configuring auto-creation of time series with parameters: "
                << section.Format();
      
      SetAutoConfiguration(TimeSeriesConfiguration::Parse(section, *this));
    }

    if (config.HasItem(MANUAL))
    {
      ConfigurationSection manual(config, MANUAL);

      std::set<std::string> names;
      manual.ListMembers(names);

      for (std::set<std::string>::const_iterator
             name = names.begin(); name != names.end(); ++name)
      {
        ConfigurationSection section(manual, *name);

        LOG(INFO) << "Creating time series \"" << *name << "\" with parameters: "
                  << section.Format();
      
        RegisterTimeSeries(*name, TimeSeriesConfiguration::Parse(section, *this));
      }
    }
  }
}
