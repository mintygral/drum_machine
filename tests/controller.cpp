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
#include "Vcontroller.h"

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
  std::cout << "controller: " << std::to_string(passed) << " out of " << std::to_string(total) << " tests passed.\n";
}

void cycle_clock(Vcontroller* controller) {
    controller->clk = 1; controller->eval();
    controller->clk = 0; controller->eval();
}
void assert_reset(Vcontroller* controller) {
    controller->rst = 1; controller->eval();
    controller->rst = 0; controller->eval();
}

void check_output(std::string state, bool cond, std::string test, int* passed, int* total) {
  /* 1	Blue	9	Light Blue
   2	Green	0	Black
   3	Aqua	10	Light Green
   4	Red	11	Light Aqua
   5	Purple	12	Light Red
   7	White	14	Light Yellow
   8	Gray	15	Bright White */
  if (cond) {
    (*passed)++;
    std::cout << "\033[0;32m";
    std::cout<<"[PASS] ";

  } else {
    std::cout << "\033[0;31m";
    std::cout<<"[FAIL] ";
  }
  std::cout << "\033[0m";
  std::cout << state << ": Checking if " << test << "\n";
  (*total)++;
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
  Vcontroller *controller = new Vcontroller; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  /***********************************/
  // controller
  int EDIT = 0;
  int PLAY = 1;
  int RAW = 2;

  // test controller reset
  controller->clk = 0; 
  controller->rst = 0; 
  controller->mode = 0;
  controller->set_edit = 0;
  controller->set_play = 0;
  controller->set_raw = 0;
  controller->eval();
  print_header("Power-on: testing reset");
  assert_reset(controller);
  check_output("RESET", controller->mode == EDIT, "state == EDIT", &passed_subtests, &total_subtests);
  
  print_header("Testing EDIT -> EDIT, shouldn't change");
  // then, shiftdown should be empty, so we assert the corresponding signal
  controller->set_edit = 1; controller->eval();
  cycle_clock(controller);
  check_output("EDIT", controller->mode == EDIT, "state == EDIT", &passed_subtests, &total_subtests);

  // strobe=1, change state to PLAY
  print_header("Testing EDIT -> PLAY");
  controller->set_edit = 0;
  controller->set_play = 1; controller->eval();
  cycle_clock(controller);
  check_output("PLAY", controller->mode == PLAY, "state == PLAY", &passed_subtests, &total_subtests);
  controller->set_play = 0; controller->eval();
  cycle_clock(controller);
  check_output("PLAY_2", controller->mode == PLAY, "state == PLAY", &passed_subtests, &total_subtests);

  // strobe=1, change state to RAW
  print_header("Testing PLAY -> RAW");
  controller->set_raw = 1; controller->eval();
  cycle_clock(controller);
  cycle_clock(controller);
  check_output("RAW", controller->mode == RAW, "state == RAW", &passed_subtests, &total_subtests);
  controller->set_raw = 0; controller->eval();
  cycle_clock(controller);
  check_output("RAW_2", controller->mode == RAW, "state == RAW", &passed_subtests, &total_subtests);

  // strobe=1 change state to EDIT
  print_header("Testing RAW -> EDIT");
  controller->set_edit = 1; controller->eval();
  cycle_clock(controller);
  check_output("EDIT", controller->mode == EDIT, "state == EDIT", &passed_subtests, &total_subtests);
  cycle_clock(controller);
  check_output("EDIT_2", controller->mode == EDIT, "state == EDIT", &passed_subtests, &total_subtests);

  // shiftdown reset is asserted, so set_play should not be true anymore
  print_header("Post-operation reset");
  controller->set_play = 0; controller->eval();
  controller->set_raw = 0; controller->eval();
  controller->set_edit = 0; controller->eval();
  controller->rst = 1; controller->eval();
  check_output("POSTRESET", controller->mode == EDIT, "state == EDIT", &passed_subtests, &total_subtests);
  
  std::cout << std::endl;
 
  update_tests(passed_subtests, total_subtests, "1");
  /***********************************/

  passed_subtests = 0;
  total_subtests = 0;

  /***********************************/
  // END TESTS
  /*************************************************************************/

  // good to have to detect bugs in the testbench
  assert(passed_test_count <= total_test_count);

  if (passed_test_count == total_test_count)
  {
    std::cout << "\n                                                ";
    std::cout << "\n     Good job!      ⢠⣿⣶⣄⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀  ";
    std::cout << "\n          ⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦⣄⣀⡀⣠⣾⡇⠀⠀⠀⠀   ";
    std::cout << "\n          ⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀   ";
    std::cout << "\n          ⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⢿⣿⣿⡇⠀⠀⠀⠀   ";
    std::cout << "\n          ⣶⣿⣦⣜⣿⣿⣿⡟⠻⣿⣿⣿⣿⣿⣿⣿⡿⢿⡏⣴⣺⣦⣙⣿⣷⣄⠀⠀⠀   ";
    std::cout << "\n          ⣯⡇⣻⣿⣿⣿⣿⣷⣾⣿⣬⣥⣭⣽⣿⣿⣧⣼⡇⣯⣇⣹⣿⣿⣿⣿⣧⠀⠀   ";
    std::cout << "\n          ⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠸⣿⣿⣿⣿⣿⣿⣿⣷⠀   ";
    std::cout << "\n                                                ";
    std::cout << "\n        ⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛⠛\n";
    std::cout << "ALL " << std::to_string(total_test_count) << " TESTS PASSED"
              << "\n";
  }
  else
  {
    std::cout << "ERROR: " << std::to_string(passed_test_count) << "/" << std::to_string(total_test_count) << " tests passed.\n";
  }

  // Final model cleanups
  controller->final();

  // Destroy models
  delete controller;
  controller = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
