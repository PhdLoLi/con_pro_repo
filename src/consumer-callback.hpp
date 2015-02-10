/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Regents of the University of California.
 *
 * @author Lijing Wang <phdloli@ucla.edu>
 */


#ifndef CONSUMER_CALLBACK_HPP
#define CONSUMER_CALLBACK_HPP
#include <ndn-cxx/contexts/consumer-context.hpp>
#include <fstream>
#include <iostream>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions

  class ConsumerCallback
  {
      
  public:
    ConsumerCallback();
    
    void
    processPayload(const uint8_t* buffer, size_t bufferSize);
    
    void
    checkStatus(Data& data);

    void
    processData(Data& data);
    
    bool
    verifyData(Data& data);
    
    void
    processConstData(const Data& data);
    
    void
    processLeavingInterest(Interest& interest);

    void
    signLeavingInterest(Interest& interest);
    
    void
    onRetx(Interest& interest)
    {
      std::cout << "Retransmitted " << interest.getName() << std::endl;
    }
    
    std::string self_identity; 
  };

} // namespace ndn
#endif
