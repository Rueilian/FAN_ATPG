module tiny_dffr(CK, d, rn, q, test_si, test_so, test_se);
  input test_si;
  input test_se;
  output test_so;
  input CK;
  input d;
  input rn;
  output q;

  SDFFR_X1 u_ff (
    .CK(CK),
    .D(d),
    .RN(rn),
    .Q(q),
    .SE(test_se),
    .SI(test_si)
  );
  assign test_so = q;
endmodule
