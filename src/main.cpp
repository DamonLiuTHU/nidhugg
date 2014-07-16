/* Copyright (C) 2014 Carl Leonardsson
 *
 * This file is part of Nidhugg.
 *
 * Nidhugg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nidhugg is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "Configuration.h"
#include "DPORDriver.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ManagedStatic.h>

#include <iostream>
#include <set>
#include <stdexcept>

void print_version(){
  std::cout << PACKAGE_STRING
            << " ("
#ifdef GIT_COMMIT
            << GIT_COMMIT << ", "
#endif
#ifndef NDEBUG
            << "Debug"
#else
            << "Release"
#endif
            << ", with LLVM-" << LLVM_VERSION << ":" << LLVM_BUILDMODE << ")\n";
};

int main(int argc, char *argv[]){
  /* Command line options */
  llvm::cl::SetVersionPrinter(print_version);
  {
    /* Hide all options defined by the LLVM library, except the ones
     * approved by Configuration.
     */
    std::set<std::string> visible_options =
      {"version"};
    visible_options.insert(Configuration::commandline_opts().begin(),
                           Configuration::commandline_opts().end());
    llvm::StringMap<llvm::cl::Option*> opts;
    llvm::cl::getRegisteredOptions(opts);
    for(auto it = opts.begin(); it != opts.end(); ++it){
      if(visible_options.count(it->getKey()) == 0){
        it->getValue()->setHiddenFlag(llvm::cl::Hidden);
      }
    }
  }
  llvm::cl::opt<std::string>
  input_file(llvm::cl::desc("<input bitcode or assembly>"),
             llvm::cl::Positional,
             llvm::cl::init("-"));
  llvm::cl::ParseCommandLineOptions(argc, argv);

  /* Use DPORDriver to explore the given module */
  try{
    Configuration conf;
    conf.assign_by_commandline();
    DPORDriver *driver =
      DPORDriver::parseIRFile(input_file,conf);

    DPORDriver::Result res = driver->run();
    std::cout << "Trace count: " << res.trace_count
              << " (also " << res.sleepset_blocked_trace_count
              << " sleepset blocked)\n";
    if(res.has_errors()){
      std::cout << "\n Error detected:\n"
                << res.error_trace.computation_to_string(2);
    }

    delete driver;

    llvm::llvm_shutdown();
  }catch(std::exception *exc){
    std::cerr << "Error: " << exc->what() << "\n";
    llvm::llvm_shutdown();
    return 1;
  }catch(std::exception &exc){
    std::cerr << "Error: " << exc.what() << "\n";
    llvm::llvm_shutdown();
    return 1;
  }

  return 0;
}
