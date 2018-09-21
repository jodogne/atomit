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


#include "AtomITToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <pugixml.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace AtomIT
{
  namespace Toolbox
  {
    static const boost::posix_time::ptime EPOCH(boost::gregorian::date(1970, 1, 1)); 

    
    int64_t GetNanosecondsClockTimestamp()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      return (now - EPOCH).total_nanoseconds();
    }
    

    int64_t GetMillisecondsClockTimestamp()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      return (now - EPOCH).total_milliseconds();
    }
    

    int64_t GetSecondsClockTimestamp()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      return (now - EPOCH).total_seconds();
    }


    static bool XmlToJsonInternal(Json::Value& json,
                                  const pugi::xml_node& node,
                                  bool simplify)
    {
      // https://davidwalsh.name/convert-xml-json

      if (node.type() == pugi::node_cdata ||
          node.type() == pugi::node_pcdata)
      {
        json = node.text().get();
        return true;
      }    
  
      if (node.type() != pugi::node_element)
      {
        return false;
      }

      json = Json::objectValue;
  
      for (pugi::xml_attribute_iterator it = node.attributes_begin();
           it != node.attributes_end(); ++it)
      {
        std::string name(it->name());
        name = "@" + name;

        if (json.isMember(name))
        {
          LOG(WARNING) << "Cannot add twice the XML attribute \"" << it->name() << "\"";
        }
    
        json[name] = it->value();
      }
  
      if (simplify &&
          json.empty())
      {
        bool isText = false;
        std::string text;

        for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
        {
          if (isText)
          {
            isText = false;
          }
    
          if (it->type() == pugi::node_cdata ||
              it->type() == pugi::node_pcdata)
          {
            isText = true;
            text = it->text().get();
          }    
        }

        if (isText)
        {
          json = text;
          return true;
        }
      }

      for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
      {
        std::string name(it->name());
    
        if (!name.empty() &&
            name[0] == '@')
        {
          LOG(WARNING) << "Cannot fully parse a XML file with a child named \"" << name << "\"";
          continue;
        }

        if (name.empty())
        {
          name = "@text";
        }

        Json::Value child;
        if (XmlToJsonInternal(child, *it, simplify))
        {
          if (!json.isMember(name))
          {
            if (simplify)
            {
              json[name] = child;
            }
            else
            {
              Json::Value v = Json::arrayValue;
              v.append(child);
              json[name] = v;          
            }
          }
          else if (json[name].type() == Json::arrayValue)
          {
            json[name].append(child);
          }
          else
          {
            // Convert the node content to a list
            Json::Value v = Json::arrayValue;
            v.append(json[name]);
            v.append(child);
            json[name] = v;
          }        
        }
      }

      return true;
    }


    bool XmlToJson(Json::Value& json,
                   const std::string& xml,
                   bool simplify)
    {
      pugi::xml_document doc;
      pugi::xml_parse_result result = doc.load_buffer(xml.c_str(), xml.size());

      if (result.status != pugi::status_ok)
      {
        return false;
      }
      else
      {
        const pugi::xml_node& root = doc.document_element();
        XmlToJsonInternal(json, root, simplify);
        return true;
      }
    }


    FileWriter::FileWriter(boost::filesystem::path path,
                           bool append,
                           bool binary)
    {
      std::ios_base::openmode mode = std::ios_base::out;

      if (append)
      {
        mode |= std::ios_base::app;
      }
      else
      {
        mode |= std::ios_base::trunc;
      }

      if (binary)
      {
        mode |= std::ios_base::binary;
      }

      stream_.open(path, mode);

      if (!stream_.is_open())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_CannotWriteFile);
      }

      isEmpty_ = (stream_.tellp() == std::streampos(0));
    }


    void FileWriter::Write(const std::string& buffer)
    {
      isEmpty_ = false;
      stream_.write(buffer.c_str(), buffer.size());
      stream_.flush();
    }
  }
}
