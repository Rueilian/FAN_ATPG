module s27(CK, G0, G1, G2, G3, G17, test_si, test_so, test_se);
  input test_si;
  input test_se;
  output test_so;

  input CK;
  input G0;
  input G1;
  input G2;
  input G3;
  output G17;
  wire G5;
  wire G10;
  wire G6;
  wire G11;
  wire G7;
  wire G13;
  wire G14;
  wire G8;
  wire G15;
  wire G12;
  wire G16;
  wire G9;
  wire _qn_DFF_0_;
  wire _qn_DFF_1_;
  wire _qn_DFF_2_;
  wire vdd;

  SDFFR_X1 DFF_0 (.CK(CK), .D(G10), .Q(G5), .QN(_qn_DFF_0_), .RN(vdd),
    .SE(test_se),
    .SI(test_si)
  );
  SDFFR_X1 DFF_1 (.CK(CK), .D(G11), .Q(G6), .QN(_qn_DFF_1_), .RN(vdd),
    .SE(test_se),
    .SI(G5)
  );
  SDFFR_X1 DFF_2 (.CK(CK), .D(G13), .Q(G7), .QN(_qn_DFF_2_), .RN(vdd),
    .SE(test_se),
    .SI(G6)
  );
  LOGIC1_X1 _vdd_tie_ (.Z(vdd));
  INV_X1 NOT_0 (.ZN(G14), .A(G0));
  INV_X1 NOT_1 (.ZN(G17), .A(G11));
  AND2_X1 AND2_0 (.ZN(G8), .A1(G14), .A2(G6));
  OR2_X1 OR2_0 (.ZN(G15), .A1(G12), .A2(G8));
  OR2_X1 OR2_1 (.ZN(G16), .A1(G3), .A2(G8));
  NAND2_X1 NAND2_0 (.ZN(G9), .A1(G16), .A2(G15));
  NOR2_X1 NOR2_0 (.ZN(G10), .A1(G14), .A2(G11));
  NOR2_X1 NOR2_1 (.ZN(G11), .A1(G5), .A2(G9));
  NOR2_X1 NOR2_2 (.ZN(G12), .A1(G1), .A2(G7));
  NOR2_X1 NOR2_3 (.ZN(G13), .A1(G2), .A2(G12));
  assign test_so = G7;
endmodule
