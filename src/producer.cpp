/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Lijing Wang phdloli@ucla.edu 
 */

#include <ndn-cxx/contexts/producer-context.hpp>
#include "producer-callback.hpp"
#include <ndn-cxx/contexts/consumer-context.hpp>
#include "consumer-callback.hpp"
#include <ctime>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions

#define IDENTITY_NAME "/GOD/Confirm"  

class Signer
{
public:
  Signer()
  : m_identityName(IDENTITY_NAME)
  {
    m_keyChain.createIdentity(m_identityName);
  };
  
  void
  onPacket(Data& data)
  {
    m_keyChain.signByIdentity(data, m_identityName);
  }
  
private:
  KeyChain m_keyChain;
  Name m_identityName;
};

int
main(int argc, char** argv)
{

//  Name sensorPrefix("/example/data/3");
  std::string dataPrefix_str = "/ndn/edu/ucla/lijing/cpu/temp";
  Name sensorPrefix(dataPrefix_str);
    
  ProducerCallback proCB;

  Producer* dataProducer = new Producer(sensorPrefix);
    
  proCB.setProducer(dataProducer); // needed for some callback functionality
    
  //setting callbacks
  dataProducer->setContextOption(INTEREST_ENTER_CNTX,
                    (ConstInterestCallback)bind(&ProducerCallback::processIncomingInterest, &proCB, _1));
                      
  dataProducer->setContextOption(CACHE_MISS,
                    (ConstInterestCallback)bind(&ProducerCallback::processInterest, &proCB, _1));

  dataProducer->setContextOption(DATA_LEAVE_CNTX,
                    (ConstDataCallback)bind(&ProducerCallback::processOutgoingData, &proCB, _1));
    
  //listening
  dataProducer->attach();

  std::string repoPrefix_str = "/ndn/edu/ucla/lijing/repo";

  Name repoPrefix(repoPrefix_str);
      
  ConsumerCallback conCB;

  proCB.commandConsumer = new Consumer(repoPrefix, SDR);


  proCB.commandConsumer->setContextOption(MUST_BE_FRESH_S, true);
    
  proCB.commandConsumer->setContextOption(INTEREST_LEAVE_CNTX, 
                    (InterestCallback)bind(&ConsumerCallback::processLeavingInterest, &conCB, _1));
  
  proCB.commandConsumer->setContextOption(DATA_ENTER_CNTX, 
                    (DataCallback)bind(&ConsumerCallback::checkStatus, &conCB, _1));
  
  /* Producer of Control light command */
  Name commandPrefix("/ndn/edu/ucla/lijing/light");
   
  ProducerCallback cmdCB;

  Producer* cmdProducer = new Producer(commandPrefix);
    
  cmdCB.setProducer(cmdProducer); // needed for some callback functionality
    
  //setting callbacks
  Signer signer;

  cmdProducer->setContextOption(INTEREST_ENTER_CNTX,
                    (ConstInterestCallback)bind(&ProducerCallback::processIncomingInterest, &cmdCB, _1));
                      
  cmdProducer->setContextOption(CACHE_MISS,
                    (ConstInterestCallback)bind(&ProducerCallback::cacheMiss, &cmdCB, _1));

  cmdProducer->setContextOption(DATA_LEAVE_CNTX,
                    (ConstDataCallback)bind(&ProducerCallback::cmdOutgoingData, &cmdCB, _1));

  cmdProducer->setContextOption(DATA_TO_SECURE,
                    (DataCallback)bind(&Signer::onPacket, &signer, _1));

    
  //listening
  cmdProducer->attach();


  while(1)
  {

    CPUTemp cputemp; 
    cputemp.SMCOpen();
    double temp = cputemp.SMCGetTemperature(SMC_KEY_CPU_TEMP); 
    cputemp.SMCClose();
    std::string content = std::to_string(temp) ; //"RELIABLE ECHO z";


    time_t time_now = std::time(0);
    std::string time_now_str = std::ctime(&time_now);
    std::cout << "Now time: " << time_now_str << std::endl;

    uint64_t timestamp = toUnixTimestamp(ndn::time::system_clock::now()).count();
    std::string timestamp_str = std::to_string(timestamp);
    std::cout << "Timestamp: " << timestamp_str << std::endl;

    proCB.dataName = Name(dataPrefix_str).append(timestamp_str);

    dataProducer->produce(Name(timestamp_str), (uint8_t*)content.c_str(), content.size());

    printf("%0.1f°C\n",temp);
    printf("------------------------------------------------------------------- END\n\n\n");
      
    sleep(300); // because attach() is non-blocking
  }
  
  return 0;
}

} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::main(argc, argv);
}
