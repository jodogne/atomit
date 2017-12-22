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

#include "SourceFilter.h"

#include <boost/date_time.hpp>

namespace AtomIT
{
  class CounterSourceFilter : public SourceFilter
  {
  private:
    std::string  metadata_;
    int64_t      counter_;
    int64_t      stop_;
    unsigned int increment_;

    boost::posix_time::time_duration  delay_;

  protected:
    virtual FetchStatus Fetch(Message& message);

  public:
    CounterSourceFilter(const std::string& name,
                        ITimeSeriesManager& manager,
                        const std::string& timeSeries);

    void SetMetadata(const std::string& metadata)
    {
      metadata_ = metadata;
    }

    const std::string& GetMetadata() const
    {
      return metadata_;
    }

    void SetRange(int64_t start, 
                  int64_t stop);

    void SetIncrement(unsigned int increment);

    void SetDelay(unsigned int milliseconds);
  };
}
