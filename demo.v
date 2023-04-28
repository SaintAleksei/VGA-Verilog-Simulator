// This is a simulation-only example of generating a VGA-like pixel clock video
// signal.
`timescale 1ns/1ns

module vgasimdemo;
  reg clk;
  wire [7:0] r;
  wire [7:0] g;
  wire [7:0] b;
  reg [15:0] hcnt;
  reg [15:0] vcnt;

  localparam HOR_BACK_PORCH_START = 96;
  localparam HOR_DATA_START  = HOR_BACK_PORCH_START + 48;
  localparam HOR_FRONT_START = HOR_DATA_START + 640;
  localparam HOR_MAX         = HOR_FRONT_START + 16;
  localparam VER_BACK_PORCH_START = 2;
  localparam VER_DATA_START  = HOR_BACK_PORCH_START + 33;
  localparam VER_FRONT_START = HOR_DATA_START + 480;
  localparam VER_MAX         = VER_FRONT_START + 10;


  assign r = hcnt[7:0];
  assign g = hcnt[7:0];
  assign b = 8'b0;

  wire hsync = (hcnt >= HOR_BACK_PORCH_START);
  wire vsync = (vcnt >= VER_BACK_PORCH_START);

  always @(posedge clk) 
  begin
    hcnt <= (hcnt == HOR_MAX - 1) ? 0 : hcnt + 1;
    vcnt <= (hcnt != HOR_MAX - 1) ? vcnt :
            (vcnt == VER_MAX - 1) ? 0 : vcnt + 1;
    //$display("demo: %d;%d;%d;%d\n", hcnt, vcnt, hsync, vsync);
  end

  initial 
  begin
    hcnt = 0;
    vcnt = 0;
    clk = 0;

    $vgasimInit(
      640,
      480,
      r, g, b,
      hsync, vsync,
      clk
    );
  end

  always #1 clk = ~clk;
endmodule
