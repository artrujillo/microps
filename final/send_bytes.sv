module send_bytes(input  logic clk,
           input  logic sck, 
           input  logic sdi,
           output logic sdo,
           input  logic load,
           output logic done,
		   output logic datastream);

	logic [31:0] orientation;

	rubiks_spi spi(sck, sdi, sdo, done, orientation);
	rubiks_core core(clk, load, orientation, done, datastream);

endmodule


/*
Our spi module does not fully work at this point. We still need to debug using
the logic analyzer to see what exactly is being passed through from the MC to the
FPGA. Once we debug this, we will no longer need to hard code 'orientation' in our 
makesquares module.
*/

// SPI module used to retrieve the current cube orientation from the MC and return
// a DONE signal to the MC once the display has been illuminated
module rubiks_spi(input  logic sck, 
						input  logic sdi,
						output logic sdo,
						input  logic done,
						output logic [31:0] orientation);

    logic         sdodelayed, wasdone;
    logic [31:0]  orientation_captured;
               
    // assert load
    // apply 32 sclks to shift orientation starting with orientation[0]
    // then deassert load, wait until done
    always_ff @(posedge sck)
        if (!wasdone)  {orientation} = {orientation[30:0], sdi};
    
    // sdo should change on the negative edge of sck
    always_ff @(negedge sck) begin
        wasdone = done;
        sdodelayed = orientation[30];
    end
    
    // when done is first asserted, shift out msb before clock edge
    assign sdo = (done & !wasdone) ? orientation[31] : sdodelayed;
endmodule

