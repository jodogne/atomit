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

#include "../Framework/ConfigurationSection.h"
#include "../Framework/TimeSeries/ITimeSeriesFactory.h"
#include "../Framework/TimeSeries/SQLiteBackend/SQLiteDatabase.h"

#include <boost/thread/mutex.hpp>

namespace AtomIT
{
  class MainTimeSeriesFactory : public ITimeSeriesFactory
  {
  private:
    enum Backend
    {
      Backend_None,
      Backend_SQLite,
      Backend_Memory
    };

    class TimeSeriesConfiguration
    {
    private:
      Backend          backend_;
      uint64_t         maxLength_;
      uint64_t         maxSize_;
      TimestampType    timestampType_;
      SQLiteDatabase*  sqlite_;  // Only valid if SQLite-based

    public:
      TimeSeriesConfiguration();

      TimeSeriesConfiguration(SQLiteDatabase& sqlite,
                              uint64_t maxLength,
                              uint64_t maxSize,
                              TimestampType timestampType);

      TimeSeriesConfiguration(Backend type,
                              uint64_t maxLength,
                              uint64_t maxSize,
                              TimestampType timestampType);

      Backend  GetBackend() const
      {
        return backend_;
      }

      TimestampType GetTimestampType() const
      {
        return timestampType_;
      }

      ITimeSeriesBackend* CreateTimeSeries(const std::string& name);
      
      std::string Format() const;

      static TimeSeriesConfiguration Parse(const ConfigurationSection& section,
                                           MainTimeSeriesFactory& that);
    };
    

    // This maps a path to the corresponding SQLite database
    typedef std::map<boost::filesystem::path, SQLiteDatabase*>   SQLiteDatabases;

    typedef std::map<std::string, TimeSeriesConfiguration>  ManualTimeSeries;

    boost::mutex             mutex_;
    SQLiteDatabases          databases_;
    ManualTimeSeries         manualTimeSeries_;
    TimeSeriesConfiguration  autoTimeSeries_;

    SQLiteDatabase& GetSQLiteDatabase(const boost::filesystem::path& path);

    void SetAutoConfiguration(const TimeSeriesConfiguration& timeSeries);

    void RegisterTimeSeries(const std::string& name,
                            const TimeSeriesConfiguration& timeSeries);

  public:
    ~MainTimeSeriesFactory();

    void RegisterMemoryTimeSeries(const std::string& name,
                                  uint64_t maxLength,
                                  uint64_t maxSize,
                                  TimestampType timestampType);

    void RegisterSQLiteTimeSeries(const std::string& name,
                                  const boost::filesystem::path& path,
                                  uint64_t maxLength,
                                  uint64_t maxSize,
                                  TimestampType timestampType);

    void SetAutoMemory(uint64_t maxLength,
                       uint64_t maxSize,
                       TimestampType timestampType);

    void SetAutoSQLite(const boost::filesystem::path& path,
                       uint64_t maxLength,
                       uint64_t maxSize,
                       TimestampType timestampType);

    virtual ITimeSeriesBackend* CreateManualTimeSeries(const std::string& name);

    virtual ITimeSeriesBackend* CreateAutoTimeSeries(TimestampType& timestampType /* out */,
                                                     const std::string& name);

    virtual void ListManualTimeSeries(std::map<std::string, TimestampType>& target);

    void LoadConfiguration(const ConfigurationSection& config);
  };
}
