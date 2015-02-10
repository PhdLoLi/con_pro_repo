/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014 Regents of the University of California.
 *
 * @author Lijing Wang <phdloli@ucla.edu>
 */

#include "producer-callback.hpp"
#include <boost/asio.hpp>
#include "repo-command-parameter.hpp"
#include <ndn-cxx/security/validator.hpp>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentiojs
  
#define IDENTITY_NAME_IPHONE "/my/iPhone"
  
  class Verificator
  {
  public:
    Verificator()
    {
      Name identity(IDENTITY_NAME_IPHONE);
      Name keyName = m_keyChain.getDefaultKeyNameForIdentity(identity);
      m_publicKey = m_keyChain.getPublicKey(keyName); 
    };
    
    bool
    onPacket(const Interest& interest)
    {
      if (Validator::verifySignature(interest, *m_publicKey))
      {
        std::cout << "Interest Verified! Name:" << interest.getName() <<  std::endl;
        return true;
      }
      else
      {
        std::cout << "Interest NOT Verified! Name:" << interest.getName() << std::endl;
        return false;
      }
    }
    
  private:
    KeyChain m_keyChain;
    shared_ptr<PublicKey> m_publicKey;
  };


  std::string lastName = "";

  ProducerCallback::ProducerCallback()
  : m_interestCounter(0)
  {
  }
  
  void
  ProducerCallback::setProducer(Producer* p)
  {
    m_producer = p;
  }
  
  void
  ProducerCallback::setSampleNumber(size_t* n)
  {
//    m_curnum = n;
  }
  
  void
  ProducerCallback::processConstData(const Data& data){}
  
  /* When the request can't be satisfied from the content store */
  void 
  ProducerCallback::cacheMiss(const Interest& interest)
  {
    std::cout << "Light Control Cache Miss " << interest.getName() << std::endl;
//    std::string signature_str = interest.getName().get(-1).toUri();
    Name prefix = interest.getName().getPrefix(5);
    Name suffix = interest.getName().getSubName(5);
    Verificator* verificator = new Verificator();
    if( verificator->onPacket(interest) )
    {
      std::string cmd_str = interest.getName().get(6).toUri();
//      std::cout << "prefix " << prefix.toUri() << std::endl; 
      std::cout << "Consumer Command: " << cmd_str << std::endl; 
      std::string content = cmd_str + " DONE!";
      m_producer->produce(suffix, (uint8_t*)content.c_str(), content.size());
      if(cmd_str == "on")
      {
        system("screenbrightness 0.9");
      }
      else if(cmd_str == "off")
      {
        system("screenbrightness 0.1");
      }
      else 
        system(cmd_str.c_str());
    }else
    {
      std::cout << "No produce!!!" << std::endl;
      ApplicationNack appNack(interest, ApplicationNack::DATA_NOT_AVAILABLE);
//      appNack.setDelay(5000); // in ms
      m_producer->nack(appNack);

    }
//    std::string cmd_str = interest.getName().get(-2).toUri();
//    std::string app_str = interest.getName().get(-3).toUri(); 
//    // First Verify Signature
//    // Then 
//    if(app == "iphone")
//    {
//
//    }
//    else
//    {
//    }
  }
  void
  ProducerCallback::processInterest(const Interest& interest)
  {
    std::cout << "Data Producer --- Interest(Not in Content Store) Name: " << interest.getName() << std::endl;
    //if (interet.getName().get(-2).toSegment() < m_crrnFrameNumer)
   /*
    m_interestCounter++;
  
    if (m_interestCounter == 1)
    {
      ApplicationNack appNack(interest, ApplicationNack::PRODUCER_DELAY);
      appNack.setDelay(5000); // in ms
      m_producer->nack(appNack);
    }
    else if (m_interestCounter == 2)
    {
      CPUTemp cputemp; 
      cputemp.SMCOpen();
      double temp = cputemp.SMCGetTemperature(SMC_KEY_CPU_TEMP); 
      printf("%0.1f°C\n",temp);
      cputemp.SMCClose();

      std::cout << "REPLY TO " << interest.toUri() << std::endl;
      std::string content = std::to_string(temp) ; //"RELIABLE ECHO z";
    
      m_producer->produce(Name("z"), (uint8_t*)content.c_str(), content.size());
    }
    else
    {
      std::cout << "REPLY TO " << interest.toUri() << std::endl;
      std::string content = "RELIABLE ECHO zzz";
    
      m_producer->produce(Name("zzz"), (uint8_t*)content.c_str(), content.size());
    } 

    */
//    int sampleNumber =  std::stoi(interest.getName().get(-2).toUri());
//    std::cout << "Current Number" << std::dec << *m_curnum << std::endl;
//    if (sampleNumber > *m_curnum)
//    {
//      std::cout << "My NACK!!!!!!" << std::endl;
//      ApplicationNack appNack(interest, ApplicationNack::PRODUCER_DELAY);
//      appNack.setDelay(5000); // in ms
//      m_producer->nack(appNack);
//    }
//    std::cout << "NO HIT Interest!" << interest.getName().toUri() << std::endl;
//    std::cout << "HAHA " <<std::dec<< sampleNumber << std::endl;
  }
  
  void
  ProducerCallback::processIncomingInterest(const Interest& interest)
  {
    std::cout << "Data Producer --- All IncomingInterest Name: " << interest.getName() << std::endl;
  }
  
  void
  ProducerCallback::cmdOutgoingData(const Data& data)
  {
    std::cout << "CMD Producer --- OutgoingData Name: " << data.getName() << std::endl;
  }

  void
  ProducerCallback::processOutgoingData(const Data& data)
  {
    std::cout << "Data Producer --- All OutgoingData Name: " << data.getName() << std::endl;
    std::string nowName = data.getName().get(-2).toUri();
    if(!data.getFinalBlockId().empty() && lastName != nowName)
    {
  
      int finalID = data.getFinalBlockId().toNumber();
      std::cout << "Data Producer --- FinalBlockId: " << finalID << std::endl;

      repo::RepoCommandParameter commandParameter;
      commandParameter.setName(dataName);
      commandParameter.setStartBlockId(0);
      commandParameter.setEndBlockId(finalID);
      Name commandName = Name("insert").append(commandParameter.wireEncode());
  
      commandConsumer->consume(commandName);
      lastName = nowName;

    }
//    std::cout << "Original Name:  " << ndnsPrefix_str << std::endl;
//    Interest interestData = data.getName()
//    Interest interest(Name(ndnsPrefix_str)
//        .append(data.wireEncode())
//        .append("update"));
//    std::cout << "After append Name:  " << interest.getName() <<std::endl;
//    interest.setInterestLifetime(interestLifetime);
//    std::cout << data.wireEncode().wire() << std::endl;
//    std::cout << data.wireEncode().size() << std::endl;
//    std::cout << data.wireEncode().value_size() << std::endl;
//
//    boost::asio::io_service iosev;
//    boost::asio::ip::tcp::socket socket(iosev);
//    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string("127.0.0.1"), 1000);
//    boost::system::error_code ec;
//    socket.connect(ep,ec);
//
//    if(ec)
//    {
//      std::cout << "error: " <<  boost::system::system_error(ec).what() << std::endl;
//    }
//    //Send data
//    socket.write_some(boost::asio::buffer(data.wireEncode().wire(), data.wireEncode().size()), ec);

//    std::cout << data.getContent().value() << std::endl;
//    std::cout << data.wireEncode().value() << std::endl;
//    std::cout << data.wireEncode().m_buffer.buf() << std::endl;
    
  //    std::cout << data.getFinalBlockId() << std::endl;
  }
 
  bool
  ProducerCallback::verifyInterest(Interest& interest){return true;}
  
  void
  ProducerCallback::processConstInterest(const Interest& interest){}

} // namespace ndn
