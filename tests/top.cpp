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
#include "Vtop.h"

#include <alsa/asoundlib.h>
#include "verilated_fst_c.h"
static VerilatedFstC* tfp = new VerilatedFstC;
const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};

static std::string device = "default";            /* playback device */

// Verilator using new C++?  Need to include these now.
#include <iostream>
#include <chrono>
#include <thread>
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, seconds

// Current simulation time (64-bit unsigned)
vluint64_t main_time = 0;
// Called by $time in Verilog
double sc_time_stamp()
{
  return main_time; // Note does conversion to real, to match SystemC
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

static int TIMESTEP = 0;
static int MOD_M = 10000;
void cycle_clocks(Vtop* top, int n) {
  while (n--) {
    top->hz2m = 0; top->eval();
    contextp->timeInc(1);
    tfp->dump(contextp->time());
    top->hz2m = 1; top->eval();
    TIMESTEP++;
    if (TIMESTEP == MOD_M) {
        top->hz100 = (top->hz100 == 1) ? 0 : 1; 
        top->eval();
        TIMESTEP = 0;
    }
    contextp->timeInc(1);
    tfp->dump(contextp->time());
  }
}

void record_audio(Vtop* top, unsigned char sample[], int sample_len) {
  for (int i = 0; i < sample_len - 1500; i++) {
    int ones = 0;
    int zeroes = 0;
    for (int j = 0; j < 256; j++) {
      cycle_clocks(top, 1);
      if (top->right & 0x1)
        ones++;
      else
        zeroes++;
    }
    sample[i] = ones & 0xff;
  }
  for (int i = sample_len - 1500; i < sample_len; i++) {
    sample[i] = 0x80;
  }
}

// https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
void play_audio(Vtop* top, snd_pcm_t *handle, snd_pcm_sframes_t frames, unsigned char sample[], long sample_len) {
  if (frames > 0 && frames < (long)sample_len)
      printf("Short write (expected %li, wrote %li)\n", (long)sample_len, frames);
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

  // enable tracing
  Verilated::traceEverOn(true);

  // Construct the Verilated model, from each module after Verilating each module file
  Vtop *top = new Vtop; // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper

  top->trace(tfp, 9);
  tfp->open("top.fst");

  // top
  int TO_EDIT = 19;
  int TO_PLAY = 18;
  int TO_RAW = 16;

  int KICK  = 3;
  int CLAP  = 2;
  int HIHAT = 1;
  int SNARE = 0;

  // test top reset
  top->hz2m = 0; 
  top->hz100 = 0; 
  top->reset = 0; 
  top->pb = 0;
  top->eval();
  contextp->timeInc(1);
  tfp->dump(contextp->time());
  
  /////////////////////////////////////////////////////////////
  // this is a testbench that will perform the following actions on your 
  // top module implementing the drum machine:

  // assert reset for 5 clock cycles of hz2m.
  top->reset = 1;
  top->eval();
  cycle_clocks(top, 5);

  top->reset = 0;
  top->eval();

  std::cout << "Test 1: Just after reset, we should be in EDIT mode (blue = 1)." << "\n";
  std::cout << "blue: " << std::to_string(top->blue) << "\n";
  std::cout << "green: " << std::to_string(top->green) << "\n";
  std::cout << "red: " << std::to_string(top->red) << "\n";

  // switch to RAW
  top->pb = 1 << TO_RAW; top->eval();
  cycle_clocks(top, MOD_M * 2 * 1);

  std::cout << "\nTest 2: Pressing W should take us to RAW mode (red = 1)." << "\n";
  std::cout << "blue: " << std::to_string(top->blue) << "\n";
  std::cout << "green: " << std::to_string(top->green) << "\n";
  std::cout << "red: " << std::to_string(top->red) << "\n";

  // initialize audio
  unsigned char kick_sample[8000];
  unsigned char clap_sample[8000];
  unsigned char hihat_sample[8000];
  unsigned char snare_sample[8000];
  unsigned char sample[8000*4 + 4000*4];
  snd_pcm_t *handle;
  snd_pcm_sframes_t frames;

  std::cout << "\nInitializing audio library..." << "\n";
  int err;
  if ((err = snd_pcm_open(&handle, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  if ((err = snd_pcm_set_params(handle,
              SND_PCM_FORMAT_U8,
              SND_PCM_ACCESS_RW_INTERLEAVED,
              1,
              8000,
              1,
              50000)) < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  sleep_until(system_clock::now() + seconds(1));

  std::cout << "\nTest 3: Recording a kick..." << "\n";
  // release all buttons...
  top->pb = 0; top->eval();
  // let 4 hz100 cycles pass...
  cycle_clocks(top, MOD_M * 2 * 2);

  // now play a kick.
  top->pb = 1 << KICK; top->eval();
  cycle_clocks(top, MOD_M * 2 * 2);
  // 4000 samples, 256 bits, 8000 sample rate, played 10 times.
  record_audio(top, kick_sample, sizeof(kick_sample));

  std::cout << "\nTest 4: Recording a clap..." << "\n";
  // release all buttons...
  top->pb = 0; top->eval();
  // let 4 hz100 cycles pass...
  cycle_clocks(top, MOD_M * 2 * 8);

  // now play a clap.
  top->pb = 1 << CLAP; top->eval();
  cycle_clocks(top, MOD_M * 2 * 2);
  // 4000 samples, 256 bits, 8000 sample rate, played 10 times.
  record_audio(top, clap_sample, sizeof(clap_sample));

  std::cout << "\nTest 5: Recording a hihat..." << "\n";

  // release all buttons...
  top->pb = 0; top->eval();
  // let 4 hz100 cycles pass...
  cycle_clocks(top, MOD_M * 2 * 8);

  // now play a hihat.
  top->pb = 1 << HIHAT; top->eval();
  cycle_clocks(top, MOD_M * 2 * 2);
  // 4000 samples, 256 bits, 8000 sample rate, played 10 times.
  record_audio(top, hihat_sample, sizeof(hihat_sample));

  std::cout << "\nTest 6: Recording a snare..." << "\n";
  // release all buttons...
  top->pb = 0; top->eval();
  // let 4 hz100 cycles pass...
  cycle_clocks(top, MOD_M * 2 * 8);

  // now play a snare.
  top->pb = 1 << SNARE; top->eval();
  cycle_clocks(top, MOD_M * 2 * 2);
  // 4000 samples, 256 bits, 8000 sample rate, played 10 times.
  record_audio(top, snare_sample, sizeof(snare_sample));

  std::cout << "\nTest 7: Playing back all recorded sound (should hear each sample 2 times)" << "\n";
  unsigned char* sample_ptr = NULL;
  for (int i = 0; i < 8000*4 + 4000*4; i++) {
    sample[i] = 0x80;
  }
  for (int i = 0; i < 4; i++) {
    switch (i) {
      case 0:
        sample_ptr = kick_sample;
        break;
      case 1:
        sample_ptr = clap_sample;
        break;
      case 2:
        sample_ptr = hihat_sample;
        break;
      case 3:
        sample_ptr = snare_sample;
        break;
    }
    for (int j = 0; j < 8000; j++) {
      sample[j + (i * 4000) + (i * 8000)] = sample_ptr[j];
    }
  }

  frames = snd_pcm_writei(handle, sample, sizeof(sample));
  if (frames < 0)
      frames = snd_pcm_recover(handle, frames, 0);
  if (frames < 0) {
      printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
  }
  play_audio(top, handle, frames, sample, sizeof(sample));

  tfp->close();

  // if (err < 0)
  //     printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
  sleep_until(system_clock::now() + seconds(1));
  // close sound device
  snd_pcm_close(handle);

  std::cout << "\nIf you were able to hear the samples, great!  If not, there may be something wrong with "
            << "your audio setup.  Ensure you are following the same settings as "
            << "described on the project page, and test it by opening a YouTube video in " 
            << "a browser, eg. Firefox.  If it plays fine, or if your audio is distorted, the issue may be "
            << "with your Verilog code.  Double-check module clock frequencies and signed-to-unsigned conversion." << "\n";

  /////////////////////////////////////////////////////////////

  top->final();

  // Destroy models
  delete top;
  top = NULL;

  // Fin
  return 0;
}
