// DESCRIPTION: Verilator: Verilog Test module
//
// Copyright 2010 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.

`ifdef VERILATOR
//We call it via $c so we can verify DPI isn't required - see bug572
`else
import "DPI-C" context function integer mon_check();
`endif

module t (/*AUTOARG*/
   // Inputs
   clk
   );

`ifdef VERILATOR
`systemc_header
extern "C" int mon_check();
`verilog
`endif

   input clk;

   reg [31:0] mem0 [1:16] /*verilator public_flat_rw @(posedge clk) */;

   integer 	  status;

   // Test loop
   initial begin
`ifdef VERILATOR
      status = $c32("mon_check()");
`else
      status = mon_check();
`endif
      for (integer i = 16; i > 0; i--)
	if (mem0[i] != i) $write("%%Error: %d : GOT = %d  EXP = %d\n", i, mem0[i], i);
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule : t
