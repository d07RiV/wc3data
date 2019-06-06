#include "detect.h"

static inline bool valid_cont(uint8 c) {
  return c >= 0x80 && c <= 0xBF;
}

bool is_text(uint8 const* ptr, size_t size, double tolerance) {
  size_t limit = size_t(double(size) * tolerance);

  size_t print = 0, total = 0;
  for (size_t i = 0; i < size && total - print < limit; ++i) {
    ++total;
    uint8 head = ptr[i];
    if (head <= 0x7F) {
      print += ((head >= 0x20 && head <= 0x7E) || head == 0x09 || head == 0x0A || head == 0x0D);
    } else if (head >= 0xC0 && head <= 0xDF) {
      if (i + 1 < size) {
        uint8 next = ptr[i + 1];
        if (valid_cont(next)) {
          uint32 cp = (uint32(head & 0x1F) << 6) | uint32(next & 0x3F);
          print += (cp >= 0xA0);
          i += 1;
        }
      }
    } else if (head >= 0xE0 && head <= 0xEF) {
      if (i + 2 < size && valid_cont(ptr[i + 1]) && valid_cont(ptr[i + 2])) {
        print += 1;
        i += 2;
      }
    } else if (head >= 0xF0 && head <= 0xF7) {
      if (i + 3 < size && valid_cont(ptr[i + 1]) && valid_cont(ptr[i + 2]) && valid_cont(ptr[i + 3])) {
        print += 1;
        i += 3;
      }
    }
  }

  return (total - print) < size_t(double(total) * tolerance);
}

bool is_wave(uint8 const* ptr, size_t size) {
  if (size < 16) {
    return false;
  }
  uint32 const* u32 = (uint32*)ptr;
  if (u32[0] != 0x46464952) { // 'RIFF'
    return false;
  }
  if (size_t(u32[1] + 8) != size) {
    return false;
  }
  if (u32[2] != 0x45564157) { // 'WAVE'
    return false;
  }
  if (u32[3] != 0x20746D66) { // 'fmt '
    return false;
  }
  return true;
}

namespace {

const size_t mpeg_sampling_rate[4][3] =
{
  {11025, 12000, 8000,  },	// MPEG 2.5
  {0,     0,     0,     },	// reserved
  {22050, 24000, 16000, },	// MPEG 2
  {44100, 48000, 32000  }		// MPEG 1
};
const size_t mpeg_bitrate[2][3][15] =
{
  {	// MPEG 1
    {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},	// Layer1
    {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},	// Layer2
    {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,}	// Layer3
  },
  {	// MPEG 2, 2.5		
    {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},		// Layer1
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},			// Layer2
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,}			// Layer3
  }
};
const size_t mpeg_samples_per_frame[2][3] =
{
  {	// MPEG 1
    384,	// Layer1
    1152,	// Layer2	
    1152	// Layer3
  },
  {	// MPEG 2, 2.5
    384,	// Layer1
    1152,	// Layer2
    576		// Layer3
  }
};
const size_t mpeg_coefficient[2][3] =
{
  {	// MPEG 1
    12,		// Layer1	(must be multiplied with 4, because of slot size)
    144,	// Layer2
    144		// Layer3
  },
  {	// MPEG 2, 2.5
    12,		// Layer1	(must be multiplied with 4, because of slot size)
    144,	// Layer2
    72		// Layer3
  }
};
const size_t mpeg_slot_size[3] =
{
  4,			// Layer1
  1,			// Layer2
  1			// Layer3
};
const size_t mpeg_side_info_size[2][2] =
{
  // MPEG 1
  {32,17},
  // MPEG 2/2.5
  {17,9}
};

const bool mpeg_allowed_mode[15][2] =
{
  // {stereo, intensity stereo, dual channel allowed,single channel allowed}
  {true,true},		// free mode
  {false,true},		// 32
  {false,true},		// 48
  {false,true},		// 56
  {true,true},		// 64
  {false,true},		// 80
  {true,true},		// 96
  {true,true},		// 112
  {true,true},		// 128
  {true,true},		// 160
  {true,true},		// 192
  {true,false},		// 224
  {true,false},		// 256
  {true,false},		// 320
  {true,false}		// 384
};

bool is_mp3_header(uint8 const* ptr) {
  if (ptr[0] != 0xFF || (ptr[1] & 0xE0) != 0xE0) {
    return false;
  }
  return true;
}

bool mp3_header(uint8 const* ptr, size_t& next) {
  if (!is_mp3_header(ptr)) {
    return false;
  }

  uint8 version = ((ptr[1] >> 3) & 0x03);
  if (version == 1) {
    return false;
  }
  if (version != 2 && version != 3) {
    // only MPEG1 / MPEG2
    return false;
  }
  uint8 is_lsf = (version != 3);

  uint8 layer_id = ((ptr[1] >> 1) & 0x03);
  if (layer_id == 0) {
    return false;
  }
  uint8 layer = 3 - layer_id;
  if (layer != 2) {
    // only MP3
    return false;
  }

  uint8 bitrate_index = ((ptr[2] >> 4) & 0x0F);
  if (bitrate_index == 0x0F) {
    return false;
  }
  size_t bitrate = mpeg_bitrate[is_lsf][layer][bitrate_index] * 1000;
  if (bitrate == 0) {
    return false;
  }

  uint8 samples_index = ((ptr[2] >> 2) & 0x03);
  if (samples_index == 0x03) {
    return false;
  }
  size_t sampling_rate = mpeg_sampling_rate[version][samples_index];

  size_t padding_size = 1 * ((ptr[2] >> 1) & 0x01);

  if ((ptr[3] & 0x03) == 2) {
    return false;
  }

  next = ((mpeg_coefficient[is_lsf][layer] * bitrate / sampling_rate) + padding_size) * mpeg_slot_size[layer];

  return true;
}

}

bool is_mp3(uint8 const* ptr, size_t size, int check) {
  size_t offs = 0;
  while (offs + 4 <= size && offs < 2048) {
    if (is_mp3_header(ptr + offs)) {
      break;
    }
    ++offs;
  }
  size_t next;
  for (int count = 0; count < check; ++count) {
    if (offs + 4 > size || !mp3_header(ptr + offs, next)) {
      return false;
    }
    offs += next;
  }
  return true;
}

bool is_mdx(uint8 const* ptr, size_t size) {
  if (size < 4) {
    return false;
  }
  if (ptr[0] != 0x4D || ptr[1] != 0x44 || ptr[2] != 0x4C || ptr[3] != 0x58) { // MDLX
    return false;
  }
  return true;
}
