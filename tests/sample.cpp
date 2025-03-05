// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2017 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0
//======================================================================
// Include common routines
#include <verilated.h>

static int passed_test_count = 0; // every time we perform a test, and the test passes, increment this by one.
static int total_test_count = 0;  // every time we perform a test, increment this by one.

// Include model header, generated from Verilating "tb_top.v"
#include "Vsample.h"

// Verilator using new C++?  Need to include these now (sstep2021)
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

// Current simulation time (64-bit unsigned)
vluint64_t main_time = 0;
// Called by $time in Verilog
double sc_time_stamp()
{
  return main_time; // Note does conversion to real, to match SystemC
}

int concat(int w, int a, int b)
{
  return (a << w) | b;
}

void cycle_clock(Vsample* sample) {
    sample->clk = 1; sample->eval();
    sample->clk = 0; sample->eval();
}

std::string bin(int b) {
  std::string ans;
  if (b == 0)
    return "0";
  while (b != 0) {
    ans += b % 2 == 0 ? "0" : "1";
    b /= 2;
  }
  std::reverse(ans.begin(), ans.end());
  return ans;
}

void update_tests(int passed, int total, std::string test) {
  // add tests to global test variables
  passed_test_count += passed;
  total_test_count += total;
  // perform sanity check on global test variables
  assert(passed_test_count <= total_test_count);
  // let TA know if this set of tests failed
  std::cout << "Part " << test << ": " << std::to_string(passed) << " out of " << std::to_string(total) << " tests passed.\n";
}

int main(int argc, char **argv, char **env)
{
  // This is a more complicated example, please also see the simpler examples/make_hello_c.

  // Prevent unused variable warnings
  if (0 && argc && argv && env) {}

  // Set debug level, 0 is off, 9 is highest presently used
  // May be overridden by commandArgs
  Verilated::debug(0);

  // Randomization reset policy
  // May be overridden by commandArgs
  Verilated::randReset(2);

  // Verilator must compute traced signals
  Verilated::traceEverOn(true);

  // Pass arguments so Verilated code can see them, e.g. $value$plusargs
  // This needs to be called before you create any model
  Verilated::commandArgs(argc, argv);
  
  // search for a kick.mem file
  // https://stackoverflow.com/questions/59022814/how-to-check-if-a-file-exists-in-c
  namespace fs = std::filesystem;
  fs::path f{ "../audio/kick.mem" };
  int kick_mem[4096];
  for (int i = 0; i < 4096; i++)
    kick_mem[i] = 0;
  if (fs::exists(f)) {
    // resolve full path to file and print it out:
    std::ifstream kick_mem_file("../audio/kick.mem");
    std::string kick_mem_line;
    int i = 0;
    // each line in the file is a byte in hex form, written with ASCII characters (ex. 00\n02\n04...)
    // for example, if the first line is "00", convert it to 8-bit 0 and store it in kick_mem[0]
    // then, print it out so it looks like this: 0x00 and do this for each line
    while (std::getline(kick_mem_file, kick_mem_line)) {
      std::stringstream ss;
      ss << std::hex << kick_mem_line;
      int x;
      ss >> x;
      // print it out as 0x00
      kick_mem[i] = x;
      i++;
    }
  }
  else {
    // print out current directory
    std::cout << "Fatal error: cannot find kick.mem file.  This file should have been copied in by ece270-setup.  Please do not remove it.";
    std::cout << "Rerun the setup script to get the file again, or ask a TA for help if the file already exists.\n";
    exit(1);
  }

  // Construct the Verilated model, from each module after Verilating each module file
  Vsample *sample = new Vsample; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  /*************************************************************************/
  // sample
  sample->clk = 0;
  sample->rst = 0;
  sample->enable = 0;
  sample->eval();

  sample->rst = 1;
  sample->eval();
  if (sample->out == kick_mem[0]) {
    passed_subtests++;
  }
  else {
    std::cout << "reset test failed - sample->out (0x" << std::hex << uint64_t(sample->out) << ")must be the first value ";
    std::cout << "(0x" << std::hex << kick_mem[0] << ") in the file.\n";
  }
  total_subtests++;

  sample->rst = 0;
  sample->enable = 1;
  sample->eval();
  // one cycle to get 
  cycle_clock(sample); cycle_clock(sample); 
  for(int i = 1; i < (4000 + 128); i++) { 
    cycle_clock(sample);
    if (int(sample->out) == kick_mem[i % 4001]) {
      passed_subtests++;
    }
    else {
      std::cout << "at idx " << std::to_string(i % 4001) << ", sample->out = ";
      std::cout << std::hex << int(sample->out);
      std::cout << " but should be ";
      std::cout << std::hex << kick_mem[i % 4001];
      std::cout << std::endl;
    }
    total_subtests++;
  }

  update_tests(passed_subtests, total_subtests, "1");
  /***********************************/

  passed_subtests = 0;
  total_subtests = 0;

  /***********************************/
  // END TESTS
  /*************************************************************************/

  // good to have to detect bugs
  assert(passed_test_count <= total_test_count);

  if (passed_test_count == total_test_count)
  {
    std::cout << "ALL " << std::to_string(total_test_count) << " TESTS PASSED"
              << "\n";
  }
  else
  {
    std::cout << "ERROR: " << std::to_string(passed_test_count) << "/" << std::to_string(total_test_count) << " tests passed.\n";
  }

  // Final model cleanups
  sample->final();

  // Destroy models
  delete sample;
  sample = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
