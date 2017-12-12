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


#include "FileWritersPool.h"

#include <Core/Logging.h>

namespace AtomIT
{
  class FileWritersPool::ActiveWriter
  {
  private:
    boost::mutex         mutex_;
    Toolbox::FileWriter  writer_;
    unsigned int         references_;

  public:
    ActiveWriter(boost::filesystem::path path,
                 bool append,
                 bool binary) :
      writer_(path, append, binary),
      references_(0)
    {
    }
      
    ~ActiveWriter()
    {
      if (references_ > 0)
      {
        LOG(ERROR) << "Some file writers are still active";
      }
    }

    class Lock : public boost::noncopyable
    {
    private:
      boost::mutex::scoped_lock  lock_;
      ActiveWriter&              that_;

    public:
      explicit Lock(ActiveWriter& that) :
        lock_(that.mutex_),
        that_(that)
      {
      }

      unsigned int& GetReferencesCounterRef()
      {
        return that_.references_;
      }

      unsigned int GetReferencesCounterValue() const
      {
        return that_.references_;
      }

      Toolbox::FileWriter& GetFileWriter() const
      {
        return that_.writer_;
      }
    };
  };
    

  FileWritersPool::Accessor::Accessor(FileWritersPool& pool,
                                      const boost::filesystem::path& path,
                                      bool append,
                                      bool binary,
                                      const std::string& header) :
    pool_(pool),
    path_(path)
  {
    {
      boost::mutex::scoped_lock lock(pool.poolMutex_);
        
      ActiveWriters::iterator found = pool.writers_.find(path);

      if (found != pool.writers_.end())
      {
        LOG(INFO) << "Reusing accessor to file: " << path;
        writer_ = found->second;
      }
      else
      {
        LOG(INFO) << "Opening file: " << path;
        writer_.reset(new ActiveWriter(path, append, binary));
        pool.writers_[path] = writer_;
      }
    }

    {
      assert(writer_.get() != NULL);

      ActiveWriter::Lock lock(*writer_);
      lock.GetReferencesCounterRef() += 1;

      if (lock.GetFileWriter().IsEmpty())
      {
        // Only write the header with the first accessor
        lock.GetFileWriter().Write(header);
      }
    }
  }


  FileWritersPool::Accessor::~Accessor()
  {
    assert(writer_.get() != NULL);
          
    {
      ActiveWriter::Lock lock(*writer_);
      assert(lock.GetReferencesCounterValue() > 0);

      if (lock.GetReferencesCounterRef() == 1)
      {
        // This was the last accessor to this file, close it and
        // remove it from the pool
        LOG(INFO) << "Closing file: " << path_;
        pool_.writers_.erase(path_);
      }
      else
      {
        LOG(INFO) << "Closing accessor to file: " << path_;
      }

      lock.GetReferencesCounterRef() -= 1;
    }
  }

  void FileWritersPool::Accessor::Write(const std::string& buffer)
  {
    assert(writer_.get() != NULL);

    {
      ActiveWriter::Lock lock(*writer_);
      assert(lock.GetReferencesCounterValue() > 0);
      lock.GetFileWriter().Write(buffer);
    }
  }
}
