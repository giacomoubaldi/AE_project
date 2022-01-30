
module IOFifosControl (
  input clk,
  input reset,

  input      [31:0] avs_writedata,
  output reg [31:0] avs_readdata,
  input      [2:0]  avs_address,
  input             avs_read,
  input             avs_write,

  output [31:0] InFifoData,
  output        InFifoWriteReq,
  input  [31:0] InFifoStatus,
  input  [31:0] OutFifoData,
  output        OutFifoReadAck,
  input  [31:0] OutFifoStatus,
  input  [31:0] OutRegFifoData,
  output        OutRegFifoReadAck,
  input  [31:0] OutRegFifoStatus

);

reg avs_writeold;
reg avs_readold;
reg [31:0] InFifoData_reg;
reg        InFifoWriteReq_reg;
reg        OutFifoReadAck_reg;
reg        OutRegFifoReadAck_reg;

assign InFifoData = InFifoData_reg;
assign InFifoWriteReq = InFifoWriteReq_reg;
assign OutFifoReadAck = OutFifoReadAck_reg;

assign OutRegFifoReadAck = OutRegFifoReadAck_reg;

always @(posedge clk) begin
  if ( (avs_write==1) && (avs_writeold==0) )begin
  	OutFifoReadAck_reg <= 0;
  	OutRegFifoReadAck_reg <= 0;
    case (avs_address)
      3'b000: begin			    // address 0 is the InFifo DATA
  	            InFifoWriteReq_reg <= 1;    // this is the only writable address
                InFifoData_reg <= avs_writedata;
              end
      default: begin
  	             InFifoWriteReq_reg <= 0;
                 InFifoData_reg <= 32'd0;
               end
    endcase
  end
  else if ( (avs_read==1) && (avs_readold==0) )begin
    InFifoWriteReq_reg <= 0;
    OutFifoReadAck_reg <= 0;
    OutRegFifoReadAck_reg <= 0;
    case (avs_address)
      3'b000: begin  // address 0 is the InFifo DATA
                avs_readdata <= InFifoData_reg;
              end 
      3'b001: begin  // address 1 is the InFifo Status
                avs_readdata <= InFifoStatus;
              end 
      3'b010: begin  // address 2 is the OutFifo DATA
                avs_readdata <= OutFifoData;
                OutFifoReadAck_reg <= 1;
              end 
      3'b011: begin  // address 3 is the OutFifo Status
                avs_readdata <= OutFifoStatus;
              end 
      3'b100: begin  // address 4 is the OutRegFifo DATA
                avs_readdata <= OutRegFifoData;
                OutRegFifoReadAck_reg <= 1;
              end 
      3'b101: begin  // address 5 is the OutRegFifo Status
                avs_readdata <= OutRegFifoStatus;
              end 
      default: avs_readdata <= 32'd0;
    endcase
  end
  else begin
  	InFifoWriteReq_reg <= 0;
  	OutFifoReadAck_reg <= 0;
  	OutRegFifoReadAck_reg <= 0;
  end
  avs_readold <= avs_read;
  avs_writeold <= avs_write;
end

endmodule
