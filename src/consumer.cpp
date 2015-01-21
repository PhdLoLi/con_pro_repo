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

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespace could be used to prevent/limit name contentions

int
main(int argc, char** argv)
{
//  Name sensorPrefix("/example/data/2/1421475525");

  std::string prefix = "/ndn/edu/ucla/lijing/cpu/temp";

  Name sensorPrefix(prefix);
      
  ConsumerCallback conCB;

  Consumer* datagramConsumer = new Consumer(sensorPrefix, RELIABLE, DATAGRAM);
  datagramConsumer->setContextOption(MUST_BE_FRESH_S, true);
    
  datagramConsumer->setContextOption(INTEREST_LEAVE_CNTX, 
                    (InterestCallback)bind(&ConsumerCallback::processLeavingInterest, &conCB, _1));
  
  datagramConsumer->setContextOption(DATA_ENTER_CNTX, 
                    (DataCallback)bind(&ConsumerCallback::processData, &conCB, _1));
  
  datagramConsumer->setContextOption(CONTENT_RETRIEVED, 
                    (ContentCallback)bind(&ConsumerCallback::processPayload, &conCB, _1, _2));
  
  if (argc > 1)
    datagramConsumer->consume(Name(argv[1]));
  else
    datagramConsumer->consume(Name(""));
//  datagramConsumer->consume(Name("zzz"));

  return 0;
}

} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::main(argc, argv);
}
