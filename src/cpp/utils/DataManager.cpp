/*
 * Copyright (c) 2024 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "DataManager.h"
#include "../Errors.h"

namespace pongasoft::utils {

static constexpr int Decode85Byte(unsigned char c) { return c >= '\\' ? c-36 : c-35; }

static void Decode85(const unsigned char* src, unsigned char* dst)
{
  while (*src)
  {
    unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
    dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
    src += 5;
    dst += 4;
  }
}

static unsigned int stb_decompress_length(const unsigned char *input)
{
  return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

#define stbIn2(x)   ((i[x] << 8) + i[(x)+1])
#define stbIn3(x)   ((i[x] << 16) + stbIn2((x)+1))
#define stbIn4(x)   ((i[x] << 24) + stbIn3((x)+1))

struct stb_decompress_context
{
  unsigned char *barrier_out_e{};
  unsigned char *barrier_out_b{};
  const unsigned char *barrier_in_b{};
  unsigned char *dout{};
};

//static unsigned char *barrier_out_e, *barrier_out_b;
//static const unsigned char *barrier_in_b;
//static unsigned char *dout;

static void stbMatch(stb_decompress_context &ctx, const unsigned char *data, unsigned int length)
{
  // INVERSE of memmove... write each byte before copying the next...
  WST_INTERNAL_ASSERT(ctx.dout + length <= ctx.barrier_out_e);
  if (ctx.dout + length > ctx.barrier_out_e) { ctx.dout += length; return; }
  if (data < ctx.barrier_out_b) { ctx.dout = ctx.barrier_out_e+1; return; }
  while (length--) *ctx.dout++ = *data++;
}

static void stbLit(stb_decompress_context &ctx, const unsigned char *data, unsigned int length)
{
  WST_INTERNAL_ASSERT(ctx.dout + length <= ctx.barrier_out_e);
  if (ctx.dout + length > ctx.barrier_out_e) { ctx.dout += length; return; }
  if (data < ctx.barrier_in_b) { ctx.dout = ctx.barrier_out_e+1; return; }
  memcpy(ctx.dout, data, length);
  ctx.dout += length;
}

static const unsigned char *stb_decompress_token(stb_decompress_context &ctx, const unsigned char *i)
{
  if (*i >= 0x20) { // use fewer if's for cases that expand small
    if (*i >= 0x80)       stbMatch(ctx, ctx.dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
    else if (*i >= 0x40)  stbMatch(ctx, ctx.dout-(stbIn2(0) - 0x4000 + 1), i[2]+1), i += 3;
    else /* *i >= 0x20 */ stbLit(ctx, i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
  } else { // more ifs for cases that expand large, since overhead is amortized
    if (*i >= 0x18)       stbMatch(ctx, ctx.dout-(stbIn3(0) - 0x180000 + 1), i[3]+1), i += 4;
    else if (*i >= 0x10)  stbMatch(ctx, ctx.dout-(stbIn3(0) - 0x100000 + 1), stbIn2(3)+1), i += 5;
    else if (*i >= 0x08)  stbLit(ctx, i+2, stbIn2(0) - 0x0800 + 1), i += 2 + (stbIn2(0) - 0x0800 + 1);
    else if (*i == 0x07)  stbLit(ctx, i+3, stbIn2(1) + 1), i += 3 + (stbIn2(1) + 1);
    else if (*i == 0x06)  stbMatch(ctx, ctx.dout-(stbIn3(1)+1), i[4]+1), i += 5;
    else if (*i == 0x04)  stbMatch(ctx, ctx.dout-(stbIn3(1)+1), stbIn2(4)+1), i += 6;
  }
  return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
  const unsigned long ADLER_MOD = 65521;
  unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
  unsigned long blocklen = buflen % 5552;

  unsigned long i;
  while (buflen) {
    for (i=0; i + 7 < blocklen; i += 8) {
      s1 += buffer[0], s2 += s1;
      s1 += buffer[1], s2 += s1;
      s1 += buffer[2], s2 += s1;
      s1 += buffer[3], s2 += s1;
      s1 += buffer[4], s2 += s1;
      s1 += buffer[5], s2 += s1;
      s1 += buffer[6], s2 += s1;
      s1 += buffer[7], s2 += s1;

      buffer += 8;
    }

    for (; i < blocklen; ++i)
      s1 += *buffer++, s2 += s1;

    s1 %= ADLER_MOD, s2 %= ADLER_MOD;
    buflen -= blocklen;
    blocklen = 5552;
  }
  return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
  stb_decompress_context ctx{};

  if (stbIn4(0) != 0x57bC0000) return 0;
  if (stbIn4(4) != 0)          return 0; // error! stream is > 4GB
  const unsigned int olen = stb_decompress_length(i);
  ctx.barrier_in_b = i;
  ctx.barrier_out_e = output + olen;
  ctx.barrier_out_b = output;
  i += 16;

  ctx.dout = output;
  for (;;) {
    const unsigned char *old_i = i;
    i = stb_decompress_token(ctx, i);
    if (i == old_i) {
      if (*i == 0x05 && i[1] == 0xfa) {
        WST_INTERNAL_ASSERT(ctx.dout == output + olen);
        if (ctx.dout != output + olen) return 0;
        if (stb_adler32(1, output, olen) != (unsigned int) stbIn4(2))
          return 0;
        return olen;
      } else {
        WST_INTERNAL_ASSERT(0); /* NOTREACHED */
        return 0;
      }
    }
    WST_INTERNAL_ASSERT(ctx.dout <= output + olen);
    if (ctx.dout > output + olen)
      return 0;
  }
}

//------------------------------------------------------------------------
// DataManager::loadCompressedBase85
//------------------------------------------------------------------------
std::vector<unsigned char> DataManager::loadCompressedBase85(char const *iCompressedBase85)
{
  // base85 => compressed binary
  auto compressedSize = ((strlen(iCompressedBase85) + 4) / 5) * 4;
  std::vector<unsigned char> compressedData(static_cast<size_t>(compressedSize));
  Decode85((const unsigned char*) iCompressedBase85, compressedData.data());

  // compressed binary => binary
  const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*) compressedData.data());
  std::vector<unsigned char> decompressedData(static_cast<size_t>(buf_decompressed_size));
  stb_decompress(decompressedData.data(), compressedData.data(), static_cast<unsigned int>(compressedData.size()));

  return decompressedData;
}

}