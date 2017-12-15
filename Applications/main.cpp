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


#include "FilterFactory.h"
#include "MainTimeSeriesFactory.h"
#include "AtomITRestApi.h"

#include "../Framework/TimeSeries/GenericTimeSeriesManager.h"
#include "../Framework/MQTT/SynchronousClient.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/SystemToolbox.h>
#include <Core/HttpServer/FilesystemHttpHandler.h>
#include <Core/HttpServer/EmbeddedResourceHttpHandler.h>
#include <Core/HttpServer/MongooseServer.h>
#include <Core/HttpClient.h>
#include <OrthancServer/OrthancHttpHandler.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>


namespace AtomIT
{
  static ConfigurationSection  globalConfiguration_;
  

  // "configurationFile" can be NULL if no command-line argument is given
  void GlobalInitialization(const char* path)
  {
    if (path != NULL)
    {
      globalConfiguration_.LoadFile(path);
    }
  }

  void GlobalFinalization()
  {
  }


  static bool StartHttpServer(ServerContext& context)
  {
    AtomITRestApi api(context);

#if ATOMIT_STANDALONE == 1
    Orthanc::EmbeddedResourceHttpHandler staticResources
      ("/app", Orthanc::EmbeddedResources::ATOMIT_WEB_INTERFACE);
#else
    Orthanc::FilesystemHttpHandler staticResources
      ("/app", ATOMIT_ROOT "/WebInterface");
#endif

    Orthanc::OrthancHttpHandler handler;
    handler.Register(api, true);
    handler.Register(staticResources, false);
      
    Orthanc::MongooseServer httpServer;
    httpServer.Register(handler);

    bool b;

    if (globalConfiguration_.GetBooleanParameter(b, "RemoteAccessAllowed"))
    {
      httpServer.SetRemoteAccessAllowed(b);
    }
    else
    {
      httpServer.SetRemoteAccessAllowed(false);
    }

    if (globalConfiguration_.GetBooleanParameter(b, "AuthenticationEnabled"))
    {
      httpServer.SetAuthenticationEnabled(b);
    }
    else
    {
      httpServer.SetAuthenticationEnabled(false);
    }

    unsigned int v;
    if (globalConfiguration_.GetUnsignedIntegerParameter(v, "HttpPort"))
    {
      if (v <= 65535)
      {
        httpServer.SetPortNumber(v);
      }
      else
      {
        LOG(ERROR) << "Bad value for a TCP port: " << v;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }
    else
    {
      httpServer.SetPortNumber(8042);
    }

    static const char* USERS = "RegisteredUsers";
    if (globalConfiguration_.HasItem(USERS))
    {
      ConfigurationSection section(globalConfiguration_, USERS);

      std::set<std::string> users;
      section.ListMembers(users);

      for (std::set<std::string>::const_iterator
             username = users.begin(); username != users.end(); ++username)
      {
        std::string password = section.GetMandatoryStringParameter(*username);
        httpServer.RegisterUser(username->c_str(), password.c_str());
      }
    }

    if (httpServer.GetPortNumber() < 1024)
    {
      LOG(WARNING) << "The HTTP port is privileged (" 
                   << httpServer.GetPortNumber() << " is below 1024), "
                   << "make sure you run Atom-IT as root/administrator";
    }
      
    httpServer.Start();
    LOG(WARNING) << "HTTP server listening on port: " << httpServer.GetPortNumber();
        
    Orthanc::SystemToolbox::ServerBarrier();

    httpServer.Stop();
    LOG(WARNING) << "    HTTP server has stopped";

    return false;                                                    
  }


  // Returns "true" if the server must restart
  bool StartServer(int argc,
                   char* argv[],
                   bool hasConfiguration)
  {
    std::auto_ptr<MainTimeSeriesFactory> f(new MainTimeSeriesFactory);

    if (hasConfiguration)
    {
      f->LoadConfiguration(globalConfiguration_);
    }
    else
    {
      f->SetAutoMemory(0, 0, TimestampType_Sequence);
    }

    GenericTimeSeriesManager manager(f.release());
 
    ServerContext context(manager);    
    
    const char* FILTERS = "Filters";
    if (globalConfiguration_.HasItem(FILTERS))
    {
      size_t size = globalConfiguration_.GetSize(FILTERS);
      
      for (size_t i = 0; i < size; i++)
      {
        ConfigurationSection config(globalConfiguration_, FILTERS, i);
        context.AddFilter(CreateFilter(manager, context.GetFileWritersPool(), config));
      }
    }

    context.Start();
    LOG(WARNING) << "The Atom-IT server has started";

    bool httpServerEnabled;
    if (!globalConfiguration_.GetBooleanParameter(httpServerEnabled, "HttpServerEnabled"))
    {
      httpServerEnabled = true;  // Start the HTTP server by default
    }

    bool restart;

    if (httpServerEnabled)
    {
      restart = StartHttpServer(context);
    }
    else
    {
      LOG(WARNING) << "The HTTP server is disabled";
      Orthanc::SystemToolbox::ServerBarrier();
      restart = false;
    }

    LOG(INFO) << "The Atom-IT server is stopping";
    context.Stop();

    return restart;  // Don't restart the server
  }

  void PrintHelp(const std::string& path)
  {
    std::cout 
      << "Usage: " << path << " [OPTION]... [CONFIGURATION]" << std::endl
      << "Atom-IT is a lightweight, RESTful microservice for IoT applications." << std::endl
      << std::endl
      << "The \"CONFIGURATION\" argument is a single JSON configuration file." << std::endl
      << std::endl
      << "Command-line options:" << std::endl
      << "  --help\t\tdisplay this help and exit" << std::endl
      << "  --logdir=[dir]\tdirectory where to store the log files" << std::endl
      << "\t\t\t(by default, the log is dumped to stderr)" << std::endl
      << "  --logfile=[file]\tfile where to store the log of Atom-IT" << std::endl
      << "\t\t\t(by default, the log is dumped to stderr)" << std::endl
      << "  --verbose\t\tbe verbose in logs" << std::endl
      << "  --trace\t\thighest verbosity in logs (for debug)" << std::endl
      << "  --version\t\toutput version information and exit" << std::endl
      << std::endl
      << "Exit status:" << std::endl
      << "   0 if success," << std::endl
      << "  -1 if error (have a look at the logs)." << std::endl
      << std::endl;
  }
  
  void PrintVersion(const std::string& path)
  {
    std::cout
      << path << " " << ATOMIT_VERSION << std::endl
      << "Copyright (C) 2017 Sebastien Jodogne, WSL S.A. (Belgium)" << std::endl
      << "Licensing GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>, with OpenSSL exception." << std::endl
      << "This is free software: you are free to change and redistribute it." << std::endl
      << "There is NO WARRANTY, to the extent permitted by law." << std::endl
      << std::endl
      << "Written by Sebastien Jodogne <s.jodogne@gmail.com>" << std::endl;
  }
}


static bool DisplayPerformanceWarning()
{
  (void) DisplayPerformanceWarning;   // Disable warning about unused function
  LOG(WARNING) << "Performance warning: Non-release build, runtime debug assertions are turned on";
  return true;
}


int main(int argc, char* argv[]) 
{
  Orthanc::Logging::Initialize();
  Orthanc::HttpClient::GlobalInitialize();
  AtomIT::MQTT::SynchronousClient::GlobalInitialization(false);

  const char* configurationFile = NULL;


  /**
   * Parse the command-line options.
   **/ 

  for (int i = 1; i < argc; i++)
  {
    std::string argument(argv[i]); 

    if (argument.empty())
    {
      // Ignore empty arguments
    }
    else if (argument[0] != '-')
    {
      if (configurationFile != NULL)
      {
        LOG(ERROR) << "More than one configuration path were provided on the command line, aborting";
        return -1;
      }
      else
      {
        // Use the first argument that does not start with a "-" as
        // the configuration file

        // TODO WHAT IS THE ENCODING?
        configurationFile = argv[i];
      }
    }
    else if (argument == "--help")
    {
      AtomIT::PrintHelp(argv[0]);
      return 0;
    }
    else if (argument == "--version")
    {
      AtomIT::PrintVersion(argv[0]);
      return 0;
    }
    else if (argument == "--verbose")
    {
      Orthanc::Logging::EnableInfoLevel(true);
    }
    else if (argument == "--trace")
    {
      Orthanc::Logging::EnableTraceLevel(true);
    }
    else if (boost::starts_with(argument, "--logdir="))
    {
      // TODO WHAT IS THE ENCODING?
      std::string directory = argument.substr(9);

      try
      {
        Orthanc::Logging::SetTargetFolder(directory);
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "The directory where to store the log files (" 
                   << directory << ") is inexistent, aborting.";
        return -1;
      }
    }
    else if (boost::starts_with(argument, "--logfile="))
    {
      // TODO WHAT IS THE ENCODING?
      std::string file = argument.substr(10);

      try
      {
        Orthanc::Logging::SetTargetFile(file);
      }
      catch (Orthanc::OrthancException&)
      {
        LOG(ERROR) << "Cannot write to the specified log file (" 
                   << file << "), aborting.";
        return -1;
      }
    }
    else
    {
      LOG(WARNING) << "Option unsupported by the Atom-IT server: " << argument;
    }
  }


  /**
   * Launch Atom-IT server.
   **/

  {
    std::string version(ATOMIT_VERSION);

    if (std::string(ATOMIT_VERSION) == "mainline")
    {
      try
      {
        boost::filesystem::path exe(Orthanc::SystemToolbox::GetPathToExecutable());
        std::time_t creation = boost::filesystem::last_write_time(exe);
        boost::posix_time::ptime converted(boost::posix_time::from_time_t(creation));
        version += " (" + boost::posix_time::to_iso_string(converted) + ")";
      }
      catch (...)
      {
      }
    }

    LOG(WARNING) << "Atom-IT version: " << version;
    assert(DisplayPerformanceWarning());
  }

  int status = 0;
  try
  {
    for (;;)
    {
      AtomIT::GlobalInitialization(configurationFile);

      bool restart = AtomIT::StartServer(argc, argv, (configurationFile != NULL));
      if (restart)
      {
        AtomIT::GlobalFinalization();
        LOG(WARNING) << "Logging system is resetting";
        Orthanc::Logging::Reset();
      }
      else
      {
        break;
      }
    }
  }
  catch (const Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Uncaught exception, stopping now: ["
               << e.What() << "] (code " << e.GetErrorCode() << ")";
#if defined(_WIN32)
    if (e.GetErrorCode() >= Orthanc::ErrorCode_START_PLUGINS)
    {
      status = static_cast<int>(Orthanc::ErrorCode_Plugin);
    }
    else
    {
      status = static_cast<int>(e.GetErrorCode());
    }
#else
    status = -1;
#endif
  }
  catch (const std::exception& e) 
  {
    LOG(ERROR) << "Uncaught exception, stopping now: [" << e.what() << "]";
    status = -1;
  }
  catch (const std::string& s) 
  {
    LOG(ERROR) << "Uncaught exception, stopping now: [" << s << "]";
    status = -1;
  }
  catch (...)
  {
    LOG(ERROR) << "Native exception, stopping now. Check your plugins, if any.";
    status = -1;
  }

  AtomIT::GlobalFinalization();
  LOG(WARNING) << "The Atom-IT server has stopped";

  Orthanc::HttpClient::GlobalFinalize();
  Orthanc::Logging::Finalize();

  return status;
}
