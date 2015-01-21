/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California.
 *
 * This file is part of NDN repo-ng (Next generation of NDN repository).
 * See AUTHORS.md for complete list of repo-ng authors and contributors.
 *
 * repo-ng is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  if (not, see <http://www.gnu.org/licenses/>.
 */

#include "insert-repo.hpp"

namespace repo {

void
InsertRepo::prepareNextData(uint64_t referenceSegmentNo)
{
  // make sure m_data has [referenceSegmentNo, referenceSegmentNo + PRE_SIGN_DATA_COUNT] Data
  if (m_isFinished)
    return;

  size_t nDataToPrepare = PRE_SIGN_DATA_COUNT;

  if (!m_data.empty()) {
    uint64_t maxSegmentNo = m_data.rbegin()->first;

    if (maxSegmentNo - referenceSegmentNo >= nDataToPrepare) {
      // nothing to prepare
      return;
    }

    nDataToPrepare -= maxSegmentNo - referenceSegmentNo;
  }

  for (size_t i = 0; i < nDataToPrepare && !m_isFinished; ++i) {
    uint8_t buffer[DEFAULT_BLOCK_SIZE];

    std::streamsize readSize =
      boost::iostreams::read(*insertStream, reinterpret_cast<char*>(buffer), DEFAULT_BLOCK_SIZE);

    if (readSize <= 0) {
      throw Error("Error reading from the input stream");
    }

    shared_ptr<ndn::Data> data =
      make_shared<ndn::Data>(Name(m_dataPrefix)
                                    .appendSegment(m_currentSegmentNo));

    if (insertStream->peek() == std::istream::traits_type::eof()) {
      data->setFinalBlockId(ndn::name::Component::fromSegment(m_currentSegmentNo));
      m_isFinished = true;
    }

    data->setContent(buffer, readSize);
    data->setFreshnessPeriod(freshnessPeriod);
    signData(*data);

    m_data.insert(std::make_pair(m_currentSegmentNo, data));

    ++m_currentSegmentNo;
  }
}

void
InsertRepo::run()
{
  m_dataPrefix = ndnName;
  if (!isUnversioned)
    m_dataPrefix.appendVersion(m_timestampVersion);

  if (isVerbose)
    std::cerr << "setInterestFilter for " << m_dataPrefix << std::endl;
  m_face.setInterestFilter(m_dataPrefix,
                           isSingle ?
                           bind(&InsertRepo::onSingleInterest, this, _1, _2)
                           :
                           bind(&InsertRepo::onInterest, this, _1, _2),
                           bind(&InsertRepo::onRegisterSuccess, this, _1),
                           bind(&InsertRepo::onRegisterFailed, this, _1, _2));


  if (hasTimeout)
    m_scheduler.scheduleEvent(timeout, bind(&InsertRepo::stopProcess, this));

  m_face.processEvents();
}

void
InsertRepo::onRegisterSuccess(const Name& prefix)
{
  startInsertCommand();
}

void
InsertRepo::startInsertCommand()
{
  RepoCommandParameter parameters;
  parameters.setName(m_dataPrefix);
  if (!isSingle) {
    parameters.setStartBlockId(0);
  }

  ndn::Interest commandInterest = generateCommandInterest(repoPrefix, "insert", parameters);

  std::cout << "Whole Interest Name" << commandInterest.getName() << std::endl;

  m_face.expressInterest(commandInterest,
                         bind(&InsertRepo::onInsertCommandResponse, this, _1, _2),
                         bind(&InsertRepo::onInsertCommandTimeout, this, _1));
}

void
InsertRepo::onInsertCommandResponse(const ndn::Interest& interest, ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  int statusCode = response.getStatusCode();
  if (statusCode >= 400) {
    throw Error("insert command failed with code " +
                boost::lexical_cast<std::string>(statusCode));
  }
  m_processId = response.getProcessId();

  m_scheduler.scheduleEvent(m_checkPeriod,
                            bind(&InsertRepo::startCheckCommand, this));
}

void
InsertRepo::onInsertCommandTimeout(const ndn::Interest& interest)
{
  throw Error("command response timeout");
}

void
InsertRepo::onInterest(const ndn::Name& prefix, const ndn::Interest& interest)
{
  if (interest.getName().size() != prefix.size() + 1) {
    if (isVerbose) {
      std::cerr << "Error processing incoming interest " << interest << ": "
                << "Unrecognized Interest" << std::endl;
    }
    return;
  }

  uint64_t segmentNo;
  try {
    ndn::Name::Component segmentComponent = interest.getName().get(prefix.size());
    segmentNo = segmentComponent.toSegment();
  }
  catch (tlv::Error& e) {
    if (isVerbose) {
      std::cerr << "Error processing incoming interest " << interest << ": "
                << e.what() << std::endl;
    }
    return;
  }

  prepareNextData(segmentNo);

  DataContainer::iterator item = m_data.find(segmentNo);
  if (item == m_data.end()) {
    if (isVerbose) {
      std::cerr << "Requested segment [" << segmentNo << "] does not exist" << std::endl;
    }
    return;
  }

  if (m_isFinished) {
    uint64_t final = m_currentSegmentNo - 1;
    item->second->setFinalBlockId(ndn::name::Component::fromSegment(final));

  }
  m_face.put(*item->second);
}

void
InsertRepo::onSingleInterest(const ndn::Name& prefix, const ndn::Interest& interest)
{
  BOOST_ASSERT(prefix == m_dataPrefix);

  if (prefix != interest.getName()) {
    if (isVerbose) {
      std::cerr << "Received unexpected interest " << interest << std::endl;
    }
    return;
  }

  uint8_t buffer[DEFAULT_BLOCK_SIZE];
  std::streamsize readSize =
    boost::iostreams::read(*insertStream, reinterpret_cast<char*>(buffer), DEFAULT_BLOCK_SIZE);

  if (readSize <= 0) {
    throw Error("Error reading from the input stream");
  }

  if (insertStream->peek() != std::istream::traits_type::eof()) {
    throw Error("Input data does not fit into one Data packet");
  }

  shared_ptr<ndn::Data> data = make_shared<ndn::Data>(m_dataPrefix);
  data->setContent(buffer, readSize);
  data->setFreshnessPeriod(freshnessPeriod);
  signData(*data);
  m_face.put(*data);

  m_isFinished = true;
}

void
InsertRepo::onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
{
  throw Error("onRegisterFailed: " + reason);
}

void
InsertRepo::stopProcess()
{
  m_face.getIoService().stop();
}

void
InsertRepo::signData(ndn::Data& data)
{
  if (useDigestSha256) {
    m_keyChain.signWithSha256(data);
  }
  else {
    if (identityForData.empty())
      m_keyChain.sign(data);
    else {
      ndn::Name keyName = m_keyChain.getDefaultKeyNameForIdentity(ndn::Name(identityForData));
      ndn::Name certName = m_keyChain.getDefaultCertificateNameForKey(keyName);
      m_keyChain.sign(data, certName);
    }
  }
}

void
InsertRepo::startCheckCommand()
{
  ndn::Interest checkInterest = generateCommandInterest(repoPrefix, "insert check",
                                                        RepoCommandParameter()
                                                          .setProcessId(m_processId));
  m_face.expressInterest(checkInterest,
                         bind(&InsertRepo::onCheckCommandResponse, this, _1, _2),
                         bind(&InsertRepo::onCheckCommandTimeout, this, _1));
}

void
InsertRepo::onCheckCommandResponse(const ndn::Interest& interest, ndn::Data& data)
{
  RepoCommandResponse response(data.getContent().blockFromValue());
  int statusCode = response.getStatusCode();
  if (statusCode >= 400) {
    throw Error("Insert check command failed with code: " +
                boost::lexical_cast<std::string>(statusCode));
  }

  if (m_isFinished) {
    uint64_t insertCount = response.getInsertNum();

    if (isSingle) {
      if (insertCount == 1) {
        m_face.getIoService().stop();
        return;
      }
    }
    // Technically, the check should not infer, but directly has signal from repo that
    // write operation has been finished

    if (insertCount == m_currentSegmentNo) {
      m_face.getIoService().stop();
      return;
    }
  }

  m_scheduler.scheduleEvent(m_checkPeriod,
                            bind(&InsertRepo::startCheckCommand, this));
}

void
InsertRepo::onCheckCommandTimeout(const ndn::Interest& interest)
{
  throw Error("check response timeout");
}

ndn::Interest
InsertRepo::generateCommandInterest(const ndn::Name& commandPrefix, const std::string& command,
                                    const RepoCommandParameter& commandParameter)
{
  ndn::Interest interest(ndn::Name(commandPrefix)
                         .append(command)
                         .append(commandParameter.wireEncode()));
  interest.setInterestLifetime(interestLifetime);

  if (identityForCommand.empty())
    m_generator.generate(interest);
  else {
    m_generator.generateWithIdentity(interest, ndn::Name(identityForCommand));
  }

  return interest;
}

static void
usage()
{
  fprintf(stderr,
          "ndnputfile [-u] [-s] [-D] [-d] [-i identity] [-I identity]"
          "  [-x freshness] [-l lifetime] [-w timeout] repo-prefix ndn-name filename\n"
          "\n"
          " Write a file into a repo.\n"
          "  -u: unversioned: do not add a version component\n"
          "  -s: single: do not add version or segment component, implies -u\n"
          "  -D: use DigestSha256 signing method instead of SignatureSha256WithRsa\n"
          "  -i: specify identity used for signing Data\n"
          "  -I: specify identity used for signing commands\n"
          "  -x: FreshnessPeriod in milliseconds\n"
          "  -l: InterestLifetime in milliseconds for each command\n"
          "  -w: timeout in milliseconds for whole process (default unlimited)\n"
          "  -v: be verbose\n"
          "  repo-prefix: repo command prefix\n"
          "  ndn-name: NDN Name prefix for written Data\n"
          "  filename: local file name; \"-\" reads from stdin\n"
          );
  exit(1);
}

int
main(int argc, char** argv)
{
  InsertRepo insertRepo;
  int opt;
  while ((opt = getopt(argc, argv, "usDi:I:x:l:w:vh")) != -1) {
    switch (opt) {
    case 'u':
      insertRepo.isUnversioned = true;
      break;
    case 's':
      insertRepo.isSingle = true;
      break;
    case 'D':
      insertRepo.useDigestSha256 = true;
      break;
    case 'i':
      insertRepo.identityForData = std::string(optarg);
      break;
    case 'I':
      insertRepo.identityForCommand = std::string(optarg);
      break;
    case 'x':
      try {
        insertRepo.freshnessPeriod = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-x option should be an integer.";
        return 1;
      }
      break;
    case 'l':
      try {
        insertRepo.interestLifetime = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-l option should be an integer.";
        return 1;
      }
      break;
    case 'w':
      insertRepo.hasTimeout = true;
      try {
        insertRepo.timeout = milliseconds(boost::lexical_cast<uint64_t>(optarg));
      }
      catch (boost::bad_lexical_cast&) {
        std::cerr << "-w option should be an integer.";
        return 1;
      }
      break;
    case 'v':
      insertRepo.isVerbose = true;
      break;
    case 'h':
      usage();
      break;
    default:
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 3)
    usage();

  insertRepo.repoPrefix = Name(argv[0]);
  insertRepo.ndnName = Name(argv[1]);
  if (strcmp(argv[2], "-") == 0) {

    insertRepo.insertStream = &std::cin;
    insertRepo.run();
  }
  else {
    std::ifstream inputFileStream(argv[2], std::ios::in | std::ios::binary);
    if (!inputFileStream.is_open()) {
      std::cerr << "ERROR: cannot open " << argv[2] << std::endl;
      return 1;
    }

    insertRepo.insertStream = &inputFileStream;
    insertRepo.run();
  }

  // insertRepo MUST NOT be used anymore because .insertStream is a dangling pointer

  return 0;
}

} // namespace repo

int
main(int argc, char** argv)
{
  try {
    return repo::main(argc, argv);
  }
  catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }
}
