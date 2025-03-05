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
#include "Vscankey.h"

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

int pinsel(int x, int a, int b) {
    assert(a < b);
    int y = 0;
    for (int i = a; i <= b; i++) {
        y |= pin(x, i);
    }
    return y;
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

void cycle_clock(Vscankey* scankey) {
    scankey->clk = 1; scankey->eval();
    scankey->clk = 0; scankey->eval();
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
  Vscankey *scankey = new Vscankey; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  // grade.py parameters - DO NOT DELETE!
  // GRADEPY 1 [15.0]
  /***********************************/
  // scankey - Step 1
  scankey->clk = 0;
  scankey->rst = 0;
  scankey->in = 0;
  scankey->eval();

  scankey->rst = 1;
  scankey->eval();
  if (scankey->out == 0 && scankey->strobe == 0)
    passed_subtests++;
  total_subtests++;

  scankey->rst = 0;
  scankey->eval();


  for(int i = 1; i < 1<<20; i++) {
    int bits = 0;
    scankey->in = i;
    scankey->eval();
    bits |= pin(scankey->in,19) | pin(scankey->in,17) | pin(scankey->in,15) | pin(scankey->in,13) | pin(scankey->in,11) | pin(scankey->in,9) | pin(scankey->in,7) | pin(scankey->in,5) | pin(scankey->in,3) | pin(scankey->in,1);
    bits |= (pinsel(scankey->in,18,19) | pinsel(scankey->in,14,15) | pinsel(scankey->in,10,11) | pinsel(scankey->in,6,7) | pinsel(scankey->in,2,3)) << 1;
    bits |= (pinsel(scankey->in,12,15) | pinsel(scankey->in,4,7)) << 2;
    bits |= (pinsel(scankey->in,8,15)) << 3;
    bits |= (pinsel(scankey->in,16,19)) << 4;
    // scankey->out MUST be combinational.
    if (scankey->out == bits)
      passed_subtests++;
    else
      std::cout << "i " << std::to_string(i) << ", scankey->out " << std::to_string(scankey->out) << " and bits " << std::to_string(bits) << std::endl;
    total_subtests++;
    // scankey->strobe MUST be the output of a flip-flop
    // that only changes after two clock cycles.  Here's one clock cycle...
    if (scankey->strobe == 0)
      passed_subtests++;
    total_subtests++;
    // First clock cycle...
    cycle_clock(scankey);
    if (scankey->strobe == 0)
      passed_subtests++;
    total_subtests++;
    // Second clock cycle...
    cycle_clock(scankey);
    // So now strobe MUST be 1.
    if (scankey->strobe == 1)
      passed_subtests++;
    total_subtests++;
    // When we "release" our button, strobe must return to zero
    // again after two clock cycles.  one clock cycle...
    scankey->in = 0;
    cycle_clock(scankey);
    if (scankey->strobe == 1)
      passed_subtests++;
    total_subtests++;
    // And now strobe should be zero.
    cycle_clock(scankey);
    if (scankey->strobe == 0)
      passed_subtests++;
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
  scankey->final();

  // Destroy models
  delete scankey;
  scankey = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
