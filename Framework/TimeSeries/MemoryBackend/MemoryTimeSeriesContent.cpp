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


#include "MemoryTimeSeriesContent.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  class MemoryTimeSeriesContent::Item : public boost::noncopyable
  {
  private:
    std::string   metadata_;
    std::string   value_;

  public:
    Item(const std::string& metadata,
         const std::string& value) :
      metadata_(metadata),
      value_(value)
    {
    }

    const std::string& GetMetadata() const
    {
      return metadata_;
    }

    const std::string& GetValue() const
    {
      return value_;
    }

    void SetValue(const std::string& metadata,
                  const std::string& value)
    {
      metadata_ = metadata;
      value_ = value;
    }
  };
    

  void MemoryTimeSeriesContent::RemoveOldest()
  {
    if (!content_.empty())
    {
      Content::iterator oldest = content_.begin();
      size_t oldestSize = oldest->second->GetValue().size();
      delete oldest->second;
      content_.erase(oldest);
      size_ -= oldestSize;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  MemoryTimeSeriesContent::MemoryTimeSeriesContent(uint64_t maxLength,
                                                   uint64_t maxSize) :
    size_(0),
    maxLength_(maxLength),
    maxSize_(maxSize),
    hasLastTimestamp_(false),
    lastTimestamp_(0)  // Dummy initialization
  {
  }

    
  MemoryTimeSeriesContent::~MemoryTimeSeriesContent()
  {
    ClearContent();
  }    

    
  void MemoryTimeSeriesContent::SetValue(int64_t timestamp,
                                         const std::string& metadata,
                                         const std::string& value)
  {
    Content::iterator found = content_.find(timestamp);

    if (found == content_.end())
    {
      if (maxLength_ != 0 &&
          content_.size() + 1 > maxLength_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      if (maxSize_ != 0 &&
          size_ + value.size() > maxSize_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      content_[timestamp] = new Item(metadata, value);
      size_ += value.size();
    }
    else
    {
      size_t oldSize = found->second->GetValue().size();
      found->second->SetValue(metadata, value);
      size_ = size_ - oldSize + value.size();
    }
  }
    

  void MemoryTimeSeriesContent::DeleteRange(int64_t start,
                                            int64_t end)
  {
    if (start >= end)
    {
      return;
    }
      
    Content::iterator from = content_.lower_bound(start);
    Content::iterator to = content_.lower_bound(end);

    for (Content::iterator it = from; it != to; ++it)
    {
      size_ -= it->second->GetValue().size();
      delete it->second;
    }
      
    content_.erase(from, to);
  }
    
    
  bool MemoryTimeSeriesContent::SeekFirst(int64_t& result) const
  {
    if (content_.empty())
    {
      return false;
    }
    else
    {
      result = content_.begin()->first;
      return true;
    }
  }
    

  bool MemoryTimeSeriesContent::SeekLast(int64_t& result) const
  {
    if (content_.empty())
    {
      return false;
    }
    else
    {
      result = content_.rbegin()->first;
      return true;
    }
  }

    
  bool MemoryTimeSeriesContent::SeekNearest(int64_t& result,
                                            int64_t timestamp) const
  {
    Content::const_iterator found = content_.lower_bound(timestamp);
      
    if (found == content_.end())
    {
      return false;
    }
    else
    {
      result = found->first;
      return true;
    }
  }
    

  bool MemoryTimeSeriesContent::SeekNext(int64_t& result,
                                         int64_t timestamp) const
  {
    Content::const_iterator found = content_.upper_bound(timestamp);

    if (found == content_.end())
    {
      return false;
    }
    else
    {
      result = found->first;
      return true;
    }
  }

    
  bool MemoryTimeSeriesContent::SeekPrevious(int64_t& result,
                                             int64_t timestamp) const
  {
    /**
     * "In a sorted container, the last element that is less than
     * or equivalent to x, is the element before the first element
     * that is greater than x. Thus you can call std::upper_bound,
     * and decrement the returned iterator once. (Before
     * decrementing, you must of course check that it is not the
     * begin iterator; if it is, then there are no elements that
     * are less than or equivalent to x.)"
     * https://stackoverflow.com/a/9989773/881731
     **/

    if (content_.empty())
    {
      return false;
    }

    Content::const_iterator found = content_.lower_bound(timestamp);

    if (found == content_.begin())
    {
      return false;
    }
    else
    {
      --found;
      result = found->first;
      return true;
    }
  }


  bool MemoryTimeSeriesContent::Read(std::string& metadata,
                                     std::string& value,
                                     int64_t timestamp) const
  {
    Content::const_iterator found = content_.find(timestamp);

    if (found == content_.end())
    {
      return false;
    }
    else
    {
      metadata.assign(found->second->GetMetadata());
      value.assign(found->second->GetValue());
      return true;
    }
  }
    

  bool MemoryTimeSeriesContent::Append(int64_t timestamp,
                                       const std::string& metadata,
                                       const std::string& value)
  {
    if (maxSize_ != 0 &&
        value.size() > maxSize_)
    {
      LOG(ERROR) << "Cannot append an observation whose size (" << value.size()
                 << " bytes) is above the max size of the time series (" << maxSize_
                 << " bytes)";
      return false;
    }

    if (hasLastTimestamp_ &&
        timestamp <= lastTimestamp_)
    {
      return false;
    }
      
    if (maxLength_ != 0)
    {
      while (content_.size() + 1 > maxLength_)
      {
        RemoveOldest();
      }
    }

    if (maxSize_ != 0)
    {
      while (size_ + value.size() > maxSize_)
      {
        RemoveOldest();
      }
    }

    SetValue(timestamp, metadata, value);

    if (hasLastTimestamp_)
    {
      lastTimestamp_ = timestamp;
    }
    else
    {
      hasLastTimestamp_ = true;
      lastTimestamp_ = timestamp;
    }
        
    return true;
  }


  void MemoryTimeSeriesContent::GetStatistics(uint64_t& length,
                                              uint64_t& size) const
  {
    length = content_.size();
    size = size_;
  }

  
  void MemoryTimeSeriesContent::ClearContent()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    content_.clear();

    size_ = 0;
  }

  
  bool MemoryTimeSeriesContent::GetLastTimestamp(int64_t& result) const
  {
    if (hasLastTimestamp_)
    {
      result = lastTimestamp_;
      return true;
    }
    else
    {
      return false;
    }
  }
}
