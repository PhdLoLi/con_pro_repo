/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014 Regents of the University of California.
 *
 * @author Lijing Wang phdloli@ucla.edu
 */

#include "consumer-callback.hpp"
#include <string>
#include <pthread.h>
#include <ctime>
#include "repo-command-response.hpp"

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions

//#define IDENTITY_NAME_IPHONE "/my/iPhone" 

class Signer
{
public:
  Signer()
//  : m_identityName(IDENTITY_NAME_IPHONE)
  {
//    m_keyChain.createIdentity(m_identityName);
  };
  
  void
  setIdentity(std::string identity)
  {
    std::cout << "self_identity " << identity << std::endl;
    m_identityName = Name(identity);
    m_keyChain.createIdentity(m_identityName);

  }
  void
  onPacket(Interest& interest)
  {
    m_keyChain.signByIdentity(interest, m_identityName);
  }
  
private:
  KeyChain m_keyChain;
  Name m_identityName;
};

  int times = 0;

  ConsumerCallback::ConsumerCallback()
  {
  }
  
  void
  ConsumerCallback::processPayload(const uint8_t* buffer, size_t bufferSize)
  {
//    std::cout << "times processPayload " << std::dec << times <<std::endl;
//    std::cout << "bufferSize " << bufferSize <<std::endl;
    std::cout << "Consumer DATA BEGIN: " << std::endl;
    std::cout << buffer <<std::endl;
    std::cout << "Consumer DATA OVER!! " << std::endl;
    times ++;

  }

  void
  ConsumerCallback::processData(Data& data)
  {
    std::cout << "DATA IN CNTX Name: " << data.getName()  << std::endl;
//    times ++;
    //<< "FinalBlockId: " <<data.getFinalBlockId() << std::endl;
  }

  void
  ConsumerCallback::checkStatus(Data& data)
  {
    repo::RepoCommandResponse response(data.getContent().blockFromValue());
    int statusCode = response.getStatusCode();
 //   std::cout << "DATA IN CNTX Name: " << data.getName()  << std::endl;
    std::cout << "Command Consumer --- StatusNumber: " << statusCode << std::endl;
    times ++;
    //<< "FinalBlockId: " <<data.getFinalBlockId() << std::endl;
  }
  
  bool
  ConsumerCallback::verifyData(Data& data)
  {
    return true;
  }
  
  void
  ConsumerCallback::processConstData(const Data& data)
  {}
  
  void
  ConsumerCallback::processLeavingInterest(Interest& interest)
  {
//    std::cout << "LEAVES " << interest.toUri() << std::endl;
    std::cout << "Command Consumer --- Leaving name: " << interest.toUri() << std::endl;
  }  

  void
  ConsumerCallback::signLeavingInterest(Interest& interest)
  {
    std::cout << "Before Signing Name " << interest.getName().toUri() << std::endl;
    Signer signer;
    signer.setIdentity(self_identity);
    signer.onPacket(interest);
    std::cout << "After Signing Name " << interest.getName().toUri() << std::endl;
  } 

} // namespace ndn
