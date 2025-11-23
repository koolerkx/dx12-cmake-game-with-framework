#include "basic_type.hlsli"

BasicType main(VSIN input) {
  BasicType output;
  output.svpos = input.pos;
  output.uv = input.uv;
  return output;
}