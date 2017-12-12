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


#include "FileReaderFilter.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  SourceFilter::FetchStatus FileReaderFilter::Fetch(Message& message)
  {
    FetchStatus status = ReadMessage(message, stream_);
    line_ += 1;

    if (status == FetchStatus_Done)
    {
      LOG(WARNING) << "Filter \"" << GetName()
                   << "\" has finished reading all lines from file: " << path_;
    }
    else if (status == FetchStatus_Invalid)
    {
      LOG(ERROR) << "Cannot decode message at line " << line_ << " of file: " << path_;
    }

    return status;
  }


  FileReaderFilter::FileReaderFilter(const std::string& name,
                                     ITimeSeriesManager& manager,
                                     const std::string& timeSeries,
                                     const boost::filesystem::path& path) :
    SourceFilter(name, manager, timeSeries),
    path_(path),
    line_(0)
  {
  }


  void FileReaderFilter::Start()
  {
    stream_.open(path_);

    if (!stream_.is_open())
    {
      LOG(ERROR) << "Filter " << GetName() << " cannot open file: " << path_;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentFile);
    }

    SourceFilter::Start();
  }


  void FileReaderFilter::Stop()
  {
    SourceFilter::Stop();
    stream_.close();
  }
}
