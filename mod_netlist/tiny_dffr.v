module tiny_dffr(clk, d, rn, q);
  input clk;
  input d;
  input rn;
  output q;

  DFFR_X1 u_ff (
    .CK(clk),
    .D(d),
    .RN(rn),
    .Q(q)
  );
endmodule
