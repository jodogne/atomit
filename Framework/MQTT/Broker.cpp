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


#include "Broker.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace MQTT
  {
    Broker::Broker() :
      server_("127.0.0.1"),
      port_(1883)
    {
    }


    bool Broker::HasCredentials() const
    {
      return !username_.empty() && !password_.empty();
    }


    const std::string& Broker::GetUsername() const
    {
      if (HasCredentials())
      {
        return username_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    
    const std::string& Broker::GetPassword() const
    {
      if (HasCredentials())
      {
        return password_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }


    void Broker::SetPort(uint16_t port)
    {
      if (port <= 0 ||
          port >= 65535)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        port_ = port;
      }
    }


    void Broker::ClearCredentials()
    {
      username_.clear();
      password_.clear();
    }

    
    void Broker::SetCredentials(const std::string& username,
                                const std::string& password)
    {
      if (username.empty())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        username_ = username;
        password_ = password;
      }
    }


    Broker Broker::Parse(const ConfigurationSection& config)
    {
      Broker broker;

      std::string s, t;
      if (config.GetStringParameter(s, "Server"))
      {
        broker.SetServer(s);
      }

      if (config.GetStringParameter(s, "Username") &&
          config.GetStringParameter(t, "Password"))
      {
        broker.SetCredentials(s, t);
      }

      unsigned int port;
      if (config.GetUnsignedIntegerParameter(port, "Port"))
      {
        if (port <= 65535)
        {
          broker.SetPort(static_cast<uint16_t>(port));
        }
        else
        {
          LOG(ERROR) << "Not a valid TCP port number: " << port;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
      }

      return broker;
    }    
  }
}
