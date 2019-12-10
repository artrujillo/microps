/////////////////////////////////////////////
// aes.sv
// HMC E155 16 September 2015
// bchasnov@hmc.edu, David_Harris@hmc.edu
/////////////////////////////////////////////

/////////////////////////////////////////////
// testbench
//   Tests AES with cases from FIPS-197 appendix
/////////////////////////////////////////////

module testbench();
    logic clk, reset, datastream, s, megaR;
	 logic [161:0] orientation1,orientation2,orientation;
	 
	 assign orientation1 = 162'h12492492492492492492492492492492492492492;
	 assign orientation2 = 162'h24924924924924924924924924924924924924924;
    
	 assign orientation = s ? orientation1 : orientation2;
	 
    // device under test
    rubiks_core dut(clk, reset, orientation, datastream, megaR);

    
    // generate clock and load signals
    initial 
        forever begin
            clk = 1'b0; #12500;
            clk = 1'b1; #12500;
        end
        
    initial begin
		reset = 1'b1; #50000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;

		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;
		reset = 1'b0; #1000000000;

		reset = 1'b1;
		s = 1'b1;	  #50000;
		reset = 1'b0;
    end 
  
    
endmodule
