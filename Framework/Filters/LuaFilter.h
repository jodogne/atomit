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

#include "DemultiplexerFilter.h"

#include <Core/Lua/LuaContext.h>

#include <boost/filesystem/path.hpp>

namespace AtomIT
{
  class LuaFilter : public DemultiplexerFilter
  {
  private:
    class OutputParser;
    
    std::string          defaultTimeSeries_;
    Orthanc::LuaContext  lua_;

    static int ApplyStringConverter(lua_State *state,
                                    void (*func) (std::string&, const std::string&),
                                    const char* name);
    
    static int LuaDecodeBase64(lua_State *state);

    static int LuaEncodeBase64(lua_State *state);

    static int LuaParseHexadecimal(lua_State *state);

    static void FormatHexadecimalUpper(std::string& target,
                                       const std::string& source);
    
    static int LuaFormatHexadecimal(lua_State *state);

    static int ParseXml(lua_State *state);
    
  protected:
    virtual void Demux(ConvertedMessages& outputs,
                       const Message& source);
    
  public:
    LuaFilter(const std::string& name,
              ITimeSeriesManager& manager,
              const std::string& inputTimeSeries);

    void ExecuteFile(const boost::filesystem::path& path);

    void SetDefaultOutputTimeSeries(const std::string& timeSeries)
    {
      defaultTimeSeries_ = timeSeries;
    }
  };
}
