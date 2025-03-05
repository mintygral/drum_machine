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
#include "Vsequence_editor.h"

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

std::string padbin(uint64_t x, int len) {
  std::string ans = bin(x);
  while (ans.length() < len) {
    ans = "0" + ans;
  }
  return ans;
}

uint64_t pin(uint64_t a, int b) {
  return (a >> b) & 1;
}

uint64_t pinsel(uint64_t x, int b, int a) {
    assert(a < b);
    uint64_t y = 0;
    for (int i = b; i >= a; i--) {
        y |= pin(x, i);
        if (i > a)
          y <<= 1;
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

void cycle_clock(Vsequence_editor* seq_editor) {
    seq_editor->clk = 1; seq_editor->eval();
    seq_editor->clk = 0; seq_editor->eval();
}

uint32_t get_seq_smpl (Vsequence_editor* seq_editor) {
  uint32_t compiled = 0;
  compiled = (compiled << 4) | (seq_editor->seq_smpl_8 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_7 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_6 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_5 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_4 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_3 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_2 & 0xF);
  compiled = (compiled << 4) | (seq_editor->seq_smpl_1 & 0xF);
  return compiled;
}

int sel_seq_smpl (Vsequence_editor* seq_editor, int idx = -1) {
  if (idx == seq_editor->set_time_idx)
    idx = seq_editor->set_time_idx;
  switch (idx) {
    case 0: return seq_editor->seq_smpl_1 & 0xF;
    case 1: return seq_editor->seq_smpl_2 & 0xF;
    case 2: return seq_editor->seq_smpl_3 & 0xF;
    case 3: return seq_editor->seq_smpl_4 & 0xF;
    case 4: return seq_editor->seq_smpl_5 & 0xF;
    case 5: return seq_editor->seq_smpl_6 & 0xF;
    case 6: return seq_editor->seq_smpl_7 & 0xF;
    case 7: return seq_editor->seq_smpl_8 & 0xF;
    default: return -1;
  }
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
  Vsequence_editor *seq_editor = new Vsequence_editor; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  int passed_subtests = 0; // if we have multiple subtests, we need to know how many passed for each
  int total_subtests = 0;

  /*************************************************************************/
  // BEGIN TESTS
  // grade.py parameters - DO NOT DELETE!
  // GRADEPY 1 [15.0]
  /***********************************/
  // sequence_editor - Step 1
  int EDIT = 0;
  int PLAY = 1;
  int RAW = 2;

  seq_editor->clk = 0;
  seq_editor->rst = 0;
  seq_editor->mode = 0;
  seq_editor->set_time_idx = 0;
  seq_editor->tgl_play_smpl = 0;
  seq_editor->eval();

  seq_editor->rst = 1;
  seq_editor->eval();
  if (get_seq_smpl(seq_editor) == 0)
    passed_subtests++;
  else {
    std::cout << "Power-on reset - rst == 1: all outputs should be 0, but is 0x";
    std::cout << std::hex << get_seq_smpl(seq_editor); 
    std::cout << "\n";
  }
  total_subtests++;
  seq_editor->rst = 0;
  seq_editor->mode = EDIT;
  seq_editor->eval();

  uint32_t exp_smpl = get_seq_smpl(seq_editor);

  for (seq_editor->set_time_idx = 0; seq_editor->set_time_idx <= 0x7; seq_editor->set_time_idx++) {
    for (seq_editor->tgl_play_smpl = 0; seq_editor->tgl_play_smpl <= 0xF; seq_editor->tgl_play_smpl++) {
      // tgl_play_smpl[3:0] should toggle corresponding bit in seq_smpl_[8:1], where 
      // seq_smpl_[8:1] is the output of the sequence editor determined by the 
      // current time index and the current mode.
      // change seq_smpl_1 when set_time_idx == 0, seq_smpl_2 if set_time_idx == 1, etc.
      // change it such that if tgl_play_smpl[3] == 1, then the 3rd bit of seq_smpl_[8:1] is toggled
      uint32_t prev_seq_smpl = get_seq_smpl(seq_editor);
      seq_editor->eval();
      cycle_clock(seq_editor);
      uint32_t seq_smpl = get_seq_smpl(seq_editor);
      exp_smpl ^= seq_editor->tgl_play_smpl << (seq_editor->set_time_idx * 4);
      // std::cout << "seq_smpl ";
      // std::cout << std::hex << seq_smpl;
      // std::cout << ", exp_smpl ";
      // std::cout << std::hex << exp_smpl;
      // std::cout << ", idx ";
      // std::cout << std::to_string(seq_editor->set_time_idx * 4 + 3) << " " << std::to_string(seq_editor->set_time_idx * 4);
      // std::cout << ", pinsel ";
      // std::cout << std::to_string(pinsel(exp_smpl, seq_editor->set_time_idx * 4 + 3, seq_editor->set_time_idx * 4));
      // std::cout << std::endl;
      if (seq_smpl == exp_smpl)
        passed_subtests++;
      else {
        std::cout << "When seq_smpl_" << std::to_string(seq_editor->set_time_idx + 1) << " was " << padbin(pinsel(prev_seq_smpl, seq_editor->set_time_idx * 4 + 3, seq_editor->set_time_idx * 4), 4) << ",\n";
        std::cout << "  Set inputs as set_time_idx=" << padbin(seq_editor->set_time_idx, 3); 
        std::cout << " tgl_play_smpl=" << padbin(seq_editor->tgl_play_smpl, 4) << "\n";
        std::cout << "But got seq_smpl_" << std::to_string(seq_editor->set_time_idx + 1) << " = 4'b"; 
        std::cout << padbin(sel_seq_smpl(seq_editor), 4);
        std::cout << " when expected val = 4'b";
        std::cout << padbin(pinsel(exp_smpl, seq_editor->set_time_idx * 4 + 3, seq_editor->set_time_idx * 4), 4);
        std::cout << ".\n\n";
      }
      total_subtests++;
    }
  }

  // async reset should work instantly
  seq_editor->rst = 1;
  seq_editor->eval();
  if (get_seq_smpl(seq_editor) == 0)
    passed_subtests++;
  else {
    std::cout << "Post-op reset - rst == 1: all outputs should be 0, but is 0x";
    std::cout << std::hex << get_seq_smpl(seq_editor); 
    std::cout << "\n";
  }
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
  seq_editor->final();

  // Destroy models
  delete seq_editor;
  seq_editor = NULL;

  // Fin
  return passed_test_count == total_test_count ? 0 : 1;
}
