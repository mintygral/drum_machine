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
#include "Vclkdiv.h"

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

void cycle_clock(Vclkdiv* clkdiv) {
    clkdiv->clk = 1; clkdiv->eval();
    clkdiv->clk = 0; clkdiv->eval();
}

// write a C++ function that takes a string, and pads it with dashes on both sides of the string 
// such that the total length of the output string is 80 characters, and prints it out.
void print_header(std::string s) {
  int n = s.length();
  int pad = (50 - n) / 2;
  std::string dashes = "";
  for (int i = 0; i < pad; i++) {
    dashes += "-";
  }
  std::cout << dashes << " " << s << " " << dashes << "\n";
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
  Vclkdiv *clkdiv = new Vclkdiv; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  // grade.py parameters - DO NOT DELETE!
  // GRADEPY 1 [15.0]
  /***********************************/
  // clkdiv - Step 1
  clkdiv->clk = 0;
  clkdiv->rst = 0;
  clkdiv->eval();

  clkdiv->rst = 1;
  clkdiv->eval();
  if (clkdiv->hzX == 0)
    passed_subtests++;
  total_subtests++;

  clkdiv->rst = 0;
  clkdiv->lim = 6;
  print_header("Testing for 8 Hz");
  std::cout << "Setting lim to 8'd" << std::to_string(clkdiv->lim) << "..." << std::endl;
  clkdiv->eval();

  int ones = 0;
  int rising_edges = 0;
  int cur = 0;

  for(int i = 0; i < 100; i++) {
    cycle_clock(clkdiv);
    ones += clkdiv->hzX;
    if (cur == 0 && clkdiv->hzX == 1)
      rising_edges += 1;
    cur = clkdiv->hzX;
  }

  std::cout << "Number of clock cycles where output was 1: " << std::to_string(ones) << std::endl;
  std::cout << "Rising edges detected in 100 clock cycles (0 ==> 1): " << std::to_string(rising_edges) << std::endl << std::endl;
  if (ones > 4) passed_subtests++;
  total_subtests++;
  if (rising_edges == 7) passed_subtests++;
  total_subtests++;
  // print out passed_subtests and total_subtests
  // std::cout << "rising_edges: " << std::to_string(rising_edges) << ", passed_subtests: " << std::to_string(passed_subtests) << ", total_subtests: " << std::to_string(total_subtests) << std::endl;
  if (rising_edges >= 1) passed_subtests++;   // give partial credit for any kind of oscillation
  total_subtests++;

  clkdiv->rst = 0;
  clkdiv->lim = 12;
  print_header("Testing for 4 Hz");
  std::cout << "Setting lim to 8'd" << std::to_string(clkdiv->lim) << "...";
  clkdiv->eval();

  ones = 0;
  rising_edges = 0;
  cur = 0;

  for(int i = 0; i < 100; i++) {
    cycle_clock(clkdiv);
    ones += clkdiv->hzX;
    if (cur == 0 && clkdiv->hzX == 1)
        rising_edges += 1;
    cur = clkdiv->hzX;
  }

  std::cout << "\nNumber of clock cycles where output was 1: " << std::to_string(ones) << std::endl;
  std::cout << "Rising edges detected in 100 clock cycles (0 ==> 1): " << std::to_string(rising_edges) << std::endl << std::endl;
  if (ones > 4) passed_subtests++;
  total_subtests++;
  if (rising_edges == 4) passed_subtests++;
  total_subtests++;
  if (rising_edges >= 1) passed_subtests++;   // give partial credit for any kind of oscillation
  total_subtests++;


  clkdiv->rst = 1; clkdiv->eval(); clkdiv->rst = 0; clkdiv->eval(); 
  clkdiv->lim = 24;
  print_header("Testing for 2 Hz");
  std::cout << "Setting lim to 8'd" << std::to_string(clkdiv->lim) << "..." << std::endl;
  clkdiv->eval();

  ones = 0;
  rising_edges = 0;
  cur = 0;

  for(int i = 0; i < 100; i++) {
    cycle_clock(clkdiv);
    ones += clkdiv->hzX;
    if (cur == 0 && clkdiv->hzX == 1)
        rising_edges += 1;
    cur = clkdiv->hzX;
  }

  std::cout << "Number of clock cycles where output was 1: " << std::to_string(ones) << std::endl;
  std::cout << "Rising edges detected in 100 clock cycles (0 ==> 1): " << std::to_string(rising_edges) << std::endl << std::endl;
  if (ones > 2) passed_subtests++;
  total_subtests++;
  if (rising_edges == 2) passed_subtests++;
  total_subtests++;
  if (rising_edges >= 1) passed_subtests++;   // give partial credit for any kind of oscillation
  total_subtests++;


  clkdiv->rst = 1; clkdiv->eval(); clkdiv->rst = 0; clkdiv->eval(); 
  clkdiv->lim = 49;
  print_header("Testing for 1 Hz");
  std::cout << "Setting lim to 8'd" << std::to_string(clkdiv->lim) << "..." << std::endl;
  clkdiv->eval();

  ones = 0;
  rising_edges = 0;
  cur = 0;

  for(int i = 0; i < 100; i++) {
    cycle_clock(clkdiv);
    ones += clkdiv->hzX;
    if (cur == 0 && clkdiv->hzX == 1)
        rising_edges += 1;
    cur = clkdiv->hzX;
  }

  std::cout << "Number of clock cycles where output was 1: " << std::to_string(ones) << std::endl;
  std::cout << "Rising edges detected in 100 clock cycles (0 ==> 1): " << std::to_string(rising_edges) << std::endl << std::endl;
  if (rising_edges == 1) passed_subtests++;
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
  clkdiv->final();

  // Destroy models
  delete clkdiv;
  clkdiv = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
