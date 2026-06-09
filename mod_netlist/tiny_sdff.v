module tiny_sdff(CK, d, q, test_si, test_so, test_se);
  input test_si;
  input test_se;
  output test_so;
  input CK;
  input d;
  output q;

  SDFF_X1 u_ff (
    .CK(CK),
    .D(d),
    .Q(q),
    .SE(test_se),
    .SI(test_si)
  );
  assign test_so = q;
endmodule
