export PATH := /home/shay/a/ece270/bin:/usr/bin:$(PATH)
export LD_LIBRARY_PATH := /home/shay/a/ece270/lib:/usr/lib:$(LD_LIBRARY_PATH)
MODULES := scankey clkdiv prienc8to3 sequencer sequence_editor pwm sample controller

YOSYS=yosys
NEXTPNR=nextpnr-ice40
SHELL=bash

PROJ   = drumbit
PINMAP = support/pinmap.pcf
SRC    = scankey.sv clkdiv.sv prienc8to3.sv sequencer.sv sequence_editor.sv pwm.sv sample.sv controller.sv top.sv
ICE    = support/ice40hx8k.sv
UART   = support/uart/*.v
FILES  = $(ICE) $(SRC) $(UART)
BUILD  = ./build

DEVICE  = 8k
TIMEDEV = hx8k
FOOTPRINT = ct256

all: verify cram

#############################################################
# Step 1 verification

ccred=echo -e "\e[31m"
ccgreen=echo -e "\e[32m"
ccyellow=echo -e "\e[33m"
ccend=echo -e "\e[0m"

verify: 
	@for mod in $(MODULES); do \
		make verify_$$mod; \
		echo; \
	done

verify_%: %.sv ../tests/%.cpp
	@echo "$$($(ccyellow))=========================== $@ ===========================$$($(ccend))"
	@rm -rf $*_dir
	@echo Compiling $*...
	@echo Synthesizing to ensure $* compatibility with ice40 FPGA...
	@yosys -p "read_verilog -sv $*.sv; synth_ice40 -top $*" 1>/dev/null
	@echo Testing $*...
	@verilator --cc --build --exe --Mdir $*_dir $*.sv --x-initial 0 ../tests/$*.cpp 1>/dev/null
	@if $*_dir/V$*; then \
			echo "$$($(ccgreen))=========================== TEST PASSED ===========================$$($(ccend))"; \
	else \
			echo "$$($(ccred))=========================== TEST FAILED ===========================$$($(ccend))"; \
	fi
	@rm -rf $*_dir

playaudio: top.sv ../tests/top.cpp
	@echo "$$($(ccyellow))=========================== $@ ===========================$$($(ccend))"
	@rm -rf $*_dir
	@echo Compiling top module...
	@verilator --cc --build --exe --Mdir top_dir top.sv --trace-fst --x-initial 0 -LDFLAGS "-I/usr/lib/x86_64-linux-gnu/ -lasound" ../tests/top.cpp 1>/dev/null
	@echo Playing audio...
	@top_dir/Vtop
	@rm -rf $*_dir

#############################################################
# Flashing design to FPGA

$(BUILD)/$(PROJ).json : $(ICE) $(SRC) $(PINMAP)
	# lint with Verilator
	verilator --lint-only --top-module top -Werror-latch $(SRC)
	# if build folder doesn't exist, create it
	mkdir -p $(BUILD)
	# synthesize using Yosys
	$(YOSYS) -p "read_verilog -sv -noblackbox $(FILES); synth_ice40 -top ice40hx8k -json $(BUILD)/$(PROJ).json"

$(BUILD)/$(PROJ).asc : $(BUILD)/$(PROJ).json
	# Place and route using nextpnr
	$(NEXTPNR) --hx8k --package ct256 --pcf $(PINMAP) --asc $(BUILD)/$(PROJ).asc --json $(BUILD)/$(PROJ).json 2> >(sed -e 's/^.* 0 errors$$//' -e '/^Info:/d' -e '/^[ ]*$$/d' 1>&2)

$(BUILD)/$(PROJ).bin : $(BUILD)/$(PROJ).asc
	# Convert to bitstream using IcePack
	icepack $(BUILD)/$(PROJ).asc $(BUILD)/$(PROJ).bin

flash: $(BUILD)/$(PROJ).bin
	# Flash FPGA with bitstream using iceprog
	iceprog $(BUILD)/$(PROJ).bin

cram: $(BUILD)/$(PROJ).bin
	# Configure FPGA with bitstream using iceprog
	iceprog -S $(BUILD)/$(PROJ).bin

time: $(BUILD)/$(PROJ).asc
	# Re-synthesize
	$(YOSYS) -p "read_verilog -sv -noblackbox $(SRC); synth_ice40 -top top -json $(BUILD)/top.json"
	# Place and route using nextpnr
	$(NEXTPNR) --hx8k --package ct256 --asc $(BUILD)/top.asc --json $(BUILD)/top.json 2> >(sed -e 's/^.* 0 errors$$//' -e '/^Info:/d' -e '/^[ ]*$$/d' 1>&2)
	icetime -tmd hx8k $(BUILD)/top.asc

clean:
	rm -rf *_dir/ build/ verilog.log sample.vcd