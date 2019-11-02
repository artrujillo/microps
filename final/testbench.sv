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
    logic clk, reset, done, datastream;
    logic [23:0] data;
    
    // device under test
    send_bytes dut(clk, reset, data, datastream, done);
    
    // test case
    initial begin   
// Test case from FIPS-197 Appendix A.1, B
        data   <= 24'b001100110011001100110011;
    end
    
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