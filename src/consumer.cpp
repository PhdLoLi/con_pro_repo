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

#include <ndn-cxx/contexts/consumer-context.hpp>
#include "consumer-callback.hpp"
#include <ndn-cxx/security/validator.hpp>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions
#define IDENTITY_NAME "/GOD/Confirm"  


  class Verificator
  {
  public:
    Verificator()
    {
      Name identity(IDENTITY_NAME);
      Name keyName = m_keyChain.getDefaultKeyNameForIdentity(identity);
      m_publicKey = m_keyChain.getPublicKey(keyName); 
    };
    
    bool
    onPacket(Data& data)
    {
    //  return true;
      std::cout << "Get Data and Verify it!!" << std::endl;
      if (Validator::verifySignature(data, *m_publicKey))
      {
        std::cout << "Data Verified!! Name:" << data.getName() <<  std::endl;
        return true;
      }
      else
      {
        std::cout << "Data NOT Verified!!" << data.getName() << std::endl;
        return false;
      }
    }
    
  private:
    KeyChain m_keyChain;
    shared_ptr<PublicKey> m_publicKey;
  };

int
main(int argc, char** argv)
{
//  Name sensorPrefix("/example/data/2/1421475525");

  std::string prefix = "/ndn/edu/ucla/lijing/cpu/temp";
  std::string self_identity = "/my/iPhone"; 

  Name sensorPrefix(prefix);
      
  ConsumerCallback conCB;

  Consumer* datagramConsumer = new Consumer(sensorPrefix, SDR);
  datagramConsumer->setContextOption(MUST_BE_FRESH_S, true);
  datagramConsumer->setContextOption(RIGHTMOST_CHILD_S, true);
    
  datagramConsumer->setContextOption(INTEREST_LEAVE_CNTX, 
                    (InterestCallback)bind(&ConsumerCallback::processLeavingInterest, &conCB, _1));
  
  datagramConsumer->setContextOption(DATA_ENTER_CNTX, 
                    (DataCallback)bind(&ConsumerCallback::processData, &conCB, _1));
  
  datagramConsumer->setContextOption(CONTENT_RETRIEVED, 
                    (ContentCallback)bind(&ConsumerCallback::processPayload, &conCB, _1, _2));
  
//  if (argc > 1)
//    datagramConsumer->consume(Name(argv[1]));
//  else
    datagramConsumer->consume(Name(""));

 
  std::string controlprefix = "/ndn/edu/ucla/lijing/light/iphone";

  Name controlPrefix(controlprefix);
  Verificator* verificator = new Verificator();
  ConsumerCallback controlCB;

  Consumer* controlConsumer = new Consumer(controlPrefix, SDR);

  controlConsumer->setContextOption(MUST_BE_FRESH_S, true);
    
  controlConsumer->setContextOption(INTEREST_LEAVE_CNTX, 
                    (InterestCallback)bind(&ConsumerCallback::signLeavingInterest, &controlCB, _1));
  
//  controlConsumer->setContextOption(DATA_ENTER_CNTX, 
//                    (DataCallback)bind(&ConsumerCallback::processData, &controlCB, _1));
  
  controlConsumer->setContextOption(CONTENT_RETRIEVED, 
                    (ContentCallback)bind(&ConsumerCallback::processPayload, &controlCB, _1, _2));

  controlConsumer->setContextOption(DATA_TO_VERIFY,
                    (DataVerificationCallback)bind(&Verificator::onPacket, verificator, _1));
  
  std::string command;
  if(argc == 3)
  {
    self_identity = argv[2];
    command = argv[1];
  }
  else if(argc == 2)
  {
    command = argv[1];
  }
  else{

    command = "on";
  }

  controlCB.self_identity = self_identity;
  controlConsumer->consume(Name(command));

  return 0;
}

} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::main(argc, argv);
}
