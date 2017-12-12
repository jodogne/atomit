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


#include "CSVFileSourceFilter.h"

#include <Core/OrthancException.h>
#include <Core/Logging.h>
#include <Core/Toolbox.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
 
namespace AtomIT
{
  static void Unescape(std::string& result,
                       const std::string& source)
  {
    if (!source.empty() &&
        source[0] == '\"' &&
        source[source.size() - 1] == '\"')
    {
      result = source.substr(1, source.size() - 2);
    }
    else
    {
      result = source;
    }
          
    boost::replace_all(result, "\"\"", "\"");
  }

  
  bool CSVFileSourceFilter::Decode(Message& message,
                                   const std::string& line) const
  {
    std::vector<std::string> columns;
    Orthanc::Toolbox::TokenizeString(columns, line, ',');

    if (columns.size() != 4)
    {
      LOG(ERROR) << "CSV files must have 4 columns: " << GetPath();
      return false;
    }

    std::string series, metadata, value, tmp;
    int64_t timestamp;

    Unescape(series, columns[0]);
    Unescape(tmp, columns[1]);
    Unescape(metadata, columns[2]);
    Unescape(value, columns[3]);
      
    try
    {
      timestamp = boost::lexical_cast<int64_t>(tmp);
    }
    catch (boost::bad_lexical_cast&)
    {
      LOG(ERROR) << "Cannot decode timestamp: \"" << tmp << "\"";
      return false;
    }

    message.SetTimestamp(timestamp);
    message.SwapMetadata(metadata);

    if (base64_)
    {
      try
      {
        Orthanc::Toolbox::DecodeBase64(tmp, value);
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "The content is not encoded as base64";
        return false;
      }
        
      message.SwapValue(tmp);
    }
    else
    {
      message.SwapValue(value);
    }

    return true;
  }

  
  SourceFilter::FetchStatus
  CSVFileSourceFilter::ReadMessage(Message& message,
                                   boost::filesystem::ifstream& stream)
  {
    std::string line;
      
    if (std::getline(stream, line))
    {
      if (Decode(message, Orthanc::Toolbox::StripSpaces(line)))
      {
        return FetchStatus_Success;
      }
      else
      {
        return FetchStatus_Invalid;
      }
    }
    else
    {
      return FetchStatus_Done;
    }
  }

    
  CSVFileSourceFilter::CSVFileSourceFilter(const std::string& name,
                                           ITimeSeriesManager& manager,
                                           const std::string& timeSeries,
                                           const boost::filesystem::path& path) :
    FileReaderFilter(name, manager, timeSeries, path),
    base64_(true)
  {
  }
}
