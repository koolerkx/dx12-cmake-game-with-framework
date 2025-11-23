struct VSOUT {
  float4 pos : POSITION;
  float4 svpos : SV_POSITION;
};

VSOUT main(float4 pos : POSITION) {
  VSOUT output;
  output.pos = pos;
  output.svpos = pos;
  return output;
}