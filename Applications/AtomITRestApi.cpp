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


#include "AtomITRestApi.h"

#include "../Framework/TimeSeries/TimeSeriesReader.h"
#include "../Framework/TimeSeries/TimeSeriesWriter.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/math/special_functions/round.hpp>


namespace AtomIT
{
  ITimeSeriesManager& AtomITRestApi::GetManager(Orthanc::RestApiCall& call)
  {
    return dynamic_cast<AtomITRestApi&>(call.GetContext()).serverContext_.GetManager();
  }
    

  void AtomITRestApi::ServeRoot(Orthanc::RestApiGetCall& call)
  {
    call.GetOutput().Redirect("app/explorer.html");
  }

  
  void AtomITRestApi::ListTimeSeries(Orthanc::RestApiGetCall& call)
  {
    std::set<std::string> series;
    GetManager(call).ListTimeSeries(series);

    Json::Value result = Json::arrayValue;

    for (std::set<std::string>::const_iterator it = series.begin();
         it != series.end(); ++it)
    {
      result.append(*it);
    }

    call.GetOutput().AnswerJson(result);
  }
  

  void AtomITRestApi::GetTimeSeriesContent(Orthanc::RestApiGetCall& call)
  {
    std::string name = call.GetUriComponent("name", "");

    Json::Value result = Json::objectValue;
    result["content"] = Json::arrayValue;
    Json::Value& content = result["content"];

    unsigned int limit = boost::lexical_cast<unsigned int>(call.GetArgument("limit", "10"));
    bool done = false;

    {
      TimeSeriesReader reader(GetManager(call), name, false);
      TimeSeriesReader::Transaction transaction(reader);

      if (call.HasArgument("since"))
      {
        done = !transaction.SeekNearest(boost::lexical_cast<uint64_t>(call.GetArgument("since", "")));
      }
      else if (call.HasArgument("last"))
      {
        done = !transaction.SeekLast();
      }
      else
      {
        done = !transaction.SeekFirst();
      }

      while (!done &&
             (limit == 0 || content.size() < limit))
      {
        int64_t timestamp;
        std::string metadata, data;
          
        if (transaction.GetTimestamp(timestamp) &&
            transaction.Read(metadata, data))
        {
          Message message;
          message.SetTimestamp(timestamp);
          message.SwapMetadata(metadata);
          message.SwapValue(data);

          Json::Value json;
          message.Format(json);
          content.append(json);
        }

        if (!transaction.SeekNext())
        {
          done = true;
        }
      }
    }

    result["name"] = name;
    result["done"] = done;

    call.GetOutput().AnswerJson(result);
  }

  
  void AtomITRestApi::GetRawValue(Orthanc::RestApiGetCall& call)
  {
    std::string name = call.GetUriComponent("name", "");
    int64_t timestamp = boost::lexical_cast<uint64_t>(call.GetUriComponent("timestamp", ""));

    {
      TimeSeriesReader reader(GetManager(call), name, false);
      TimeSeriesReader::Transaction transaction(reader);

      transaction.Seek(timestamp);
        
      std::string metadata, data;
      if (transaction.Read(metadata, data))
      {
        static const boost::regex mimePattern("^([a-zA-Z0-9.\\-])+/([a-zA-Z0-9.\\-])+$");

        // Try and parse the metadata as a MIME type
        boost::cmatch what;
        if (regex_match(metadata.c_str(), what, mimePattern))
        {
          call.GetOutput().AnswerBuffer(data, metadata);
        }
        else
        {
          // Metadata is not formatted as a MIME type
          call.GetOutput().AnswerBuffer(data, "application/octet-stream");
        }
      }
    }
  }

  
  void AtomITRestApi::DeleteTimestamp(Orthanc::RestApiDeleteCall& call)
  {
    std::string name = call.GetUriComponent("name", "");
    int64_t timestamp = boost::lexical_cast<uint64_t>(call.GetUriComponent("timestamp", ""));

    LOG(INFO) << "Deleting timestamp " << timestamp << " in time series \"" << name << "\"";
      
    {
      TimeSeriesWriter writer(GetManager(call), name);
      TimeSeriesWriter::Transaction transaction(writer);
      transaction.DeleteRange(timestamp, timestamp + 1);
    }

    call.GetOutput().AnswerBuffer("{}", "application/json");
  }

  
  void AtomITRestApi::DeleteContent(Orthanc::RestApiDeleteCall& call)
  {
    std::string name = call.GetUriComponent("name", "");
    LOG(INFO) << "Deleting whole content of time series \"" << name << "\"";
      
    {
      TimeSeriesWriter writer(GetManager(call), name);
      TimeSeriesWriter::Transaction transaction(writer);
      transaction.ClearContent();
    }

    call.GetOutput().AnswerBuffer("{}", "application/json");
  }
  

  template <typename Call>
  void AtomITRestApi::AppendMessage(Call& call)
  {
    std::string name = call.GetUriComponent("name", "");

    Message message;
    message.SetMetadata(call.GetHttpHeader("content-type", "application/octet-stream"));

    std::string timestamp = call.GetUriComponent("timestamp", "");
    if (!timestamp.empty())
    {
      message.SetTimestamp(boost::lexical_cast<uint64_t>(timestamp));
    }

    std::string value;
    call.BodyToString(value);
    message.SwapValue(value);

    LOG(INFO) << "Message appended through REST API to time series \"" << name << "\": \""
              << message.FormatValue() << "\"";

    {
      TimeSeriesWriter writer(GetManager(call), name);
      writer.Append(message);
    }

    call.GetOutput().AnswerBuffer("{}", "application/json");
  }
  

  void AtomITRestApi::GetTimeSeriesStatistics(Orthanc::RestApiGetCall& call)
  {
    std::string name = call.GetUriComponent("name", "");
    uint64_t length, size;
      
    {
      TimeSeriesReader reader(GetManager(call), name, false);
      TimeSeriesReader::Transaction transaction(reader);
      transaction.GetStatistics(length, size);
    }

    Json::Value result = Json::objectValue;
    result["name"] = name;
    result["length"] = static_cast<unsigned int>(length);
    result["sizeMB"] = static_cast<unsigned int>
      (boost::math::round(size / static_cast<uint64_t>(1024 * 1024)));
    result["size"] = boost::lexical_cast<std::string>(size);

    call.GetOutput().AnswerJson(result);
  }


  AtomITRestApi::AtomITRestApi(ServerContext& serverContext) :
    serverContext_(serverContext)
  {
    Register("/", ServeRoot);
    Register("/series", ListTimeSeries);
    Register("/series/{name}", AutoListChildren);
    Register("/series/{name}", AppendMessage<Orthanc::RestApiPostCall>);
    Register("/series/{name}/content", GetTimeSeriesContent);
    Register("/series/{name}/content", DeleteContent);
    Register("/series/{name}/content/{timestamp}", GetRawValue);
    Register("/series/{name}/content/{timestamp}", DeleteTimestamp);
    Register("/series/{name}/content/{timestamp}", AppendMessage<Orthanc::RestApiPutCall>);
    Register("/series/{name}/statistics", GetTimeSeriesStatistics);
  }
}
