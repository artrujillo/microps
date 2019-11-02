/*
module aes_spi(input  logic sck, 
               input  logic sdi,
               output logic sdo,
               input  logic done,
               output logic [23:0] key, plaintext,
               input  logic [127:0] cyphertext);

    logic         sdodelayed, wasdone;
    logic [127:0] cyphertextcaptured;
               
    // assert load
    // apply 256 sclks to shift in key and plaintext, starting with plaintext[0]
    // then deassert load, wait until done
    // then apply 128 sclks to shift out cyphertext, starting with cyphertext[0]
    always_ff @(posedge sck)
        if (!wasdone)  {plaintext, key} = {plaintext[126:0], key, sdi};
        else           {plaintext, key} = {plaintext, key, sdi}; 
    
    // sdo should change on the negative edge of sck
    always_ff @(negedge sck) begin
        wasdone = done;
        sdodelayed = cyphertextcaptured[126];
    end
    
    // when done is first asserted, shift out msb before clock edge
    assign sdo = (done & !wasdone) ? cyphertext[127] : sdodelayed;
endmodule
*/

/////////////////////////////////////////////////////////////
// Takes in 24-bit color data, outputs one bit that follows 
// the pattern detailed here: 
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// for 1s and 0s. Assumes a 40 MHz clock.
/////////////////////////////////////////////////////////////
module send_bytes(input logic clk, reset,
                  input logic [23:0] data,
                  output logic datastream, done);

  // counter logic
  logic [10:0] counterval;
  logic [4:0] bitcounter;
  logic [10:0] count;

  // control logic
  logic currentbit, nextbit;
  logic [2:0] s;
  logic reset_counter, incbitcounter;

  // state type
  typedef enum logic [2:0] {T0H, T1H, T0L, T1L, R} statetype;
  statetype state, nextstate;

  // register for main counter
  always_ff @(posedge clk)
    if (reset | reset_counter) count <= 0;
    else if (~done) count <= count+1;

  // register for bitcounter
  always_ff @(posedge clk)
    if (reset) bitcounter <= 0;
    else if (reset_counter & incbitcounter & ~done) bitcounter <= bitcounter+1;

  // register for FSM
  always_ff @(posedge clk)
    if (reset) begin
      if (currentbit) state <= T1H;
      else state <= T0H;
    end
    else state <= nextstate;

  // nextstate logic
  always_comb
    case (state)
      T0H: if (~reset_counter) nextstate = T0H;
           else nextstate = T0L;
      T1H: if (~reset_counter) nextstate = T1H;
           else nextstate = T1L;
      T0L: if (~reset_counter) nextstate = T0L;
           else if (nextbit) nextstate = T1H;
			  else if (bitcounter == 5'd23) nextstate = R;
           else nextstate = T0H;
      T1L: if (~reset_counter) nextstate = T1L;
           else if (nextbit) nextstate = T1H;
			  else if (bitcounter == 5'd23) nextstate = R;
           else nextstate = T0H;
      R:   if (~reset_counter) nextstate = R;
           else if (bitcounter == 5'd24) nextstate = R;
           else if (nextbit) nextstate = T1H;
           else nextstate = T0H;
      default: nextstate = R;
    endcase

  // control signal logic 
  assign currentbit = data[bitcounter];
  assign nextbit = data[bitcounter+1];
  assign done = (bitcounter == 5'd24 & resetcounter);
  assign reset_counter = (count == counterval);
  assign incbitcounter = (state == T0L)|(state==T1L);
  assign datastream = (state == T1H)|(state == T0H);
  assign s = {{state==R},(state==T0L)|(state==T1L), (state==T1H)|(state==T1L)};
  countervalmux cntrvalmux(s, counterval);

endmodule


// mux for choosing counter value, depending on state
module countervalmux(input  logic  [2:0] s,
                     output logic [10:0] out);

  always_comb
    case (s)
      3'b000: out = 11'd16;
      3'b001: out = 11'd32;
      3'b010: out = 11'd34;
      3'b011: out = 11'd18;
      3'b100: out = 11'd2000;
		default: out = 11'd2000;
    endcase

endmodule
