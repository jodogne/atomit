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


#include "../Framework/TimeSeries/GenericTimeSeriesManager.h"
#include "../Framework/TimeSeries/TimeSeriesReader.h"
#include "../Framework/TimeSeries/TimeSeriesWriter.h"
#include "../Framework/TimeSeries/MemoryBackend/MemoryTimeSeriesBackend.h"
#include "../Framework/TimeSeries/SQLiteBackend/SQLiteTimeSeriesBackend.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <gtest/gtest.h>
#include <boost/lexical_cast.hpp>

enum BackendType
{
  BackendType_Memory,
  BackendType_SQLite
};

class BackendTest : public ::testing::TestWithParam<BackendType>
{
private:
  class FactoryBase : public AtomIT::ITimeSeriesFactory
  {
  protected:
    BackendTest&  that_;

  public:
    explicit FactoryBase(BackendTest& that) :
      that_(that)
    {
    }

    virtual void ListManualTimeSeries(std::map<std::string, AtomIT::TimestampType>& target)
    {
      target.clear();
    }                                      

    virtual AtomIT::ITimeSeriesBackend* CreateAutoTimeSeries(AtomIT::TimestampType& timestampType,
                                                             const std::string& name)
    {
      // These tests do not allow the automated creation of time series
      return NULL;
    }
  };

  class MemoryFactory : public FactoryBase
  {
  public:
    explicit MemoryFactory(BackendTest& that) :
      FactoryBase(that)
    {
    }
    
    virtual AtomIT::ITimeSeriesBackend* CreateManualTimeSeries(const std::string& name)
    {
      return new AtomIT::MemoryTimeSeriesBackend(that_.maxLength_, that_.maxSize_);
    }
  };

  class SQLiteFactory : public FactoryBase
  {
  private:
    AtomIT::SQLiteDatabase&  database_;
    
  public:
    SQLiteFactory(BackendTest& that,
                  AtomIT::SQLiteDatabase& database) :
      FactoryBase(that),
      database_(database)
    {
    }
    
    virtual AtomIT::ITimeSeriesBackend* CreateManualTimeSeries(const std::string& name)
    {
      database_.CreateTimeSeries(name, that_.maxLength_, that_.maxSize_);
      return new AtomIT::SQLiteTimeSeriesBackend(database_, name);
    }
  };

