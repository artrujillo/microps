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
    logic clk, reset, datastream;
    
    // device under test
    send_bytes dut(clk, reset, datastream);

    
    // generate clock and load signals
    initial 
        forever begin
            clk = 1'b0; #12500;
            clk = 1'b1; #12500;
        end
        
    initial begin
		reset = 1'b1; #50000;
		reset = 1'b0;
    end 
  
    
endmodule
