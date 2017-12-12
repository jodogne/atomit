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


#include <MQTTClient.h>

extern "C"
{
#include <Log.h>
#include <Socket.h>
}

#include "SynchronousClient.h"

#if defined(_WIN32) || defined(WIN64)
#include <windows.h>

extern "C" BOOL APIENTRY DllMain(HANDLE hModule,
                                 DWORD  ul_reason_for_call,
                                 LPVOID lpReserved);

#else
extern "C" void MQTTClient_init();

#endif


#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace AtomIT
{
  namespace MQTT
  {
    class SynchronousClient::StringHolder : public boost::noncopyable
    {
    private:
      char* value_;

    public:
      StringHolder() : value_(NULL)
      {
      }

      char** operator&()
      {
        if (value_ == NULL)
        {
          return &value_;
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }

      void Copy(std::string& target)
      {
        if (value_ == NULL)
        {
          target.clear();
        }
        else
        {
          target.assign(value_);
        }
      }

      void Copy(std::string& target,
                unsigned int size)
      {
        if (value_ == NULL ||
            size == 0)
        {
          target.clear();
        }
        else
        {
          target.assign(value_, size);
        }
      }

      ~StringHolder()
      {
        if (value_ != NULL)
        {
          MQTTClient_free(value_);
        }
      }
    };

    
    class SynchronousClient::ReceivedMessageHolder : public boost::noncopyable
    {
    private:
      MQTTClient_message* value_;
    
    public:
      ReceivedMessageHolder() : value_(NULL)
      {
      }

      bool IsNull() const
      {
        return value_ == NULL;
      }

      MQTTClient_message** operator&()
      {
        if (value_ == NULL)
        {
          return &value_;
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }

      void Copy(std::string& target)
      {
        if (value_ == NULL ||
            value_->payloadlen == 0)
        {
          target.clear();
        }
        else
        {
          target.assign(reinterpret_cast<const char*>(value_->payload), value_->payloadlen);
        }
      }

      ~ReceivedMessageHolder()
      {
        if (value_ != NULL)
        {
          MQTTClient_freeMessage(&value_);
          value_ = NULL;
        }
      }
    };


    class SynchronousClient::PImpl
    {
    private:
      static void UpdateReferenceCounter(int offset)
      {
        static boost::mutex  mutex_;

        {
          boost::mutex::scoped_lock lock(mutex_);

          static int counter_ = 0;
          counter_ += offset;

          if (counter_ < 0)
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
          else if (counter_ > 1)
          {
            LOG(WARNING) << "More than one MQTT client is active at the same time ("
                         << counter_ << "), which is not well supported by Paho "
                         << "and might lead to instabilities";
          }
        }
      }
      
    public:
      MQTTClient   client_;

      PImpl() :
        client_(NULL)
      {
        UpdateReferenceCounter(1);
      }

      ~PImpl()
      {
        UpdateReferenceCounter(-1);
      }
    };


    static void PahoLogCallback(enum LOG_LEVELS level, const char *message)
    {
      if (level == LOG_ERROR ||
          level == LOG_SEVERE ||
          level == LOG_FATAL)
      {
        LOG(INFO) << "MQTT: " << message;
      }
      else
      {
        VLOG(0) << "MQTT: " << message;
      }
    }


    void SynchronousClient::GlobalInitialization(bool useSsl)
    {
#if defined(_WIN32)
      // Static library doesn't work (visual studio)
      // https://github.com/eclipse/paho.mqtt.c/issues/263
      DllMain(NULL, DLL_PROCESS_ATTACH, NULL);
#else
      MQTTClient_init();
#endif

      Log_setTraceCallback(PahoLogCallback);
    
      MQTTClient_init_options options;
      memset(&options, 0, sizeof(options));

      options.struct_id[0] = 'M';
      options.struct_id[1] = 'Q';
      options.struct_id[2] = 'T';
      options.struct_id[3] = 'G';
      options.struct_version = 0;
      options.do_openssl_init = (useSsl ? 1 : 0);

      MQTTClient_global_init(&options);
    }

    
    SynchronousClient::SynchronousClient()
    {
      pimpl_ = new PImpl;
      if (pimpl_ == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
    

    SynchronousClient::~SynchronousClient()
    {
      Disconnect();

      assert(pimpl_ != NULL);
      delete pimpl_;
    }


    bool SynchronousClient::IsConnected() const
    {
      return pimpl_->client_ != NULL;
    }

    
    void SynchronousClient::Connect(const Broker& broker,
                                    const std::string& clientId)
    {
      if (IsConnected())
      {
        // Cannot connect twice
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      MQTTClient_connectOptions options;
      memset(&options, 0, sizeof(options));

      options.struct_id[0] = 'M';
      options.struct_id[1] = 'Q';
      options.struct_id[2] = 'T';
      options.struct_id[3] = 'C';
      options.struct_version = 5;
      options.keepAliveInterval = 20;   // Default: 20 seconds
      options.cleansession = 1;
      options.reliable = 1;
      options.will = NULL;
      options.username = broker.HasCredentials() ? broker.GetUsername().c_str() : NULL;
      options.password = broker.HasCredentials() ? broker.GetPassword().c_str() : NULL;
      options.connectTimeout = 5;  // Default: 5 seconds
      options.retryInterval = 1;
      options.ssl = NULL;
      options.serverURIcount = 0;
      options.serverURIs = NULL;
      options.MQTTVersion = MQTTVERSION_3_1;

      if (MQTTClient_create(&pimpl_->client_,
                            broker.GetServer().c_str(),
                            clientId.c_str(),
                            MQTTCLIENT_PERSISTENCE_NONE,   // TODO
                            NULL) != MQTTCLIENT_SUCCESS ||
          pimpl_->client_ == NULL)
      {
        pimpl_->client_ = NULL;

        LOG(INFO) << "Cannot create a MQTT connection";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      int error = MQTTClient_connect(pimpl_->client_, &options);
      if (error != MQTTCLIENT_SUCCESS)
      {
        MQTTClient_destroy(&pimpl_->client_);
        pimpl_->client_ = NULL;

        LOG(INFO) << "Cannot connect to MQTT broker " << broker.GetServer()
                  << ", check out the network and credentials";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      LOG(WARNING) << "Connected to MQTT broker " << broker.GetServer();
    }
    

    void SynchronousClient::Disconnect()
    {
      if (IsConnected())
      {
        if (MQTTClient_disconnect(pimpl_->client_, 5 /* timeout in seconds */) != MQTTCLIENT_SUCCESS)
        {
          LOG(ERROR) << "Cannot cleanly disconnect from the MQTT server";
        }

        MQTTClient_destroy(&pimpl_->client_);
        pimpl_->client_ = NULL;
      }
    }

    void SynchronousClient::Subscribe(const std::vector<std::string>& topics)
    {
      if (!IsConnected())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      if (topics.empty())
      {
        LOG(WARNING) << "You have not subscribed to any MQTT topic";
      }
      else
      {
        // In this implementation, we use QoS0, aka. "At most once",
        // aka. "Fire and forget"
        std::vector<int> qos(topics.size(), 0);
    
        std::vector<char*> tmp(topics.size());
        for (size_t i = 0; i < topics.size(); i++)
        {
          tmp[i] = const_cast<char*>(topics[i].c_str());
        }

        if (MQTTClient_subscribeMany(pimpl_->client_, tmp.size(),
                                     tmp.empty() ? NULL : &tmp[0],
                                     qos.empty() ? NULL : &qos[0]) != MQTTCLIENT_SUCCESS)
        {
          LOG(INFO) << "Cannot subscribe to topics against the MQTT broker";
          Disconnect();
        }
      }
    }
    

    bool SynchronousClient::Receive(std::string& topic,
                                    std::string& message,
                                    unsigned int timeout /* in milliseconds */)
    {
      if (!IsConnected())
      {
        return false;
      }

      ReceivedMessageHolder payload;
      StringHolder topicName;
      int topicLength = 0;

      int code = MQTTClient_receive(pimpl_->client_, &topicName, &topicLength, &payload, timeout);

      switch (code)
      {
        case MQTTCLIENT_SUCCESS:
        case MQTTCLIENT_TOPICNAME_TRUNCATED:
        {
          if (payload.IsNull())
          {
            // This is a timeout, ignore the event
            return false;
          }
          else
          {
            if (code == MQTTCLIENT_TOPICNAME_TRUNCATED)
            {
              topicName.Copy(topic, topicLength);
            }
            else
            {
              topicName.Copy(topic);
            }

            payload.Copy(message);
            return true;
          }
        }

        case MQTTCLIENT_DISCONNECTED:
          LOG(ERROR) << "The MQTT client has been disconnected";
          MQTTClient_destroy(&pimpl_->client_);
          pimpl_->client_ = NULL;
          break;

        default:
          LOG(ERROR) << "Unhandled error code in MQTT: " << code;
          break;
      }

      return false;
    }

    
    bool SynchronousClient::Publish(const std::string& topic,
                                    const std::string& message,
                                    unsigned int timeout /* in milliseconds */)
    {
      if (IsConnected())
      {
        int code = MQTTClient_publish(pimpl_->client_, topic.c_str(),
                                      message.size(), const_cast<char*>(message.c_str()),
                                      0 /* use QOS 0 */,
                                      0 /* no use of retained messages */,
                                      NULL /* no use of delivery tokens, as QOS 0 */);

        return code == MQTTCLIENT_SUCCESS;
      }
      else
      {
        return false;
      }
    }


    void SynchronousClient::PublishPending()
    {
      MQTTClient_yield();
    }
  }
}
