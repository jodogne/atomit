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


#include "LuaFilter.h"

#include "../LoRa/LoRaToolbox.h"
#include "../AtomITToolbox.h"

#include <Core/Logging.h>
#include <Core/Lua/LuaFunctionCall.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Core/SystemToolbox.h>

static const char* CONVERT_CALLBACK = "Convert";


namespace AtomIT
{
  class LuaFilter::OutputParser : public Orthanc::LuaFunctionCall
  {
  private:
    class ITableVisitor : public boost::noncopyable
    {
    public:
      virtual ~ITableVisitor()
      {
      }

      virtual bool Apply(lua_State* lua) = 0;

      static bool Apply(lua_State* lua,
                        ITableVisitor& visitor,
                        int top)
      {
        if (!lua_istable(lua, top))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        lua_pushvalue(lua, top);
        lua_pushnil(lua);

        bool ok = true;
      
        while (lua_next(lua, -2))
        {
          lua_pushvalue(lua, -2);

          if (!visitor.Apply(lua))
          {
            ok = false;
          }

          lua_pop(lua, 2);
        }

        lua_pop(lua, 1);
 
        return ok;
      }
    };
    
    
    
    class MessageVisitor : public ITableVisitor
    {
    private:
      Message      message_;
      std::string  timeSeries_;

    public:
      MessageVisitor(const Message& original,
                     const std::string& defaultTimeSeries) :
        message_(original),
        timeSeries_(defaultTimeSeries)
      {
      }

      bool Apply(lua_State* lua)
      {
        if (lua_isstring(lua, -1))
        {
          std::string key(lua_tostring(lua, -1));

          if (key == "timestamp")
          {
            if (!lua_isstring(lua, -2))
            {
              return false;
            }
            else
            {
              message_.SetTimestamp(static_cast<int64_t>(lua_tonumber(lua, -2)));
              return true;
            }
          }
          else if (key == "metadata" ||
                   key == "value" ||
                   key == "series")
          {
            if (!lua_isstring(lua, -2))
            {
              return false;
            }
            
            size_t len;
            const char *s = lua_tolstring(lua, -2, &len);

            std::string value(s, len);

            if (key == "metadata")
            {
              message_.SwapMetadata(value);
              return true;
            }
            else if (key == "value")
            {
              message_.SwapValue(value);
              return true;
            }
            else if (key == "series")
            {
              timeSeries_ = value;
              return true;
            }
            else
            {
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
          }
          else
          {
            return false;
          }
        }
        else
        {
          return false;
        }
      }
      
      const Message& GetMessage() const
      {
        return message_;
      }

      const std::string& GetTimeSeries() const
      {
        if (timeSeries_.empty())
        {
          LOG(ERROR) << "No default \"Output\" time series was configured for the Lua filter";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          return timeSeries_;
        }
      }
    };

    
    class ArrayVisitor : public ITableVisitor
    {
    private:
      DemultiplexerFilter::ConvertedMessages&  output_;
      Message      original_;
      std::string  defaultTimeSeries_;

    public:
      ArrayVisitor(DemultiplexerFilter::ConvertedMessages& output,
                   const Message& original,
                   const std::string& defaultTimeSeries) :
        output_(output),
        original_(original),
        defaultTimeSeries_(defaultTimeSeries)
      {
      }

      bool Apply(lua_State* lua)
      {
        if (lua_isstring(lua, -1) &&
            lua_istable(lua, -2))
        {
          MessageVisitor message(original_, defaultTimeSeries_);

          if (ITableVisitor::Apply(lua, message, -2))
          {
            output_[message.GetTimeSeries()] = message.GetMessage();
            return true;
          }
        }
        
        return false;
      }
    };


    bool                                    success_;
    DemultiplexerFilter::ConvertedMessages  outputs_;
    
  public:
    OutputParser(Orthanc::LuaContext& context,
                 const Message& original,
                 const std::string& defaultTimeSeries) :
      LuaFunctionCall(context, CONVERT_CALLBACK),
      success_(false)
    {
      PushInteger(original.GetTimestamp());
      PushString(original.GetMetadata());
      PushString(original.GetValue());
      ExecuteInternal(1);

      if (lua_isnil(GetState(), 1))
      {
        // The message must be discarded
        LOG(INFO) << "The Lua filter has skipped one input message";
        success_ = true;
      }
      else
      {
        MessageVisitor visitor(original, defaultTimeSeries);
        if (ITableVisitor::Apply(GetState(), visitor, 1))
        {
          outputs_[visitor.GetTimeSeries()] = visitor.GetMessage();
          success_ = true;
        }
        else
        {
          ArrayVisitor visitor(outputs_, original, defaultTimeSeries);
          success_ = ITableVisitor::Apply(GetState(), visitor, 1);
        }
      }
    }

