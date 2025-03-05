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
#include "Vsequencer.h"

// Verilator using new C++?  Need to include these now (sstep2021)
#include <iostream>
using namespace std;

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

std::string bin(int b)
{
  std::string ans;
  while (b != 0)
  {
    ans += b % 2 == 0 ? "0" : "1";
    b /= 2;
  }
  std::reverse(ans.begin(), ans.end());
  return ans;
}

int pin(int a, int b) {
  return (a >> b) & 1;
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

void cycle_clock(Vsequencer* sequencer) {
    sequencer->clk = 1; sequencer->eval();
    sequencer->clk = 0; sequencer->eval();
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

  // Construct the Verilated model, from each module after Verilating each module file
  Vsequencer *sequencer = new Vsequencer; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  // grade.py parameters - DO NOT DELETE!
  // GRADEPY 1 [15.0]
  /***********************************/
  // sequencer - Step 1
  sequencer->clk = 0;
  sequencer->rst = 0;
  sequencer->srst = 0;
  sequencer->go_left = 0;
  sequencer->go_right = 0;
  sequencer->eval();

  sequencer->rst = 1;
  sequencer->eval();
  if (sequencer->seq_out == 0x80)
    passed_subtests++;
  else
    std::cout << "sequencer - rst == 1: out should be 0x80, but is 0x" << std::hex << sequencer->seq_out << "\n";
  total_subtests++;

  sequencer->rst = 0;
  sequencer->go_left = 0;
  sequencer->go_right = 1;
  sequencer->eval();
  // should move to the right, with the last bit wrapping around to the leftmost bit
  int expected = (sequencer->seq_out == 0x1) ? 0x80 : sequencer->seq_out >> 1;
  for (int i = 0; i < 12; i++) {
    cycle_clock(sequencer);
    if (sequencer->seq_out == expected)
      passed_subtests++;
    else
      std::cout << "sequencer - going right: out should be 0x" << std::hex << expected << " , but is 0x" << std::hex << (sequencer->seq_out) << "\n";
    total_subtests++;
    expected = (expected == 0x1) ? 0x80 : expected >> 1;
  }
  
  sequencer->go_left = 1;
  sequencer->go_right = 0;
  sequencer->eval();
  // should move to the left, with the first bit wrapping around to the rightmost bit
  expected = (sequencer->seq_out == 0x80) ? 0x1 : sequencer->seq_out << 1;
  for (int i = 0; i < 13; i++) {
    cycle_clock(sequencer);
    if (sequencer->seq_out == expected)
      passed_subtests++;
    else
      std::cout << "sequencer - going left: out should be 0x" << std::hex << expected << " , but is 0x" << std::hex << (sequencer->seq_out) << "\n";
    total_subtests++;
    expected = (expected == 0x80) ? 0x1 : expected << 1;
  }

  expected = sequencer->seq_out;
  sequencer->go_left = 0;
  sequencer->go_right = 0;
  sequencer->srst = 1;
  sequencer->eval();
  // srst is asserted, but not clocked yet - value must not change.
  if (sequencer->seq_out == expected)
    passed_subtests++;
  else
    std::cout << "sequencer - srst == 1 but did not clock: out should remain 0x" << std::hex << expected << ", but is 0x" << std::hex << (sequencer->seq_out) << "\n";
  total_subtests++;
  expected = 0x80;
  for (int i = 0; i < 16; i++) {
    cycle_clock(sequencer);
    if (sequencer->seq_out == expected)
      passed_subtests++;
    else
      std::cout << "sequencer - srst == 1 + rising clk edge: out should be 0x" << std::hex << expected << ", but is 0x" << std::hex << (sequencer->seq_out) << "\n";
    total_subtests++;
  }

  // should operate normally again
  sequencer->go_left = 1;
  sequencer->go_right = 0;
  sequencer->srst = 0;
  sequencer->eval();
  expected = (sequencer->seq_out == 0x80) ? 0x1 : sequencer->seq_out << 1;
  for (int i = 0; i < 8; i++) {
    cycle_clock(sequencer);
    if (sequencer->seq_out == expected)
      passed_subtests++;
    else
      std::cout << "sequencer - normal operation 2: out should be 0x" << std::hex << expected << " , but is 0x" << std::hex << (sequencer->seq_out) << "\n";
    total_subtests++;
    expected = (expected == 0x80) ? 0x1 : expected << 1;
  }

  // async reset should work instantly
  sequencer->rst = 1;
  sequencer->eval();
  if (sequencer->seq_out == 0x80)
    passed_subtests++;
  else
    std::cout << "sequencer - rst == 1: out should be 0x80, but is 0x" << std::hex << sequencer->seq_out << "\n";
  total_subtests++;

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
  sequencer->final();

  // Destroy models
  delete sequencer;
  sequencer = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
