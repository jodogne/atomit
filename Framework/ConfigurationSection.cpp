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


#include "ConfigurationSection.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/SystemToolbox.h>

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <json/reader.h>
#include <json/value.h>


namespace AtomIT
{
  ConfigurationSection::ConfigurationSection(const Json::Value& value) :
    configuration_(value)
  {
    if (configuration_.type() != Json::objectValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
    }
  }      

    
  ConfigurationSection::ConfigurationSection(const ConfigurationSection& parent,
                                             const std::string& section)
  {
    if (parent.HasItem(section))
    {
      configuration_ = parent.configuration_[section];

      if (configuration_.type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
      }
    }
    else
    {
      configuration_ = Json::objectValue;
    }
  }


  ConfigurationSection::ConfigurationSection(const ConfigurationSection& parent,
                                             const std::string& section,
                                             size_t index)
  {
    if (parent.HasItem(section))
    {
      configuration_ = parent.configuration_[section];

      if (configuration_.type() == Json::arrayValue)
      {
        configuration_ = configuration_[static_cast<Json::Value::ArrayIndex>(index)];
        if (configuration_.type() != Json::objectValue)
        {
          LOG(ERROR) << "Item " << index << " of section \"" << section
                     << "\" of the configuration file should be an object";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }
      else
      {
        LOG(ERROR) << "Section \"" << section << "\" of configuration file should contain an array";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      LOG(ERROR) << "Unknown section in configuration file: \"" << section << "\"";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  size_t ConfigurationSection::GetSize(const std::string& section) const
  {
    if (HasItem(section))
    {
      if (configuration_[section].type() == Json::arrayValue)
      {
        return configuration_[section].size();
      }
      else
      {
        LOG(ERROR) << "Section \"" << section << "\" of configuration file should contain an array";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      LOG(ERROR) << "Unknown section in configuration file: \"" << section << "\"";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  void ConfigurationSection::ListMembers(std::set<std::string>& target) const
  {
    assert(configuration_.type() == Json::objectValue);

    target.clear();

    Json::Value::Members v = configuration_.getMemberNames();

    for (size_t i = 0; i < v.size(); i++)
    {
      target.insert(v[i]);
    }
  }

    
  void ConfigurationSection::LoadFile(const boost::filesystem::path& path)
  {
    LOG(WARNING) << "Loading configuration from: " << path;
      
    std::string content;
    Orthanc::SystemToolbox::ReadFile(content, path.string());

    Json::Reader reader;
    if (!reader.parse(content, configuration_))
    {
      LOG(ERROR) << "Cannot parse the configuration file (invalid JSON)";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadJson);
    }
  }


  bool ConfigurationSection::GetStringParameter(std::string& value,
                                                const std::string& name) const
  {
    if (configuration_.isMember(name))
    {
      if (configuration_[name].type() == Json::stringValue)
      {
        value = configuration_[name].asString();
        return true;
      }
      else
      {
        LOG(ERROR) << "Parameter \"" << name << "\" should be a string value";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      return false;
    }
  }


  bool ConfigurationSection::GetIntegerParameter(int& value,
                                                 const std::string& name) const
  {
    if (configuration_.isMember(name))
    {
      bool ok = false;
        
      if (configuration_[name].type() == Json::intValue)
      {
        value = configuration_[name].asInt();
        ok = true;
      }
      else if (configuration_[name].type() == Json::uintValue)
      {
        value = configuration_[name].asUInt();
        ok = true;
      }
      else if (configuration_[name].type() == Json::stringValue)
      {
        try
        {
          value = boost::lexical_cast<int>(configuration_[name].asString());
          ok = true;
        }
        catch (boost::bad_lexical_cast&)
        {
        }
      }

      if (ok)
      {
        return true;
      }
      else
      {
        LOG(ERROR) << "Parameter \"" << name << "\" should be a integer value";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      return false;
    }
  }
    

  bool ConfigurationSection::GetUnsignedIntegerParameter(unsigned int& value,
                                                         const std::string& name) const
  {
    int tmp;
      
    if (GetIntegerParameter(tmp, name))
    {
      if (tmp < 0)
      {
        LOG(ERROR) << "Parameter \"" << name << "\" should be an unsigned integer value";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);     
      }
      else
      {
        value = static_cast<unsigned int>(tmp);
        return true;
      }
    }
    else
    {
      return false;
    }
  }


  bool ConfigurationSection::GetBooleanParameter(bool& value,
                                                 const std::string& name) const
  {
    if (configuration_.isMember(name))
    {
      bool ok = false;
        
      if (configuration_[name].type() == Json::intValue)
      {
        int i = configuration_[name].asInt();

        if (i == 0)
        {
          value = false;
          ok = true;
        }
        else if (i == 1)
        {
          value = true;
          ok = true;
        }
      }
      else if (configuration_[name].type() == Json::uintValue)
      {
        unsigned int i = configuration_[name].asUInt();

        if (i == 0)
        {
          value = false;
          ok = true;
        }
        else if (i == 1)
        {
          value = true;
          ok = true;
        }
      }
      else if (configuration_[name].type() == Json::booleanValue)
      {
        value = configuration_[name].asBool();
        ok = true;
      }

      if (ok)
      {
        return true;
      }
      else
      {
        LOG(ERROR) << "Parameter \"" << name << "\" should be a integer value";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      return false;
    }
  }
    

  std::string ConfigurationSection::GetMandatoryStringParameter(const std::string& name) const
  {
    std::string s;
      
    if (GetStringParameter(s, name))
    {
      return s;
    }
    else
    {
      LOG(ERROR) << "Mandatory string parameter \"" << name << "\" is missing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }
    

  int ConfigurationSection::GetMandatoryIntegerParameter(const std::string& name) const
  {
    int v;
      
    if (GetIntegerParameter(v, name))
    {
      return v;
    }
    else
    {
      LOG(ERROR) << "Mandatory integer parameter \"" << name << "\" is missing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }
    

  unsigned int ConfigurationSection::
  GetMandatoryUnsignedIntegerParameter(const std::string& name) const
  {
    unsigned int v;
      
    if (GetUnsignedIntegerParameter(v, name))
    {
      return v;
    }
    else
    {
      LOG(ERROR) << "Mandatory unsigned integer parameter \"" << name << "\" is missing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }    


  bool ConfigurationSection::GetMandatoryBooleanParameter(const std::string& name) const
  {
    bool v;
      
    if (GetBooleanParameter(v, name))
    {
      return v;
    }
    else
    {
      LOG(ERROR) << "Mandatory Boolean parameter \"" << name << "\" is missing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }    


  std::string ConfigurationSection::GetStringArrayItem(const std::string& section,
                                                       size_t index) const
  {
    if (HasItem(section))
    {
      const Json::Value& a = configuration_[section];

      if (a.type() == Json::arrayValue)
      {
        const Json::Value& b = a[static_cast<Json::Value::ArrayIndex>(index)];

        if (b.type() == Json::stringValue)
        {
          return b.asString();
        }
        else
        {
          LOG(ERROR) << "Item " << index << " of section \"" << section
                     << "\" of the configuration file should be a string";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }
      else
      {
        LOG(ERROR) << "Section \"" << section << "\" of configuration file should contain an array";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      LOG(ERROR) << "Unknown section in configuration file: \"" << section << "\"";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }
}