module rubiks_core(input logic clk, reset,
					input logic [31:0] orientation,
					output logic finished,
					output logic datastream);
		
	typedef enum logic [1:0] {switching, sending, over, finish} statetype;
	statetype state, nextstate;
	
	logic resetsb, done, red;
	logic [23:0] data;
	logic [8:0] count;
	
	make_data_stream mds(clk, resetsb, data, datastream, done);
	
	always_ff @(posedge clk)
		if (reset) begin
			state <= switching;
			red <= 0;
			count <= 0;
		end
		else begin
			state <= nextstate;
			if (state == sending) red <= ~red;
			else count <= count+1;
		end
		
	always_comb 
		case (state)
			switching: nextstate = sending;
			sending:   if (count == 9'd65) nextstate = over;
						  else if (done)     nextstate = switching;
						  else               nextstate = sending;
			over: 	  nextstate = finish;
			finish:    nextstate = finish;
			default:	  nextstate = finish;
						  
		endcase
	
	assign finished = (state == finish);
	assign resetsb = (state == switching);
	//assign data = red ? 24'h00b000 : 24'h00f060; // BB, RR, GG
	//makesquares ms(clk, reset, resetsb, orientation, data);
	 makesquares ms(clk, reset, resetsb, data);
	
endmodule


module makesquares(input  logic clk, reset, switchcolor,
					//input logic [31:0] orientation,
						 output logic[23:0] color);
			
	logic [3:0] count;
	logic [3:0] column, nextcolumn;
	logic [3:0] row, nextrow;
	
	logic switchcolumn, oddcol;
	
	logic [31:0] orientation;
	
	assign orientation = 32'b00000000001010011000100101100000;
	
	logic color1, color2, color3, color4, color5, color6, color7, color8, color9, blank;
	logic [9:0] controlcolors;
	
	always_ff @(posedge clk)
		if (reset) begin
			row <= 4'd9;
			column <= 4'd0;
		end
		else if (switchcolumn & switchcolor) column <= nextcolumn;
		else if (switchcolor) row <= nextrow;
	
	
	// two finite state machines, one switches cols, one switches rows
	// goes through in a snakelike order, order in which LEDs are written
	always_comb 
		case (column)
			4'd0: nextcolumn = 4'd1;
			4'd1: nextcolumn = 4'd2;
			4'd2: nextcolumn = 4'd3;
			4'd3: nextcolumn = 4'd4;
			4'd4: nextcolumn = 4'd5;
			4'd5: nextcolumn = 4'd6;
			4'd6: nextcolumn = 4'd7;
			4'd7: nextcolumn = 4'd8;
			4'd8: nextcolumn = 4'd8;
			default: nextcolumn = 4'd8;
		endcase
		
	always_comb
		case (row)
			4'd9: nextrow = 4'd0;
			4'd0: if (column == 4'd7) nextrow = 4'd8;
					else if (oddcol) nextrow = 4'd0;
					else 		        nextrow = 4'd1;
			4'd1: if (oddcol)  	  nextrow = 4'd0;
					else 				  nextrow = 4'd2;					
			4'd2: if (oddcol)  	  nextrow = 4'd1;
					else 				  nextrow = 4'd3;					
			4'd3: if (oddcol)  	  nextrow = 4'd2;
					else 				  nextrow = 4'd4;					
			4'd4: if (oddcol)  	  nextrow = 4'd3;
					else 				  nextrow = 4'd5;					
			4'd5: if (oddcol)      nextrow = 4'd4;
					else 				  nextrow = 4'd6;				
			4'd6: if (oddcol)      nextrow = 4'd5;
					else 				  nextrow = 4'd7;				
			4'd7: if (oddcol)      nextrow = 4'd6;
					else	 			  nextrow = 4'd7;
			4'd8: nextrow = 4'd8;
			default: nextrow = 4'd8;
		endcase
		
	assign switchcolumn = (oddcol & (row == 4'd0)) | ((~oddcol) & (row == 4'd7));
	
	
	assign oddcol = (column == 4'd1)|(column == 4'd3)|(column == 4'd5)|(column == 4'd7);
	
	
	// this is the correct way
	assign blank = (row == 4'd2)|(row == 4'd5)|(column== 4'd5)|(column== 4'd2);
	assign color1 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd0)|(column== 4'd1));
	assign color2 = ((row == 4'd3)|(row == 4'd4))&((column== 4'd0)|(column== 4'd1));
	assign color3 = ((row == 4'd6)|(row == 4'd7))&((column== 4'd0)|(column== 4'd1));
	assign color4 = ((row == 4'd6)|(row == 4'd7))&((column== 4'd3)|(column== 4'd4));
	assign color5 = ((row == 4'd3)|(row == 4'd4))&((column== 4'd3)|(column== 4'd4));
	assign color6 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd3)|(column== 4'd4));
	assign color7 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd6)|(column== 4'd7));
	assign color8 = ((row == 4'd3)|(row == 4'd4))&((column== 4'd6)|(column== 4'd7));
	assign color9 = ((row == 4'd6)|(row == 4'd7))&((column== 4'd6)|(column== 4'd7));
	
	/*
	assign blank = (row == 4'd7)|(column == 4'd7)|(row == 4'd6)|(column == 4'd6);
	assign color1 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd0)|(column== 4'd1));
	assign color2 = ((row == 4'd2)|(row == 4'd3))&((column== 4'd0)|(column== 4'd1));
	assign color3 = ((row == 4'd4)|(row == 4'd5))&((column== 4'd0)|(column== 4'd1));
	assign color4 = ((row == 4'd4)|(row == 4'd5))&((column== 4'd2)|(column== 4'd3));
	assign color5 = ((row == 4'd2)|(row == 4'd3))&((column== 4'd2)|(column== 4'd3));
	assign color6 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd2)|(column== 4'd3));
	assign color7 = ((row == 4'd0)|(row == 4'd1))&((column== 4'd4)|(column== 4'd5));
	assign color8 = ((row == 4'd2)|(row == 4'd3))&((column== 4'd4)|(column== 4'd5));
	assign color9 = ((row == 4'd4)|(row == 4'd5))&((column== 4'd4)|(column== 4'd5));
	*/
	
	assign controlcolors = {blank, color9, color8, color7, color6, color5, color4, color3, color2, color1};
	colormux cm(controlcolors, orientation, color);
		
endmodule

module convert_orientation(input  logic  [2:0]  bit_value,
									output logic  [23:0] hex_value);

	always_comb
		case (bit_value)
			3'b000:  hex_value = 24'h00b000; // red
			3'b001:  hex_value = 24'h00f060; // orange
			3'b010:  hex_value = 24'h00b0b0; // yellow
			3'b011:  hex_value = 24'h0000b0; // green
			3'b100:  hex_value = 24'hb00000; // blue
			3'b101:  hex_value = 24'hb05000; // purple
			default: hex_value = 24'h000000; // blank
		endcase
									
endmodule

module colormux(input logic  [9:0] colorcontrol,
				input logic [31:0] orientation,
					 output logic [23:0] color);
	logic [23:0] sqr1color, sqr2color, sqr3color, sqr4color, sqr5color, sqr6color, sqr7color, sqr8color, sqr9color;
	
	convert_orientation color1(orientation[2:0], sqr1color);
	convert_orientation color2(orientation[5:3], sqr2color);
	convert_orientation color3(orientation[8:6], sqr3color);
	convert_orientation color4(orientation[11:9], sqr4color);
	convert_orientation color5(orientation[14:12], sqr5color);
	convert_orientation color6(orientation[17:15], sqr6color);
	convert_orientation color7(orientation[20:18], sqr7color);
	convert_orientation color8(orientation[23:21], sqr8color);
	convert_orientation color9(orientation[26:24], sqr9color);
	
	always_comb
		case (colorcontrol)
			10'b0000000001: color = sqr1color;
			10'b0000000010: color = sqr2color;
			10'b0000000100: color = sqr3color;
			10'b0000001000: color = sqr4color;
			10'b0000010000: color = sqr5color;
			10'b0000100000: color = sqr6color;
			10'b0001000000: color = sqr7color;
			10'b0010000000: color = sqr8color;
			10'b0100000000: color = sqr9color;
			10'b1000000000: color = 24'h000000;
			default: color = 24'h000000;
		endcase

endmodule



/////////////////////////////////////////////////////////////
// Takes in 24-bit color data, outputs one bit that follows 
// the pattern detailed here: 
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// for 1s and 0s. Assumes a 40 MHz clock.
/////////////////////////////////////////////////////////////
module make_data_stream(input logic clk, reset,
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
  assign done = (bitcounter == 5'd24 & reset_counter);
  assign reset_counter = (count == counterval);
  assign incbitcounter = (state == T0L)|(state==T1L);
  assign datastream = ((state == T1H)|(state == T0H))&(~reset);
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
