// top level module, contains spi and core modules
module send_bytes(input  logic clk,
                  input  logic reset,
                  input  logic sck, 
                  input  logic sdi,
                  input  logic load,
                  output logic datastream);

	logic [431:0] orientation, hardcoded;

	assign hardcoded = 432'b000001010000010100000101000001010000010100000101000001010000010100000101000001000000010000000100000001000000010000000100000001000000010000000100000000110000001100000011000000110000001100000011000000110000001100000011000000100000001000000010000000100000001000000010000000100000001000000010000000010000000100000001000000010000000100000001000000010000000100000001000000000000000000000000000000000000000000000000000000000000000000000000;

	rubiks_spi spi(sck, sdi, load, orientation);
	rubiks_core core(clk, reset, hardcoded, datastream);

endmodule

// SPI module used to retrieve the current cube orientation from the MC and return
// a DONE signal to the MC once the display has been illuminated
module rubiks_spi(input  logic sck, 
                  input  logic sdi,
                  input  logic load,
                  output logic [431:0] orientation);
 
   // assert load
   // apply 72 sclks to shift orientation starting with orientation[0]
   always_ff @(posedge sck)
      if (load) {orientation} = {orientation[430:0], sdi};
 
endmodule

// programs a rubiks face with colors given by orientation
module rubiks_core(input  logic clk, reset,
                   input  logic [431:0] orientation,
                   output logic datastream);
	
	typedef enum logic [1:0] {switching, sending, new_face, over} statetype;
	statetype state, nextstate;
	
	logic resetsb, done, red, change_face, face_reset;
	logic [71:0] current_face_orientation;
	logic [23:0] data;
	logic [8:0] count;
	logic [2:0] face_count;
	
	assign face_reset = reset ^ change_face;
	// register for finite state machine, counter
	always_ff @(posedge clk)
	   if (face_reset) begin
	                 state <= switching;
	                 count <= 0;
					 face_count <= 0;
					 change_face <= 0;
	              end
	   else if (state == new_face) begin
                                      face_count <= face_count + 1;
									  state <= nextstate;
									  change_face <= 1;
								   end
	   else       begin
	                 state <= nextstate;
					 change_face <= 0;
						  face_count <= face_count;
	                 if (state == sending) red <= ~red; // for testing
	                 else
						  count <= count+1; // counter for how many LEDs have been programmed
	              end
	
	// next state logic for finite state machine
	// sending: waits for make_data_stream to send 24 bits
	// switching: updates data to be a new 24 color bits
	// over and finish: show that this face is done writing so that we can write the next one
	always_comb 
      case (state)
	      switching: nextstate = sending;
	      sending:  if      (count == 9'd65)       nextstate = new_face;
	                else if (done)                 nextstate = switching;
	                else                           nextstate = sending;
		  new_face: if      (face_count == 3'b101) nextstate = over; // may need to make this 3'b110 -- will test
		            else                           nextstate = switching;													  
	      over:                                    nextstate = over;
	      default:	                               nextstate = over;
	
	   endcase

	// control logic
	assign resetsb = (state == switching);

	// determine the current face orientation to be displayed
	face_fsm get_face(clk, reset, change_face, orientation, current_face_orientation);
	
	// get the 24-bit color data from make squares
	makesquares ms(clk, face_reset, resetsb, current_face_orientation, data); 
	
	// make the datastream based on the 24 bits of color data
	make_data_stream mds(clk, resetsb, data, datastream, done);
	
endmodule

// finite state machine that determines which face we are currently lighting up
module face_fsm(input  logic clk, reset, change_face,
                input  logic [431:0] full_orientation,
                output logic [71:0] current_face_orientation);
	
   logic [71:0] blank_face;
	
	assign blank_face = 72'b0;
   typedef enum logic [3:0] {red_face, orange_face, yellow_face, green_face, blue_face, purple_face, over} statetype;
   
	statetype state, nextstate;

	always_ff @(posedge clk)
	   if (reset) state <= red_face;
	   else       state <= nextstate;
	
	always_comb
	   case(state)
	     red_face:    if (change_face) begin
		                                   nextstate = orange_face;
													  current_face_orientation = blank_face;
												  end
		               else begin            
							                 nextstate = red_face;
												  current_face_orientation = full_orientation[71:0];
								  end
	     orange_face: if (change_face) begin
		                                   nextstate = yellow_face;
													  current_face_orientation = blank_face;
												  end
		               else begin            
							                 nextstate = orange_face;
												  current_face_orientation = full_orientation[143:72];
								  end
		  yellow_face: if (change_face) begin
		                                   nextstate = green_face;
													  current_face_orientation = blank_face;
												  end
		               else begin            
							                 nextstate = yellow_face;
												  current_face_orientation = full_orientation[215:144];
								  end
		  green_face:  if (change_face) begin
		                                   nextstate = blue_face;
													  current_face_orientation = blank_face;
												  end
		               else begin            
							                 nextstate = green_face;
												  current_face_orientation = full_orientation[287:216];
								  end
		  blue_face:   if (change_face) begin
		                                   nextstate = purple_face;
													  current_face_orientation = blank_face;
												  end
		               else begin            
							                 nextstate = blue_face;
												  current_face_orientation = full_orientation[359:288];
								  end
		  purple_face: if (change_face) begin
		                                   nextstate = over;
													  current_face_orientation = blank_face;
												  end
		               else begin           
							                 nextstate = purple_face;
												  current_face_orientation = full_orientation[431:360];
								  end
		  over:             begin       
		                                nextstate = over;
		                                current_face_orientation = blank_face;
								  end
		  default:                      nextstate = over;
	   endcase
	
endmodule

// outputs colors in correct order to display squares for rubiks cube
module makesquares(input  logic clk, reset, switchcolor,
                   input  logic [71:0] orientation,
                   output logic [23:0] color);
	
	logic [3:0] count;
	logic [3:0] column, nextcolumn;
	logic [3:0] row, nextrow;
	logic switchcolumn, oddcol;
	
	// control logic for choosing correct color
	logic color1, color2, color3, color4, color5, color6, color7, color8, color9, blank;
	logic [9:0] controlcolors;
	
	always_ff @(posedge clk)
	   if      (reset) begin
	                                        row <= 4'd0; // Changing this to a 0 instead of a 9 fixed the odd alignment??
	                                     column <= 4'd0;
	                   end
	   else if (switchcolumn & switchcolor) column <= nextcolumn;
	   else if (switchcolor)                   row <= nextrow;
	
	// two finite state machines, one switches cols, one switches rows
	// goes through in a snakelike order, order in which LEDs are written
	always_comb 
	   case (column)
	      4'd0:    nextcolumn = 4'd1;
	      4'd1:    nextcolumn = 4'd2;
	      4'd2:    nextcolumn = 4'd3;
	      4'd3:    nextcolumn = 4'd4;
	      4'd4:    nextcolumn = 4'd5;
	      4'd5:    nextcolumn = 4'd6;
	      4'd6:    nextcolumn = 4'd7;
	      4'd7:    nextcolumn = 4'd8;
	      4'd8:    nextcolumn = 4'd8;
	      default: nextcolumn = 4'd8;
	endcase
	
	always_comb
	   case (row)
	      4'd9: nextrow = 4'd0; // buffer state to handle reset case
	      4'd0: if      (column == 4'd7) nextrow = 4'd8;
	            else if (oddcol)         nextrow = 4'd0;
	            else                     nextrow = 4'd1;
	      4'd1: if      (oddcol)         nextrow = 4'd0;
	            else                     nextrow = 4'd2;	
	      4'd2: if      (oddcol)         nextrow = 4'd1;
	            else                     nextrow = 4'd3;	
	      4'd3: if      (oddcol)         nextrow = 4'd2;
	            else                     nextrow = 4'd4;	
	      4'd4: if      (oddcol)         nextrow = 4'd3;
	            else                     nextrow = 4'd5;	
	      4'd5: if      (oddcol)         nextrow = 4'd4;
	            else                     nextrow = 4'd6;	
	      4'd6: if      (oddcol)         nextrow = 4'd5;
	            else                     nextrow = 4'd7;	
	      4'd7: if      (oddcol)         nextrow = 4'd6;
	            else	                   nextrow = 4'd7;
	      4'd8:                          nextrow = 4'd8;
	      default:                       nextrow = 4'd8;
	   endcase
	
	// control logic 
	assign switchcolumn = (oddcol & (row == 4'd0)) | ((~oddcol) & (row == 4'd7));
	assign oddcol = (column == 4'd1)|(column == 4'd3)|(column == 4'd5)|(column == 4'd7);
	
	// color bit logic 
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
	
	/* experimenting with another way we could display a face
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
	// choose the color data based on the color mux
	assign controlcolors = {blank, color9, color8, color7, color6, color5, color4, color3, color2, color1};
	colormux cm(controlcolors, orientation, color);
	
endmodule

// takes in 3 bits of current orientation and converts them to the 
// corresponding HEX values that we need to illuminate the matrix
module convert_orientation(input  logic [7:0] bit_value,
                           output logic [23:0] hex_value);

	always_comb
	case (bit_value)
		8'b00000000: hex_value =  24'h00b000; // red
		8'b00000001: hex_value =  24'h00f060; // orange
		8'b00000010: hex_value =  24'h00b0b0; // yellow
		8'b00000011: hex_value =  24'h0000b0; // green
		8'b00000100: hex_value =  24'hb00000; // blue
		8'b00000101: hex_value =  24'hb05000; // purple
		default:     hex_value = 24'h000000; // blank
	endcase
	
endmodule

// takes in the current orientation as well as a one-hot encoding that 
// allows us to illuminate the matrix properly
module colormux(input  logic [9:0] colorcontrol,
                input  logic [71:0] orientation,
                output logic [23:0] color);

	logic [23:0] sqr1color, sqr2color, sqr3color, sqr4color, sqr5color, sqr6color, sqr7color, sqr8color, sqr9color;
	
	// convert each necessary piece of the orientation into the proper
	// HEX value for the square that the color corresponds to
	convert_orientation color1(orientation[7:0], sqr1color);
	convert_orientation color2(orientation[15:8], sqr2color);
	convert_orientation color3(orientation[23:16], sqr3color);
	convert_orientation color4(orientation[31:24], sqr4color);
	convert_orientation color5(orientation[39:32], sqr5color);
	convert_orientation color6(orientation[47:40], sqr6color);
	convert_orientation color7(orientation[55:48], sqr7color);
	convert_orientation color8(orientation[63:56], sqr8color);
	convert_orientation color9(orientation[71:64], sqr9color);
	
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
	      default:        color = 24'h000000;
	   endcase

endmodule

/////////////////////////////////////////////////////////////
// Takes in 24-bit color data, outputs one bit that follows 
// the pattern detailed here: 
// https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
// for 1s and 0s. Assumes a 40 MHz clock.
/////////////////////////////////////////////////////////////
module make_data_stream(input  logic clk, reset,
                        input  logic [23:0] data,
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
      if      (reset | reset_counter) count <= 0;
      else if (~done)                 count <= count+1;

   // register for bitcounter
   always_ff @(posedge clk)
      if      (reset)                                 bitcounter <= 0;
      else if (reset_counter & incbitcounter & ~done) bitcounter <= bitcounter+1;

   // register for FSM
   always_ff @(posedge clk)
      if (reset) begin
         if (currentbit) state <= T1H;
         else            state <= T0H;
      end
      else               state <= nextstate;

   // nextstate logic
   // T0H: high pulse for writing a 0
   // T1H: high pulse for writing a 1
   // T0L: low pulse for writing a 0
   // T1L: low pulse for writing a 1
   // R: reset to indicate shift
   always_comb
      case (state)
         T0H: if      (~reset_counter)       nextstate = T0H;
              else                           nextstate = T0L;
         T1H: if      (~reset_counter)       nextstate = T1H;
              else                           nextstate = T1L;
         T0L: if      (~reset_counter)       nextstate = T0L;
              else if (nextbit)              nextstate = T1H;
	          else if (bitcounter == 5'd23)  nextstate = R;
              else                           nextstate = T0H;
         T1L: if      (~reset_counter)       nextstate = T1L;
              else if (nextbit)              nextstate = T1H;
              else if (bitcounter == 5'd23)  nextstate = R;
              else                           nextstate = T0H;
         R:   if      (~reset_counter)       nextstate = R;
              else if (bitcounter == 5'd24)  nextstate = R;
              else if (nextbit)              nextstate = T1H;
              else                           nextstate = T0H;
         default:                            nextstate = R;
      endcase

   // control signal logic 
   assign currentbit = data[bitcounter]; // current bit that is being written
   assign nextbit = data[bitcounter+1]; // get the next bit to write
   assign done = (bitcounter == 5'd24 & reset_counter); // when all 24 bits have been written, single pulse
   assign reset_counter = (count == counterval); // switch to next state, and reset counter
   assign incbitcounter = (state == T0L)|(state==T1L); // when the bit counter should be updated, which is low pulse
   assign datastream = ((state == T1H)|(state == T0H))&(~reset); // data stream is high when we are in high pulse states
   assign s = {{state==R},(state==T0L)|(state==T1L), (state==T1H)|(state==T1L)}; // input to mux to choose constants
   countervalmux cntrvalmux(s, counterval); // mux chooses constants, depending on how long the pulse should be

endmodule

// mux for choosing counter value, depending on state
module countervalmux(input logic [2:0] s,
   output logic [10:0] out);

   always_comb
      case (s)
         3'b000:  out = 11'd16; // T0H
         3'b001:  out = 11'd32; // T1H
         3'b010:  out = 11'd34; // T0L
         3'b011:  out = 11'd18; // T1L
         3'b100:  out = 11'd2000; // R
	      default: out = 11'd2000; // R
      endcase

endmodule

module mux2 (input  logic d0, d1, s,
				 output logic y);
	assign y = s ? d1 : d0;
endmodule
