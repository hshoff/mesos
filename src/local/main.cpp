/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <string>

#include <stout/os.hpp>
#include <stout/stringify.hpp>

#include "detector/detector.hpp"

#include "local/flags.hpp"
#include "local/local.hpp"

#include "master/master.hpp"

#include "slave/slave.hpp"

#include "logging/logging.hpp"

using namespace mesos::internal;

using mesos::internal::master::Master;
using mesos::internal::slave::Slave;

using std::cerr;
using std::endl;
using std::string;


void usage(const char* argv0, const flags::FlagsBase& flags)
{
  cerr << "Usage: " << os::basename(argv0).get() << " [...]" << endl
       << endl
       << "Launches an in-memory cluster within a single process."
       << endl
       << "Supported options:" << endl
       << flags.usage();
}


int main(int argc, char **argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // TODO(benh): Inherit from both slave::Flags and master::Flags in
  // order to pass those flags on to the master. Alternatively, add a
  // way to load flags and ignore unknowns in order to load
  // master::flags, then slave::Flags, then local::Flags.
  local::Flags flags;

  // The following flags are executable specific (e.g., since we only
  // have one instance of libprocess per execution, we only want to
  // advertise the port and ip option once, here).
  uint16_t port;
  flags.add(&port, "port", "Port to listen on", 5050);

  Option<string> ip;
  flags.add(&ip, "ip", "IP address to listen on");

  bool help;
  flags.add(&help,
            "help",
            "Prints this help message",
            false);

  // Load flags from environment and command line but allow unknown
  // flags since we might have some master/slave flags as well.
  Try<Nothing> load = flags.load("MESOS_", argc, argv, true);

  if (load.isError()) {
    cerr << load.error() << endl;
    usage(argv[0], flags);
    exit(1);
  }

  if (help) {
    usage(argv[0], flags);
    exit(1);
  }

  // Initialize libprocess.
  os::setenv("LIBPROCESS_PORT", stringify(port));

  if (ip.isSome()) {
    os::setenv("LIBPROCESS_IP", ip.get());
  }

  process::initialize("master");

  logging::initialize(argv[0], flags);

  process::wait(local::launch(flags));

  return 0;
}
