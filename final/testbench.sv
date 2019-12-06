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
    logic clk, sck, sdi, load, datastream;
    
    // device under test
    send_bytes dut(clk, sck, sdi, load, datastream);

    
    // generate clock and load signals
    initial 
        forever begin
            clk = 1'b0; #12500;
            clk = 1'b1; #12500;
        end
        
    initial begin
		load = 1'b1; #50000;
		load = 1'b0;
    end 
  
    
endmodule