    bool IsSuccess() const
    {
      return success_;
    }

    const DemultiplexerFilter::ConvertedMessages& GetOutputs() const
    {
      return outputs_;
    }
  };
  
  
  int LuaFilter::ApplyStringConverter(lua_State *state,
                                      void (*func) (std::string&, const std::string&),
                                      const char* name)
  {
    // Check the types of the arguments
    int nArgs = lua_gettop(state);
    if ((nArgs < 1 || nArgs > 2) ||
        !lua_isstring(state, 1))
    {
      LOG(ERROR) << "Lua: Bad parameters for " << name;
      lua_pushnil(state);
    }
    else
    {
      size_t len;
      const char* tmp = lua_tolstring(state, 1, &len);

      std::string source;
      source.assign(tmp, len);

      bool ok = false;
      std::string converted;

      try
      {
        func(converted, source);
        ok = true;
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "Lua: Cannot do " << name;
      }

      if (ok)
      {
        lua_pushlstring(state, converted.c_str(), converted.size());
      }
      else
      {
        lua_pushnil(state);
      }
    }

    return 1;
  }

  
  int LuaFilter::LuaDecodeBase64(lua_State *state)
  {
    return ApplyStringConverter(state, Orthanc::Toolbox::DecodeBase64, "DecodeBase64()");
  }

  
  int LuaFilter::LuaEncodeBase64(lua_State *state)
  {
    return ApplyStringConverter(state, Orthanc::Toolbox::EncodeBase64, "EncodeBase64()");
  }

  
  int LuaFilter::LuaParseHexadecimal(lua_State *state)
  {
    return ApplyStringConverter(state, LoRa::LoRaToolbox::ParseHexadecimal, "ParseHexadecimal()");
  }

  
  void LuaFilter::FormatHexadecimalUpper(std::string& target,
                                         const std::string& source)
  {
    target = LoRa::LoRaToolbox::FormatHexadecimal(source, true);
  }

  
  int LuaFilter::LuaFormatHexadecimal(lua_State *state)
  {
    return ApplyStringConverter(state, FormatHexadecimalUpper, "FormatHexadecimal()");
  }


  int LuaFilter::ParseXml(lua_State *state)
  {
    // Check the types of the arguments
    int nArgs = lua_gettop(state);
    if ((nArgs < 1 || nArgs > 3) ||
        !lua_isstring(state, 1) || 
        (nArgs == 3 && !lua_isboolean(state, 2)))
    {
      LOG(ERROR) << "Lua: Bad parameters for XML to JSON conversion";
      lua_pushnil(state);
    }
    else
    {
      const char* xml = lua_tostring(state, 1);

      bool simplify = true;
      if (nArgs == 2)
      {
        simplify = lua_toboolean(state, 2) ? true : false;
      }

      Json::Value json;
      bool ok = false;
      try
      {
        Toolbox::XmlToJson(json, xml, simplify);
        ok = true;
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "Lua: Cannot do string conversion";
      }

      if (ok)
      {
        Orthanc::LuaContext::GetLuaContext(state).PushJson(json);
      }
      else
      {
        lua_pushnil(state);
      }
    }

    return 1;
  }


  void LuaFilter::Demux(ConvertedMessages& outputs,
                        const Message& source)
  {
    std::auto_ptr<OutputParser> parser;
      
    try
    {
      if (lua_.IsExistingFunction(CONVERT_CALLBACK))
      {
        parser.reset(new OutputParser(lua_, source, defaultTimeSeries_));
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Error in Lua function: " << e.What();
      parser.reset(NULL);
    }

    if (parser.get() != NULL &&
        parser->IsSuccess())
    {
      outputs = parser->GetOutputs();
    }
  }

 
  LuaFilter::LuaFilter(const std::string& name,
                       ITimeSeriesManager& manager,
                       const std::string& inputTimeSeries) :
    DemultiplexerFilter(name, manager, inputTimeSeries)
  {
    lua_.RegisterFunction("DecodeBase64", LuaDecodeBase64);
    lua_.RegisterFunction("EncodeBase64", LuaEncodeBase64);
    lua_.RegisterFunction("FormatHexadecimal", LuaFormatHexadecimal);
    lua_.RegisterFunction("ParseHexadecimal", LuaParseHexadecimal);
    lua_.RegisterFunction("ParseXml", ParseXml);
  }

  
  void LuaFilter::ExecuteFile(const boost::filesystem::path& path)
  {
    std::string f;
    Orthanc::SystemToolbox::ReadFile(f, path.string());
    lua_.Execute(f);
  }
}
