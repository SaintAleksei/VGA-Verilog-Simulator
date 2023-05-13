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

  localparam HOR_BACK_PORCH_START = 192;
  localparam HOR_DATA_START  = HOR_BACK_PORCH_START + 304;
  localparam HOR_FRONT_START = HOR_DATA_START + 1600;
  localparam HOR_MAX         = HOR_FRONT_START + 64;
  localparam VER_BACK_PORCH_START = 3;
  localparam VER_DATA_START  = HOR_BACK_PORCH_START + 46;
  localparam VER_FRONT_START = HOR_DATA_START + 1200;
  localparam VER_MAX         = VER_FRONT_START + 1;


  assign r = hcnt[7:0];
  assign b = vcnt[7:0];
  assign g = 8'b0;

  wire hsync = (hcnt >= HOR_BACK_PORCH_START);
  wire vsync = (vcnt >= VER_BACK_PORCH_START);

  always @(posedge clk) 
  begin
    hcnt <= (hcnt == HOR_MAX - 1) ? 0 : hcnt + 1;
    vcnt <= (hcnt != HOR_MAX - 1) ? vcnt :
            (vcnt == VER_MAX - 1) ? 0 : vcnt + 1;
  end

  initial 
  begin
    hcnt = 0;
    vcnt = 0;
    clk = 0;

    $vgasim(
      "1600x1200_60Hz",
      r, g, b,
      hsync, vsync,
      clk
    );
  end

  always #1 clk = ~clk;
endmodule