  uint64_t                                         maxLength_;
  uint64_t                                         maxSize_;
  std::auto_ptr<AtomIT::SQLiteDatabase>            sqlite_;
  std::auto_ptr<AtomIT::GenericTimeSeriesManager>  manager_;
  
public:
  virtual void SetUp()
  {
    maxLength_ = 0;
    maxSize_ = 0;

    std::auto_ptr<AtomIT::ITimeSeriesFactory> factory;

    switch (GetParam())
    {
      case BackendType_Memory:
        factory.reset(new MemoryFactory(*this));
        break;

      case BackendType_SQLite:
        sqlite_.reset(new AtomIT::SQLiteDatabase);  // Test in-memory SQLite DB
        //sqlite_.reset(new AtomIT::SQLiteDatabase("test.db"));
        factory.reset(new SQLiteFactory(*this, *sqlite_));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    manager_.reset(new AtomIT::GenericTimeSeriesManager(factory.release()));
  }

  virtual void TearDown()
  {
    // Don't switch the commands below to ensure destruction in the correct order
    manager_.reset(NULL);

    if (sqlite_.get() != NULL)
    {
      sqlite_->DeleteTimeSeries("hello");
      sqlite_->DeleteTimeSeries("world");
    }
    
    sqlite_.reset(NULL);
  }

  AtomIT::GenericTimeSeriesManager& GetManager()
  {
    if (manager_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return *manager_;
    }
  }

  void SetQuota(uint64_t maxLength,
                uint64_t maxSize)
  {
    maxLength_ = maxLength;
    maxSize_ = maxSize;
  }

  uint64_t GetLength(const std::string& timeSeries)
  {
    uint64_t length;

    {
      AtomIT::TimeSeriesReader reader(GetManager(), timeSeries, false);
      AtomIT::TimeSeriesReader::Transaction transaction(reader);

      uint64_t size;
      transaction.GetStatistics(length, size);
    }

    return length;
  }

  uint64_t GetSize(const std::string& timeSeries)
  {
    uint64_t size;

    {
      AtomIT::TimeSeriesReader reader(GetManager(), timeSeries, false);
      AtomIT::TimeSeriesReader::Transaction transaction(reader);

      uint64_t length;
      transaction.GetStatistics(length, size);
    }

    return size;
  }

  bool CheckStatistics(const std::string& timeSeries)
  {
    AtomIT::TimeSeriesReader reader(GetManager(), timeSeries, false);

    uint64_t length, size;

    {
      AtomIT::TimeSeriesReader::Transaction transaction(reader);
      transaction.GetStatistics(length, size);
    }

    uint64_t l2 = 0, s2 = 0;
    
    {
      AtomIT::TimeSeriesReader::Transaction transaction(reader);
      transaction.SeekFirst();
      
      for (;;)
      {
        if (transaction.IsValid())
        {
          std::string metadata, value;
          if (!transaction.Read(metadata, value))
          {
            return false;
          }

          l2 += 1;
          s2 += value.size();
        }

        if (!transaction.SeekNext())
        {
          break;
        }
      }
    }

    return (length == l2 && size == s2);
  }
};


INSTANTIATE_TEST_CASE_P(BackendTestInstances,
                        BackendTest,
                        ::testing::Values(
                          BackendType_Memory,
                          BackendType_SQLite));


TEST_P(BackendTest, CreateTimeSeries)
{
  std::set<std::string> series;
  GetManager().ListTimeSeries(series);
  ASSERT_TRUE(series.empty());

  ASSERT_THROW(GetLength("hello"), Orthanc::OrthancException);
  ASSERT_THROW(GetSize("hello"), Orthanc::OrthancException);

  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  GetManager().ListTimeSeries(series);
  ASSERT_EQ(1u, series.size());
  ASSERT_TRUE(series.find("hello") != series.end());

  ASSERT_EQ(0llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));
  ASSERT_THROW(GetLength("world"), Orthanc::OrthancException);
  ASSERT_THROW(GetSize("world"), Orthanc::OrthancException);

  GetManager().CreateTimeSeries("world", AtomIT::TimestampType_Sequence);
  GetManager().ListTimeSeries(series);
  ASSERT_EQ(2u, series.size());
  ASSERT_TRUE(series.find("hello") != series.end());
  ASSERT_TRUE(series.find("world") != series.end());

  ASSERT_EQ(0llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_EQ(0llu, GetLength("world"));
  ASSERT_EQ(0llu, GetSize("world"));

  ASSERT_THROW(GetManager().CreateTimeSeries("world", AtomIT::TimestampType_Sequence),
               Orthanc::OrthancException);
  ASSERT_THROW(GetManager().DeleteTimeSeries("nope"), Orthanc::OrthancException);

  GetManager().DeleteTimeSeries("world");
  GetManager().ListTimeSeries(series);
  ASSERT_EQ(1u, series.size());
  ASSERT_TRUE(series.find("hello") != series.end());

  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_THROW(GetLength("world"), Orthanc::OrthancException);

  GetManager().DeleteTimeSeries("hello");
  GetManager().ListTimeSeries(series);
  ASSERT_TRUE(series.empty());

  ASSERT_THROW(GetLength("hello"), Orthanc::OrthancException);
}


TEST_P(BackendTest, SimpleWriter)
{
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  ASSERT_EQ(0llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));

  int64_t t;
  std::string m, v;

