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


#include "GenericTimeSeriesManager.h"

#include <boost/thread.hpp>

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  class GenericTimeSeriesManager::TimeSeries : public boost::noncopyable
  {
  private:
    typedef std::set<ITimeSeriesObserver*>   Observers;
    typedef std::set<Accessor*>              Accessors;
    typedef boost::shared_mutex              Mutex;
    typedef boost::shared_lock<Mutex>        BackendLock;
    typedef boost::unique_lock<Mutex>        ExclusiveLock;

    Mutex                              mutex_;
    std::string                        name_;
    std::auto_ptr<ITimeSeriesBackend>  backend_;
    TimestampType                      defaultTimestamp_;
    Observers                          observers_;
    Accessors                          accessors_;

    void NotifyModification(BackendLock& /* ensure proper locking */)
    {
      for (Observers::iterator it = observers_.begin();
           it != observers_.end(); ++it)
      {
        assert(*it != NULL);
        (*it)->NotifySeriesModified(name_);
      }
    }

  public:
    TimeSeries(const std::string& name,
               ITimeSeriesBackend* backend,
               TimestampType type) :
      name_(name),
      backend_(backend),
      defaultTimestamp_(type)
    {
      if (backend == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      LOG(INFO) << "Time series created: " << name_;
    }

    ~TimeSeries()
    {
      assert(observers_.empty());
      assert(accessors_.empty());
      LOG(INFO) << "Time series deleted: " << name_;
    }

    void Delete()
    {
      ExclusiveLock lock(mutex_);

      for (Observers::iterator it = observers_.begin();
           it != observers_.end(); ++it)
      {
        assert(*it != NULL);
        (*it)->NotifySeriesDeleted(name_);
      }
        
      backend_.reset(NULL);
    }
                     
    void RegisterAccessor(Accessor& accessor)
    {
      ExclusiveLock lock(mutex_);
        
      if (accessors_.find(&accessor) != accessors_.end())
      {
        LOG(ERROR) << "Cannot register twice the same accessor";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        accessors_.insert(&accessor);
      }
    }

    void UnregisterAccessor(Accessor& accessor)
    {
      ExclusiveLock lock(mutex_);
        
      if (accessors_.find(&accessor) == accessors_.end())
      {
        LOG(ERROR) << "Trying to unregister an unknown accessor";
      }
      else
      {
        accessors_.erase(&accessor);
      }
    }

    void RegisterObserver(ITimeSeriesObserver& observer)
    {
      ExclusiveLock lock(mutex_);
        
      if (observers_.find(&observer) != observers_.end())
      {
        LOG(ERROR) << "Cannot register twice the same observer";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        observers_.insert(&observer);
      }
    }

    void UnregisterObserver(ITimeSeriesObserver& observer)
    {
      ExclusiveLock lock(mutex_);
        
      if (observers_.find(&observer) == observers_.end())
      {
        LOG(ERROR) << "Trying to unregister an unknown observer";
      }
      else
      {
        observers_.erase(&observer);
      }
    }

    class Lock : public ITimeSeriesAccessor::ILock
    {
    private:
      BackendLock  lock_;
      TimeSeries&  that_;

    public:
      explicit Lock(TimeSeries& that) :
        lock_(that.mutex_),
        that_(that)
      {
      }

      virtual bool HasBackend() const
      {
        return that_.backend_.get() != NULL;
      }

      virtual ITimeSeriesBackend& GetBackend()
      {
        assert(HasBackend());
        return *that_.backend_;
      }

      virtual void NotifyModification()
      {
        that_.NotifyModification(lock_);
      }

      virtual TimestampType GetDefaultTimestampType() const
      {
        return that_.defaultTimestamp_;
      }
    };
  };


  class GenericTimeSeriesManager::Accessor : public ITimeSeriesAccessor
  {
  protected:
    boost::shared_ptr<TimeSeries>  timeSeries_;

  public:
    explicit Accessor(boost::shared_ptr<TimeSeries>& timeSeries) :
      timeSeries_(timeSeries)
    {
      if (timeSeries.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
        timeSeries_->RegisterAccessor(*this);
      }
    }

    virtual ~Accessor()
    {
      timeSeries_->UnregisterAccessor(*this);
    }

    virtual ILock* Lock()
    {
      return new TimeSeries::Lock(*timeSeries_);
    }
      
    virtual bool WaitModification(unsigned int milliseconds)
    {
      LOG(WARNING) << "Cannot use WaitModification(), as hasSynchronousWait is set to false";
      boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
      return true;
    }
  };


  class GenericTimeSeriesManager::SynchronousAccessor :
    public GenericTimeSeriesManager::Accessor,
    private ITimeSeriesObserver
  {
  private:
    boost::mutex               mutex_;
    boost::condition_variable  condition_;
    bool                       isModified_;

    void NotifyModification()
    {
      boost::mutex::scoped_lock lock(mutex_);
      isModified_ = true;
      condition_.notify_one();
    }
      
    virtual void NotifySeriesDeleted(const std::string& name)
    {
      NotifyModification();
    }

    virtual void NotifySeriesModified(const std::string& name)
    {
      NotifyModification();
    }

  public:
    explicit SynchronousAccessor(boost::shared_ptr<TimeSeries>& timeSeries) :
      Accessor(timeSeries),
      isModified_(false)
    {
      timeSeries_->RegisterObserver(*this);
    }

    virtual ~SynchronousAccessor()
    {
      timeSeries_->UnregisterObserver(*this);
    }

    virtual bool WaitModification(unsigned int milliseconds)
    {
      boost::mutex::scoped_lock lock(mutex_);

      while (!isModified_)
      {
        if (!condition_.timed_wait(lock, boost::posix_time::milliseconds(milliseconds)))
        {
          return false;
        }
      }

      isModified_ = false;
      return true;
    }
  };

    
  boost::shared_ptr<GenericTimeSeriesManager::TimeSeries>&
  GenericTimeSeriesManager::GetTimeSeries(const std::string& name)
  {
    Content::iterator found = content_.find(name);

    if (found == content_.end())
    {
      TimestampType timestampType;
      std::auto_ptr<ITimeSeriesBackend> backend
        (factory_->CreateAutoTimeSeries(timestampType, name));

      if (backend.get() != NULL)
      {
        LOG(WARNING) << "Auto-creation of time series: " << name;
        boost::shared_ptr<TimeSeries> series
          (new TimeSeries(name, backend.release(), timestampType));
        content_[name] = series;
        return content_[name];
      }
      else
      {
        LOG(ERROR) << "Unknown time series: " << name;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
      }
    }
    else
    {
      return found->second;
    }
  }
    
  
  GenericTimeSeriesManager::GenericTimeSeriesManager(ITimeSeriesFactory* factory) :
    factory_(factory)
  {
    if (factory == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    typedef std::map<std::string, TimestampType>  List;

    List timeSeries;
    factory->ListManualTimeSeries(timeSeries);

    for (List::const_iterator it = timeSeries.begin(); it != timeSeries.end(); ++it)
    {
      CreateTimeSeries(it->first, it->second);
    }
  }

  
  GenericTimeSeriesManager::~GenericTimeSeriesManager()
  {
    for (Content::iterator it = content_.begin();
         it != content_.end(); ++it)
    {
      it->second.reset();
    }
  }
  
    
  void GenericTimeSeriesManager::Register(ITimeSeriesObserver& observer,
                                          const std::string& timeSeries)
  {
    boost::mutex::scoped_lock lock(mutex_);

    try
    {
      GetTimeSeries(timeSeries)->RegisterObserver(observer);
    }
    catch (Orthanc::OrthancException&)
    {
      // The series was deleted
    }
  }
  

  void GenericTimeSeriesManager::Unregister(ITimeSeriesObserver& observer,
                                            const std::string& timeSeries)
  {
    boost::mutex::scoped_lock lock(mutex_);

    try
    {
      GetTimeSeries(timeSeries)->UnregisterObserver(observer);
    }
    catch (Orthanc::OrthancException&)
    {
      // The series was deleted
    }
  }
  

  void GenericTimeSeriesManager::ListTimeSeries(std::set<std::string>& target)
  {
    boost::mutex::scoped_lock lock(mutex_);

    target.clear();

    for (Content::const_iterator it = content_.begin();
         it != content_.end(); ++it)
    {
      target.insert(it->first);
    }
  }

  
  void GenericTimeSeriesManager::CreateTimeSeries(const std::string& name,
                                                  TimestampType timestampType)
  {
    boost::mutex::scoped_lock lock(mutex_);

    LOG(WARNING) << "Creating time series: " << name;

    if (content_.find(name) != content_.end())
    {
      LOG(ERROR) << "Cannot create twice the same time series: " << name;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      std::auto_ptr<ITimeSeriesBackend> backend(factory_->CreateManualTimeSeries(name));
      boost::shared_ptr<TimeSeries> series(new TimeSeries(name, backend.release(), timestampType));
      content_[name] = series;
    }
  }

  
  void GenericTimeSeriesManager::DeleteTimeSeries(const std::string& name)
  {
    boost::mutex::scoped_lock lock(mutex_);

    Content::iterator found = content_.find(name);

    if (found == content_.end())
    {
      LOG(ERROR) << "Unknown time series: " << name;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
    else
    {
      found->second->Delete();
      content_.erase(found);
    }
  }

  
  ITimeSeriesAccessor* GenericTimeSeriesManager::CreateAccessor(const std::string& name,
                                                                bool hasSynchronousWait)
  {
    boost::mutex::scoped_lock lock(mutex_);

    boost::shared_ptr<TimeSeries> series(GetTimeSeries(name));

    if (hasSynchronousWait)
    {
      return new SynchronousAccessor(series);
    }
    else
    {
      return new Accessor(series);
    }
  }
}
