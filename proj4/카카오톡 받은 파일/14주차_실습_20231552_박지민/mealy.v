`timescale 1ns / 1ps

module mealy(
input clk, rst, in,
output reg out,
output reg [3:0]  seq
);


reg[3:0] target = 4'b1101;

initial out = 1'b0; 

always @(posedge clk) begin
    if(rst) begin
        seq <= 4'b0000;
        out <= 1'b0;
    end
    else begin
        seq[3] = seq[2];
        seq[2] = seq[1];
        seq[1] = seq[0];
        seq[0] = in;
        if(seq == target) 
        out = 1'b1;
        else 
        out = 1'b0;
    end 
end

endmodule