  AtomIT::TimeSeriesReader reader(GetManager(), "hello", true);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_FALSE(transaction.IsValid());
    ASSERT_FALSE(transaction.GetTimestamp(t));
    ASSERT_FALSE(transaction.SeekFirst());
    ASSERT_FALSE(transaction.SeekLast());
    ASSERT_FALSE(transaction.SeekNearest(120));
    ASSERT_FALSE(transaction.SeekNext());
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_FALSE(transaction.Read(m, v));
    ASSERT_FALSE(transaction.IsValid());
    ASSERT_FALSE(transaction.GetTimestamp(t));

    transaction.Seek(100);
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(100, t);    
    ASSERT_FALSE(transaction.SeekFirst());
    ASSERT_FALSE(transaction.SeekLast());
    ASSERT_FALSE(transaction.SeekNearest(120));
    ASSERT_FALSE(transaction.SeekNext());
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_FALSE(transaction.Read(m, v));
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(100, t);    
  }

  size_t count = 0;
  for (unsigned int i = 0; i < 50; i++)
  {
    {
      AtomIT::TimeSeriesWriter::Transaction transaction(writer);

      if (i == 0)
      {
        ASSERT_FALSE(transaction.GetLastTimestamp(t));
      }
      else
      {
        ASSERT_TRUE(transaction.GetLastTimestamp(t));
        ASSERT_EQ((i - 1) * 10, t);
      }

      std::string s = boost::lexical_cast<std::string>(i);
      ASSERT_TRUE(transaction.Append(10 * i, "metadata " + s, "value " + s));
      ASSERT_TRUE(transaction.GetLastTimestamp(t));
      ASSERT_EQ(i * 10, t);

      count += ("value " + s).size();
    }

    ASSERT_EQ(i + 1, GetLength("hello"));
    ASSERT_EQ(count, GetSize("hello"));
    ASSERT_TRUE(CheckStatistics("hello"));
  }    

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_FALSE(transaction.IsValid());
    ASSERT_FALSE(transaction.GetTimestamp(t));

    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(0, t);
    ASSERT_EQ("metadata 0", m);
    ASSERT_EQ("value 0", v);

    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(0, t);

    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(490, t);
    ASSERT_EQ("metadata 49", m);
    ASSERT_EQ("value 49", v);

    ASSERT_FALSE(transaction.SeekNext());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(490, t);

    ASSERT_TRUE(transaction.SeekNearest(111));
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(120, t);
    ASSERT_EQ("metadata 12", m);
    ASSERT_EQ("value 12", v);

    ASSERT_TRUE(transaction.SeekNearest(120));
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(120, t);
    ASSERT_EQ("metadata 12", m);
    ASSERT_EQ("value 12", v);

    ASSERT_TRUE(transaction.SeekNext());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(130, t);
    ASSERT_EQ("metadata 13", m);
    ASSERT_EQ("value 13", v);

    ASSERT_TRUE(transaction.SeekPrevious());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(120, t);
    ASSERT_EQ("metadata 12", m);
    ASSERT_EQ("value 12", v);

    transaction.Seek(100);
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(100, t);
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ("metadata 10", m);
    ASSERT_EQ("value 10", v);

    transaction.Seek(101);
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(101, t);
    ASSERT_FALSE(transaction.Read(m, v));

    ASSERT_TRUE(transaction.SeekNext());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ(110, t);
    ASSERT_EQ("metadata 11", m);
    ASSERT_EQ("value 11", v);
  }
}


