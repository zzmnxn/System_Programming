`timescale 1ns / 1ps

module mealy_tb;

   
    reg clk, rst, in;
    wire out;
    wire [3:0] seq;

    
    mealy u_mealy (
        .clk(clk),
        .rst(rst),
        .in(in),
        .out(out),
        .seq(seq)
    );

    
    initial begin
        clk = 1'b0;
        rst = 1'b1;  
        in = 1'b0;
        #50 rst = 1'b0;  
    end

 
    always #10 clk = ~clk;  

    
    initial begin
        
        in = 1'b0;

        
        #20 in = 1'b1;  
        #20 in = 1'b1;  
        #20 in = 1'b0;  
        #20 in = 1'b1;  
        #20 in = 1'b0;  
        #20 in = 1'b1;  
        #20 in = 1'b1;  
        #20 in = 1'b0;  
        #20 in = 1'b1;  
        #20 in = 1'b0;  
        #20 in = 1'b0;  
        #20 in = 1'b1;  
        #20 in = 1'b1; 
        #20 in = 1'b0;  
    end

 
    initial begin
        #500;  
        $finish;
    end

endmodule
