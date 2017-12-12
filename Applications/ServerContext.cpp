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


#include "ServerContext.h"

#include <Core/OrthancException.h>
#include <Core/Logging.h>

namespace AtomIT
{
  bool ServerContext::StartFilter(IFilter& filter)
  {
    try
    {
      filter.Start();
      LOG(INFO) << "Filter " << filter.GetName() << " has started";
      return true;
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception while starting filter " << filter.GetName() << ": " << e.What();
    }
    catch (...)
    {
      LOG(ERROR) << "Native exception while starting filter " << filter.GetName();
    }

    return false;
  }
    

  bool ServerContext::StopFilter(IFilter& filter)
  {
    try
    {
      filter.Stop();
      LOG(INFO) << "Filter " << filter.GetName() << " has stopped";
      return true;
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception while stopping filter " << filter.GetName() << ": " << e.What();
    }
    catch (...)
    {
      LOG(ERROR) << "Native exception while stopping filter " << filter.GetName();
    }      

    return false;
  }
    

  void ServerContext::WorkerThread(bool* continue_,
                                   IFilter* filter)
  {
    while (*continue_)
    {
      try
      {
        if (!filter->Step())
        {
          break;  // The filter has finished its task
        }
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(INFO) << "Exception in filter " << filter->GetName() << ": " << e.What();
      }
      catch (...)
      {
        LOG(INFO) << "Native exception in filter " << filter->GetName();
      }
    }
  }

    
  bool ServerContext::StopInternal()
  {
    if (state_ == State_Done)
    {
      return true;
    }
    else if (state_ == State_Setup)
    {
      return false;
    }
    else
    {
      LOG(WARNING) << "Stopping the filters";
      continue_ = false;

      for (size_t i = 0; i < threads_.size(); i++)
      {
        if (threads_[i] &&
            threads_[i]->joinable())
        {
          threads_[i]->join();
        }

        delete threads_[i];
        threads_[i] = NULL;
      }

      bool success = true;
        
      for (Filters::iterator it = filters_.begin(); it != filters_.end(); ++it)
      {
        assert(*it != NULL);
        if (!StopFilter(**it))
        {
          success = false;
        }
      }

      if (!success)
      {
        LOG(WARNING) << "Some of the filter(s) didn't stop properly";
      }

      state_ = State_Done;
      return true;
    }
  }


  ServerContext::ServerContext(ITimeSeriesManager& manager) :
    continue_(true),
    state_(State_Setup),
    manager_(manager)
  {
  }


  ServerContext::~ServerContext()
  {
    StopInternal();
      
    for (Filters::iterator it = filters_.begin(); it != filters_.end(); ++it)
    {
      delete *it;
    }        
  }


  void ServerContext::AddFilter(IFilter* filter)
  {
    boost::mutex::scoped_lock lock(mutex_);
      
    if (filter == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    } 

    if (state_ != State_Setup)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);        
    }

    LOG(INFO) << "Adding filter " << filter->GetName();
    filters_.push_back(filter);
  }

  
  void ServerContext::Start()
  {
    boost::mutex::scoped_lock lock(mutex_);
      
    if (state_ == State_Running)
    {
      return;  // Already running
    }
    else if (state_ == State_Done)
    {
      // Cannot run a second time
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);        
    }
    else
    {
      LOG(WARNING) << "Starting the filters";

      for (Filters::iterator it = filters_.begin(); it != filters_.end(); ++it)
      {
        assert(*it != NULL);
          
        if (!StartFilter(**it))
        {
          // Stopping the already-started filters before giving up
          for (Filters::iterator it2 = filters_.begin(); it2 != it; ++it2)
          {
            assert(*it2 != NULL);
            StopFilter(**it2);
          }

          LOG(ERROR) << "Cannot start one of the filters, stopping the Atom-IT server";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }      

      threads_.reserve(filters_.size());

      for (Filters::iterator it = filters_.begin(); it != filters_.end(); ++it)
      {
        threads_.push_back(new boost::thread(WorkerThread, &continue_, *it));
      }
        
      state_ = State_Running;
    }
  }


  void ServerContext::Stop()
  {
    boost::mutex::scoped_lock lock(mutex_);

    if (!StopInternal())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