TEST_P(BackendTest, LengthRecycling)
{
  SetQuota(10, 0);
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  int64_t t;
  std::string m, v;

  AtomIT::TimeSeriesReader reader(GetManager(), "hello", true);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  for (unsigned int i = 0; i < 50; i++)
  {
    {
      AtomIT::TimeSeriesWriter::Transaction transaction(writer);
      std::string s = boost::lexical_cast<std::string>(i);
      ASSERT_TRUE(transaction.Append(10 * i, "metadata " + s, "value " + s));
      ASSERT_FALSE(transaction.Append(10 * i, "", ""));  // Not after the trailer
    }

    {
      AtomIT::TimeSeriesReader::Transaction transaction(reader);
      ASSERT_TRUE(transaction.SeekFirst());
      ASSERT_TRUE(transaction.IsValid());
      ASSERT_TRUE(transaction.GetTimestamp(t));
      ASSERT_EQ(i < 10 ? 0 : 10 * (i - 9), t);
    }

    ASSERT_EQ(i < 10 ? i + 1 : 10, GetLength("hello"));
    ASSERT_TRUE(CheckStatistics("hello"));
  }

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);

    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(490, t);
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ("metadata 49", m);
    ASSERT_EQ("value 49", v);

    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.IsValid());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(400, t);
    ASSERT_TRUE(transaction.Read(m, v));
    ASSERT_EQ("metadata 40", m);
    ASSERT_EQ("value 40", v);

    unsigned int count = 1;
    while (transaction.SeekNext())
    {
      count++;
    }

    ASSERT_EQ(10u, count);
  }

  ASSERT_EQ(10llu, GetLength("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    transaction.ClearContent();
  }

  ASSERT_EQ(0llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));
}


TEST_P(BackendTest, SizeRecycling)
{
  SetQuota(0, 10);
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  int64_t t;

  AtomIT::TimeSeriesReader reader(GetManager(), "hello", true);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.Append(0, "", "0123456789"));
    ASSERT_FALSE(transaction.Append(1, "", "0123456789a"));  // Too long (> 10)
  }

  ASSERT_EQ(1llu, GetLength("hello"));
  ASSERT_EQ(10llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(0, t);
    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(0, t);
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.Append(1, "", "01234"));
  }

  ASSERT_EQ(1llu, GetLength("hello"));
  ASSERT_EQ(5llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(1, t);
    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(1, t);
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.Append(2, "", "56789"));
  }

  ASSERT_EQ(2llu, GetLength("hello"));
  ASSERT_EQ(10llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(1, t);
    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(2, t);
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.Append(3, "", "012345"));
  }

  ASSERT_EQ(1llu, GetLength("hello"));
  ASSERT_EQ(6llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(3, t);
    ASSERT_TRUE(transaction.SeekLast());
    ASSERT_TRUE(transaction.GetTimestamp(t));
    ASSERT_EQ(3, t);
  }
}


TEST_P(BackendTest, DeleteRange)
{
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  int64_t t;

  AtomIT::TimeSeriesReader reader(GetManager(), "hello", true);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  for (unsigned int i = 0; i < 10; i++)
  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.Append(i, "", ""));
  }

  ASSERT_EQ(10llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    transaction.DeleteRange(3, 7);
    transaction.DeleteRange(-10, 0);   // No effect
    transaction.DeleteRange(10, 100);  // No effect
    transaction.DeleteRange(100, -20); // No effect
  }

  ASSERT_EQ(6llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst()); ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(0, t);
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(1, t);
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(2, t);
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(7, t);
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(8, t);
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(9, t);
    ASSERT_TRUE(transaction.SeekLast());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(9, t);
    ASSERT_FALSE(transaction.SeekNext());
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    transaction.DeleteRange(-10, 2);
    transaction.DeleteRange(9, 20);
  }

  ASSERT_EQ(3llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst()); ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(2, t);
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(7, t);
    ASSERT_TRUE(transaction.SeekNext());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(8, t);
    ASSERT_TRUE(transaction.SeekLast());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(8, t);
    ASSERT_FALSE(transaction.SeekNext());
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    transaction.DeleteRange(3, 20);
    transaction.DeleteRange(2, 2);    // No effect
  }

  ASSERT_EQ(1llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_TRUE(transaction.SeekFirst()); ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(2, t);
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_FALSE(transaction.SeekNext());
    ASSERT_TRUE(transaction.SeekLast());  ASSERT_TRUE(transaction.GetTimestamp(t)); ASSERT_EQ(2, t);
  }

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    transaction.DeleteRange(2, 3);
  }

  ASSERT_EQ(0llu, GetLength("hello"));
  ASSERT_EQ(0llu, GetSize("hello"));
  ASSERT_TRUE(CheckStatistics("hello"));

  {
    AtomIT::TimeSeriesReader::Transaction transaction(reader);
    ASSERT_FALSE(transaction.SeekFirst());
    ASSERT_FALSE(transaction.SeekPrevious());
    ASSERT_FALSE(transaction.SeekNext());
    ASSERT_FALSE(transaction.SeekLast());
  }
}



