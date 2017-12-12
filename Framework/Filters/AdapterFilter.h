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

#include "IFilter.h"
#include "../TimeSeries/TimeSeriesReader.h"
#include "../TimeSeries/TimeSeriesWriter.h"

#include <memory>

namespace AtomIT
{
  // Filter with 1 input and N >= 0 output(s)
  class AdapterFilter : public IFilter
  {
  private:
    std::string         name_;
    ITimeSeriesManager& manager_;
    std::string         timeSeries_;
    TimeSeriesReader    reader_;
    bool                replayHistory_;
    bool                isValid_;
    int64_t             timestamp_;

    std::auto_ptr<TimeSeriesWriter>  inputPopper_;

  protected:
    enum PushStatus
    {
      PushStatus_Success,
      PushStatus_Retry,
      PushStatus_Failure
    };
    
    virtual PushStatus Push(const Message& message) = 0;

  public:
    AdapterFilter(const std::string& name,
                  ITimeSeriesManager& manager,
                  const std::string& timeSeries);

    void SetReplayHistory(bool replay)
    {
      replayHistory_ = replay;
    }

    bool IsReplayHistory() const
    {
      return replayHistory_;
    }

    const std::string& GetInputTimeSeries() const
    {
      return timeSeries_;
    }

    void SetPopInput(bool pop);

    bool IsPopInput() const;
    
    virtual std::string GetName() const
    {
      return name_;
    }

    virtual void Start();
    
    virtual bool Step();
    
    virtual void Stop()
    {
    }
  };
}
