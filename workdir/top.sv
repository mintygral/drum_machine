module top (
  // I/O ports
  input  logic hz2m, hz100, reset,
  input  logic [20:0] pb,
  /* verilator lint_off UNOPTFLAT */
  output logic [7:0] left, right,
         ss7, ss6, ss5, ss4, ss3, ss2, ss1, ss0,
  output logic red, green, blue,

  // UART ports
  output logic [7:0] txdata,
  input  logic [7:0] rxdata,
  output logic txclk, rxclk,
  input  logic txready, rxready
);

endmodule