TEST_P(BackendTest, Sequence)
{
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  int64_t t;
  
  AtomIT::Message message;
  message.SetTimestampType(AtomIT::TimestampType_Sequence);

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_FALSE(transaction.GetLastTimestamp(t));
  }

  ASSERT_TRUE(writer.Append(message));

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.GetLastTimestamp(t));
    ASSERT_EQ(0ll, t);
  }

  ASSERT_TRUE(writer.Append(message));

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.GetLastTimestamp(t));
    ASSERT_EQ(1ll, t);

    transaction.ClearContent();

    ASSERT_TRUE(transaction.GetLastTimestamp(t));
    ASSERT_EQ(1ll, t);
  }

  ASSERT_TRUE(writer.Append(message));

  {
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.GetLastTimestamp(t));
    ASSERT_EQ(2ll, t);
  }
}

TEST_P(BackendTest, Sequence2)
{
  GetManager().CreateTimeSeries("hello", AtomIT::TimestampType_Sequence);
  AtomIT::TimeSeriesWriter writer(GetManager(), "hello");

  AtomIT::Message message;

  ASSERT_TRUE(writer.Append(message));

  message.SetTimestamp(10);
  ASSERT_TRUE(writer.Append(message));

  {
    int64_t t;
  
    AtomIT::TimeSeriesWriter::Transaction transaction(writer);
    ASSERT_TRUE(transaction.GetLastTimestamp(t));
    ASSERT_EQ(10ll, t);

    transaction.DeleteRange(10, 11);
    ASSERT_EQ(10ll, t);
  }

  ASSERT_FALSE(writer.Append(message));
}



static uint64_t GetLength(AtomIT::SQLiteDatabase& db,
                          const std::string& name)
{
  uint64_t length, size;  
  AtomIT::SQLiteTimeSeriesTransaction t(db, name);
  t.GetStatistics(length, size);
  return length;
}


static uint64_t GetSize(AtomIT::SQLiteDatabase& db,
                        const std::string& name)
{
  uint64_t length, size;  
  AtomIT::SQLiteTimeSeriesTransaction t(db, name);
  t.GetStatistics(length, size);
  return size;
}


TEST(SQLiteBackend, ChangeQuota)
{
  AtomIT::SQLiteDatabase db;

  db.CreateTimeSeries("world", 0, 0);

  for (unsigned int i = 0; i < 10; i++)
  {
    AtomIT::SQLiteTimeSeriesTransaction t(db, "world");
    ASSERT_TRUE(t.Append(i, "", "v" + boost::lexical_cast<std::string>(i)));
  }

  ASSERT_EQ(10u, GetLength(db, "world"));
  ASSERT_EQ(20u, GetSize(db, "world"));

  db.CreateTimeSeries("world", 0, 0);

  // Change quota
  db.CreateTimeSeries("world", 5, 0);

  ASSERT_EQ(5u, GetLength(db, "world"));
  ASSERT_EQ(10u, GetSize(db, "world"));

  db.CreateTimeSeries("world", 0, 6);

  ASSERT_EQ(3u, GetLength(db, "world"));
  ASSERT_EQ(6u, GetSize(db, "world"));

  db.CreateTimeSeries("world", 0, 4);

  ASSERT_EQ(2u, GetLength(db, "world"));
  ASSERT_EQ(4u, GetSize(db, "world"));
}
