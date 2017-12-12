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

#include <string>
#include <stdint.h>

#include "../ConfigurationSection.h"

namespace AtomIT
{
  namespace MQTT
  {
    class Broker
    {
    private:
      std::string   server_;
      uint16_t      port_;
      std::string   username_;
      std::string   password_;

    public:
      Broker();

      const std::string& GetServer() const
      {
        return server_;
      }

      uint16_t GetPort() const
      {
        return port_;
      }

      bool HasCredentials() const;

      const std::string& GetUsername() const;

      const std::string& GetPassword() const;

      void SetServer(const std::string& server)
      {
        server_ = server;
      }

      void SetPort(uint16_t port);

      void ClearCredentials();
  
      void SetCredentials(const std::string& username,
                          const std::string& password);

      static Broker Parse(const ConfigurationSection& config);
    };
  }
}
