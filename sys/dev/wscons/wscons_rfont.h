#include "opt_wsfont_choice.h"

#define DONT_HAVE_FONT_YET

#if defined(WSFONT_FIXED6x13) && defined(DONT_HAVE_FONT_YET)
#undef DONT_HAVE_FONT_YET
#define X(n) 0x##n##000000
static u_int32_t dwsfont_0x00_pixels[]
 = { X(00), X(f0), X(f0), X(f0), X(f0), X(f0), X(f0), X(f0), X(f0), X(f0), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x01_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(20), X(70), X(f8), X(70), X(20), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x02_pixels[]
 = { X(00), X(00), X(00), X(00), X(a8), X(50), X(a8), X(50), X(a8), X(50), X(a8), X(00), X(00) };
static u_int32_t dwsfont_0x03_pixels[]
 = { X(00), X(00), X(00), X(00), X(a0), X(a0), X(e0), X(a0), X(a0), X(70), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x04_pixels[]
 = { X(00), X(00), X(00), X(00), X(e0), X(80), X(c0), X(80), X(f0), X(40), X(60), X(40), X(40) };
static u_int32_t dwsfont_0x05_pixels[]
 = { X(00), X(00), X(00), X(00), X(70), X(80), X(80), X(70), X(70), X(48), X(70), X(50), X(48) };
static u_int32_t dwsfont_0x06_pixels[]
 = { X(00), X(00), X(00), X(00), X(80), X(80), X(80), X(e0), X(70), X(40), X(60), X(40), X(40) };
static u_int32_t dwsfont_0x07_pixels[]
 = { X(00), X(00), X(00), X(00), X(60), X(90), X(90), X(60), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x08_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(20), X(f8), X(20), X(20), X(00), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x09_pixels[]
 = { X(00), X(00), X(00), X(00), X(88), X(c8), X(a8), X(98), X(88), X(40), X(40), X(40), X(78) };
static u_int32_t dwsfont_0x0a_pixels[]
 = { X(00), X(00), X(00), X(00), X(88), X(88), X(50), X(20), X(00), X(f8), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x0b_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(e0), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x0c_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(e0), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x0d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(3c), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x0e_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(3c), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x0f_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(fc), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x10_pixels[]
 = { X(00), X(00), X(00), X(fc), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x11_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(fc), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x12_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(fc), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x13_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(fc), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x14_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(fc), X(00) };
static u_int32_t dwsfont_0x15_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(3c), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x16_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(e0), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x17_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(fc), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x18_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(fc), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x19_pixels[]
 = { X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0x1a_pixels[]
 = { X(00), X(00), X(00), X(08), X(10), X(20), X(40), X(20), X(10), X(08), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x1b_pixels[]
 = { X(00), X(00), X(00), X(80), X(40), X(20), X(10), X(20), X(40), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x1c_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(f8), X(50), X(50), X(50), X(90), X(00), X(00) };
static u_int32_t dwsfont_0x1d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(08), X(f8), X(20), X(f8), X(80), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x1e_pixels[]
 = { X(00), X(00), X(00), X(00), X(30), X(48), X(40), X(f0), X(20), X(f0), X(a8), X(e0), X(00) };
static u_int32_t dwsfont_0x1f_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(20), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x20_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x21_pixels[]
 = { X(00), X(00), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(00), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x22_pixels[]
 = { X(00), X(00), X(50), X(50), X(50), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x23_pixels[]
 = { X(00), X(00), X(00), X(50), X(50), X(f8), X(50), X(f8), X(50), X(50), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x24_pixels[]
 = { X(00), X(20), X(70), X(a0), X(a0), X(70), X(28), X(28), X(70), X(20), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x25_pixels[]
 = { X(00), X(00), X(48), X(a8), X(50), X(10), X(20), X(40), X(50), X(a8), X(90), X(00), X(00) };
static u_int32_t dwsfont_0x26_pixels[]
 = { X(00), X(00), X(40), X(a0), X(a0), X(40), X(a0), X(98), X(90), X(68), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x27_pixels[]
 = { X(00), X(00), X(30), X(20), X(40), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x28_pixels[]
 = { X(00), X(00), X(10), X(20), X(20), X(40), X(40), X(40), X(20), X(20), X(10), X(00), X(00) };
static u_int32_t dwsfont_0x29_pixels[]
 = { X(00), X(00), X(40), X(20), X(20), X(10), X(10), X(10), X(20), X(20), X(40), X(00), X(00) };
static u_int32_t dwsfont_0x2a_pixels[]
 = { X(00), X(00), X(00), X(20), X(a8), X(f8), X(70), X(f8), X(a8), X(20), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x2b_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(20), X(f8), X(20), X(20), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x2c_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(30), X(20), X(40), X(00) };
static u_int32_t dwsfont_0x2d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(f8), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x2e_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(20), X(70), X(20), X(00) };
static u_int32_t dwsfont_0x2f_pixels[]
 = { X(00), X(00), X(08), X(08), X(10), X(10), X(20), X(40), X(40), X(80), X(80), X(00), X(00) };
static u_int32_t dwsfont_0x30_pixels[]
 = { X(00), X(00), X(20), X(50), X(88), X(88), X(88), X(88), X(88), X(50), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x31_pixels[]
 = { X(00), X(00), X(20), X(60), X(a0), X(20), X(20), X(20), X(20), X(20), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x32_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(08), X(10), X(20), X(40), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x33_pixels[]
 = { X(00), X(00), X(f8), X(08), X(10), X(20), X(70), X(08), X(08), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x34_pixels[]
 = { X(00), X(00), X(10), X(10), X(30), X(50), X(50), X(90), X(f8), X(10), X(10), X(00), X(00) };
static u_int32_t dwsfont_0x35_pixels[]
 = { X(00), X(00), X(f8), X(80), X(80), X(b0), X(c8), X(08), X(08), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x36_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(80), X(f0), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x37_pixels[]
 = { X(00), X(00), X(f8), X(08), X(10), X(10), X(20), X(20), X(40), X(40), X(40), X(00), X(00) };
static u_int32_t dwsfont_0x38_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(88), X(70), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x39_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(88), X(78), X(08), X(08), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x3a_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(70), X(20), X(00), X(00), X(20), X(70), X(20), X(00) };
static u_int32_t dwsfont_0x3b_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(70), X(20), X(00), X(00), X(30), X(20), X(40), X(00) };
static u_int32_t dwsfont_0x3c_pixels[]
 = { X(00), X(00), X(08), X(10), X(20), X(40), X(80), X(40), X(20), X(10), X(08), X(00), X(00) };
static u_int32_t dwsfont_0x3d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(f8), X(00), X(00), X(f8), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x3e_pixels[]
 = { X(00), X(00), X(80), X(40), X(20), X(10), X(08), X(10), X(20), X(40), X(80), X(00), X(00) };
static u_int32_t dwsfont_0x3f_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(08), X(10), X(20), X(20), X(00), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x40_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(98), X(a8), X(a8), X(b0), X(80), X(78), X(00), X(00) };
static u_int32_t dwsfont_0x41_pixels[]
 = { X(00), X(00), X(20), X(50), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x42_pixels[]
 = { X(00), X(00), X(f0), X(48), X(48), X(48), X(70), X(48), X(48), X(48), X(f0), X(00), X(00) };
static u_int32_t dwsfont_0x43_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(80), X(80), X(80), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x44_pixels[]
 = { X(00), X(00), X(f0), X(48), X(48), X(48), X(48), X(48), X(48), X(48), X(f0), X(00), X(00) };
static u_int32_t dwsfont_0x45_pixels[]
 = { X(00), X(00), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x46_pixels[]
 = { X(00), X(00), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(80), X(00), X(00) };
static u_int32_t dwsfont_0x47_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(80), X(80), X(98), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x48_pixels[]
 = { X(00), X(00), X(88), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x49_pixels[]
 = { X(00), X(00), X(70), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x4a_pixels[]
 = { X(00), X(00), X(38), X(10), X(10), X(10), X(10), X(10), X(10), X(90), X(60), X(00), X(00) };
static u_int32_t dwsfont_0x4b_pixels[]
 = { X(00), X(00), X(88), X(88), X(90), X(a0), X(c0), X(a0), X(90), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x4c_pixels[]
 = { X(00), X(00), X(80), X(80), X(80), X(80), X(80), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x4d_pixels[]
 = { X(00), X(00), X(88), X(88), X(d8), X(a8), X(a8), X(88), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x4e_pixels[]
 = { X(00), X(00), X(88), X(c8), X(c8), X(a8), X(a8), X(98), X(98), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x4f_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x50_pixels[]
 = { X(00), X(00), X(f0), X(88), X(88), X(88), X(f0), X(80), X(80), X(80), X(80), X(00), X(00) };
static u_int32_t dwsfont_0x51_pixels[]
 = { X(00), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(a8), X(70), X(08), X(00) };
static u_int32_t dwsfont_0x52_pixels[]
 = { X(00), X(00), X(f0), X(88), X(88), X(88), X(f0), X(a0), X(90), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x53_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(80), X(70), X(08), X(08), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x54_pixels[]
 = { X(00), X(00), X(f8), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x55_pixels[]
 = { X(00), X(00), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x56_pixels[]
 = { X(00), X(00), X(88), X(88), X(88), X(88), X(50), X(50), X(50), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x57_pixels[]
 = { X(00), X(00), X(88), X(88), X(88), X(88), X(a8), X(a8), X(a8), X(d8), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x58_pixels[]
 = { X(00), X(00), X(88), X(88), X(50), X(50), X(20), X(50), X(50), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x59_pixels[]
 = { X(00), X(00), X(88), X(88), X(50), X(50), X(20), X(20), X(20), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x5a_pixels[]
 = { X(00), X(00), X(f8), X(08), X(10), X(10), X(20), X(40), X(40), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x5b_pixels[]
 = { X(00), X(00), X(70), X(40), X(40), X(40), X(40), X(40), X(40), X(40), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x5c_pixels[]
 = { X(00), X(00), X(80), X(80), X(40), X(40), X(20), X(10), X(10), X(08), X(08), X(00), X(00) };
static u_int32_t dwsfont_0x5d_pixels[]
 = { X(00), X(00), X(70), X(10), X(10), X(10), X(10), X(10), X(10), X(10), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x5e_pixels[]
 = { X(00), X(00), X(20), X(50), X(88), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x5f_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(f8), X(00) };
static u_int32_t dwsfont_0x60_pixels[]
 = { X(00), X(00), X(30), X(10), X(08), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x61_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0x62_pixels[]
 = { X(00), X(00), X(80), X(80), X(80), X(f0), X(88), X(88), X(88), X(88), X(f0), X(00), X(00) };
static u_int32_t dwsfont_0x63_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(80), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x64_pixels[]
 = { X(00), X(00), X(08), X(08), X(08), X(78), X(88), X(88), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0x65_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(f8), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x66_pixels[]
 = { X(00), X(00), X(30), X(48), X(40), X(40), X(f0), X(40), X(40), X(40), X(40), X(00), X(00) };
static u_int32_t dwsfont_0x67_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(88), X(88), X(78), X(08), X(88), X(70) };
static u_int32_t dwsfont_0x68_pixels[]
 = { X(00), X(00), X(80), X(80), X(80), X(b0), X(c8), X(88), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x69_pixels[]
 = { X(00), X(00), X(00), X(20), X(00), X(60), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x6a_pixels[]
 = { X(00), X(00), X(00), X(10), X(00), X(30), X(10), X(10), X(10), X(10), X(90), X(90), X(60) };
static u_int32_t dwsfont_0x6b_pixels[]
 = { X(00), X(00), X(80), X(80), X(80), X(90), X(a0), X(c0), X(a0), X(90), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x6c_pixels[]
 = { X(00), X(00), X(60), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x6d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(d0), X(a8), X(a8), X(a8), X(a8), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x6e_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(b0), X(c8), X(88), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x6f_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x70_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(f0), X(88), X(88), X(88), X(f0), X(80), X(80), X(80) };
static u_int32_t dwsfont_0x71_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(78), X(88), X(88), X(88), X(78), X(08), X(08), X(08) };
static u_int32_t dwsfont_0x72_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(b0), X(c8), X(80), X(80), X(80), X(80), X(00), X(00) };
static u_int32_t dwsfont_0x73_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(60), X(10), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0x74_pixels[]
 = { X(00), X(00), X(00), X(40), X(40), X(f0), X(40), X(40), X(40), X(48), X(30), X(00), X(00) };
static u_int32_t dwsfont_0x75_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(88), X(88), X(88), X(98), X(68), X(00), X(00) };
static u_int32_t dwsfont_0x76_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(88), X(88), X(50), X(50), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x77_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(88), X(a8), X(a8), X(a8), X(50), X(00), X(00) };
static u_int32_t dwsfont_0x78_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(50), X(20), X(20), X(50), X(88), X(00), X(00) };
static u_int32_t dwsfont_0x79_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(88), X(88), X(98), X(68), X(08), X(88), X(70) };
static u_int32_t dwsfont_0x7a_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(f8), X(10), X(20), X(40), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0x7b_pixels[]
 = { X(00), X(00), X(18), X(20), X(20), X(20), X(c0), X(20), X(20), X(20), X(18), X(00), X(00) };
static u_int32_t dwsfont_0x7c_pixels[]
 = { X(00), X(00), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0x7d_pixels[]
 = { X(00), X(00), X(c0), X(20), X(20), X(20), X(18), X(20), X(20), X(20), X(c0), X(00), X(00) };
static u_int32_t dwsfont_0x7e_pixels[]
 = { X(00), X(00), X(48), X(a8), X(90), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x7f_pixels[]
 = { X(a8), X(50), X(a8), X(50), X(a8), X(50), X(a8), X(50), X(a8), X(50), X(a8), X(50), X(a8) };
static u_int32_t dwsfont_0x80_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x81_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x82_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x83_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x84_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x85_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x86_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x87_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x88_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x89_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8a_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8b_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8c_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8e_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x8f_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x90_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x91_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x92_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x93_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x94_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x95_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x96_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x97_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x98_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x99_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9a_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9b_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9c_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9d_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9e_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0x9f_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xa0_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xa1_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(00), X(20), X(20), X(20), X(20), X(20), X(20), X(20) };
static u_int32_t dwsfont_0xa2_pixels[]
 = { X(00), X(00), X(20), X(20), X(70), X(88), X(80), X(88), X(70), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0xa3_pixels[]
 = { X(00), X(00), X(30), X(48), X(40), X(40), X(f0), X(40), X(40), X(88), X(f0), X(00), X(00) };
static u_int32_t dwsfont_0xa4_pixels[]
 = { X(00), X(00), X(00), X(a8), X(50), X(88), X(88), X(50), X(a8), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xa5_pixels[]
 = { X(00), X(00), X(88), X(88), X(50), X(50), X(20), X(f8), X(20), X(f8), X(20), X(00), X(00) };
static u_int32_t dwsfont_0xa6_pixels[]
 = { X(00), X(00), X(20), X(20), X(20), X(20), X(00), X(20), X(20), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0xa7_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(70), X(88), X(88), X(70), X(08), X(88), X(70), X(00) };
static u_int32_t dwsfont_0xa8_pixels[]
 = { X(00), X(50), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xa9_pixels[]
 = { X(00), X(00), X(70), X(88), X(a8), X(d8), X(c8), X(d8), X(a8), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xaa_pixels[]
 = { X(00), X(00), X(00), X(60), X(90), X(70), X(90), X(e8), X(00), X(f8), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xab_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(28), X(50), X(a0), X(50), X(28), X(00), X(00) };
static u_int32_t dwsfont_0xac_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(f8), X(08), X(08), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xad_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(78), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xae_pixels[]
 = { X(00), X(00), X(70), X(88), X(e8), X(d8), X(e8), X(d8), X(d8), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xaf_pixels[]
 = { X(00), X(00), X(70), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb0_pixels[]
 = { X(00), X(00), X(60), X(90), X(90), X(60), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb1_pixels[]
 = { X(00), X(00), X(00), X(00), X(20), X(20), X(f8), X(20), X(20), X(00), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0xb2_pixels[]
 = { X(00), X(60), X(90), X(90), X(20), X(40), X(f0), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb3_pixels[]
 = { X(00), X(60), X(90), X(20), X(10), X(90), X(60), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb4_pixels[]
 = { X(00), X(18), X(60), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb5_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(88), X(88), X(88), X(88), X(98), X(e8), X(80), X(80) };
static u_int32_t dwsfont_0xb6_pixels[]
 = { X(00), X(78), X(a8), X(a8), X(a8), X(68), X(28), X(28), X(28), X(28), X(28), X(00), X(00) };
static u_int32_t dwsfont_0xb7_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(20), X(70), X(20), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xb8_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(00), X(10), X(30) };
static u_int32_t dwsfont_0xb9_pixels[]
 = { X(00), X(20), X(60), X(20), X(20), X(20), X(70), X(00), X(00), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xba_pixels[]
 = { X(00), X(00), X(00), X(70), X(88), X(88), X(88), X(70), X(00), X(f8), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xbb_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(00), X(a0), X(50), X(28), X(50), X(a0), X(00), X(00) };
static u_int32_t dwsfont_0xbc_pixels[]
 = { X(00), X(40), X(c0), X(40), X(48), X(f0), X(38), X(38), X(68), X(bc), X(08), X(00), X(00) };
static u_int32_t dwsfont_0xbd_pixels[]
 = { X(00), X(40), X(c0), X(40), X(48), X(f0), X(38), X(24), X(48), X(90), X(1c), X(00), X(00) };
static u_int32_t dwsfont_0xbe_pixels[]
 = { X(00), X(60), X(90), X(20), X(98), X(70), X(38), X(38), X(68), X(bc), X(08), X(00), X(00) };
static u_int32_t dwsfont_0xbf_pixels[]
 = { X(00), X(00), X(00), X(20), X(00), X(20), X(20), X(40), X(80), X(88), X(88), X(70), X(00) };
static u_int32_t dwsfont_0xc0_pixels[]
 = { X(c0), X(30), X(00), X(70), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc1_pixels[]
 = { X(18), X(60), X(00), X(70), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc2_pixels[]
 = { X(20), X(50), X(00), X(70), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc3_pixels[]
 = { X(68), X(b0), X(20), X(50), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc4_pixels[]
 = { X(50), X(00), X(20), X(50), X(88), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc5_pixels[]
 = { X(20), X(50), X(20), X(00), X(70), X(88), X(88), X(f8), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xc6_pixels[]
 = { X(00), X(00), X(3c), X(50), X(90), X(90), X(9c), X(f0), X(90), X(90), X(9c), X(00), X(00) };
static u_int32_t dwsfont_0xc7_pixels[]
 = { X(00), X(00), X(70), X(88), X(80), X(80), X(80), X(80), X(80), X(88), X(70), X(10), X(30) };
static u_int32_t dwsfont_0xc8_pixels[]
 = { X(c0), X(30), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0xc9_pixels[]
 = { X(18), X(60), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0xca_pixels[]
 = { X(20), X(50), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0xcb_pixels[]
 = { X(50), X(00), X(f8), X(80), X(80), X(80), X(f0), X(80), X(80), X(80), X(f8), X(00), X(00) };
static u_int32_t dwsfont_0xcc_pixels[]
 = { X(c0), X(30), X(70), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xcd_pixels[]
 = { X(18), X(60), X(70), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xce_pixels[]
 = { X(20), X(50), X(70), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xcf_pixels[]
 = { X(50), X(00), X(70), X(20), X(20), X(20), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd0_pixels[]
 = { X(00), X(00), X(f0), X(48), X(48), X(48), X(e8), X(48), X(48), X(48), X(f0), X(00), X(00) };
static u_int32_t dwsfont_0xd1_pixels[]
 = { X(68), X(b0), X(88), X(c8), X(c8), X(a8), X(a8), X(98), X(98), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xd2_pixels[]
 = { X(c0), X(30), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd3_pixels[]
 = { X(18), X(60), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd4_pixels[]
 = { X(20), X(50), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd5_pixels[]
 = { X(68), X(b0), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd6_pixels[]
 = { X(50), X(00), X(70), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd7_pixels[]
 = { X(00), X(00), X(00), X(00), X(88), X(50), X(20), X(50), X(88), X(00), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xd8_pixels[]
 = { X(00), X(00), X(78), X(98), X(98), X(a8), X(a8), X(c8), X(c8), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xd9_pixels[]
 = { X(c0), X(30), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xda_pixels[]
 = { X(18), X(60), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xdb_pixels[]
 = { X(20), X(50), X(00), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xdc_pixels[]
 = { X(50), X(00), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xdd_pixels[]
 = { X(18), X(60), X(88), X(88), X(50), X(50), X(20), X(20), X(20), X(20), X(20), X(00), X(00) };
static u_int32_t dwsfont_0xde_pixels[]
 = { X(00), X(00), X(80), X(80), X(f0), X(88), X(88), X(88), X(f0), X(80), X(80), X(00), X(00) };
static u_int32_t dwsfont_0xdf_pixels[]
 = { X(00), X(00), X(60), X(90), X(90), X(a0), X(b0), X(88), X(88), X(88), X(b0), X(00), X(00) };
static u_int32_t dwsfont_0xe0_pixels[]
 = { X(00), X(00), X(c0), X(30), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe1_pixels[]
 = { X(00), X(00), X(18), X(60), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe2_pixels[]
 = { X(00), X(00), X(20), X(50), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe3_pixels[]
 = { X(00), X(00), X(68), X(b0), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe4_pixels[]
 = { X(00), X(00), X(00), X(50), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe5_pixels[]
 = { X(00), X(20), X(50), X(20), X(00), X(70), X(08), X(78), X(88), X(88), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe6_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(d0), X(28), X(78), X(a0), X(a0), X(78), X(00), X(00) };
static u_int32_t dwsfont_0xe7_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(70), X(88), X(80), X(80), X(88), X(70), X(10), X(30) };
static u_int32_t dwsfont_0xe8_pixels[]
 = { X(00), X(00), X(c0), X(30), X(00), X(70), X(88), X(f8), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xe9_pixels[]
 = { X(00), X(00), X(18), X(60), X(00), X(70), X(88), X(f8), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xea_pixels[]
 = { X(00), X(00), X(20), X(50), X(00), X(70), X(88), X(f8), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xeb_pixels[]
 = { X(00), X(00), X(00), X(50), X(00), X(70), X(88), X(f8), X(80), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xec_pixels[]
 = { X(00), X(00), X(c0), X(30), X(00), X(60), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xed_pixels[]
 = { X(00), X(00), X(30), X(c0), X(00), X(60), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xee_pixels[]
 = { X(00), X(00), X(20), X(50), X(00), X(60), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xef_pixels[]
 = { X(00), X(00), X(00), X(50), X(00), X(60), X(20), X(20), X(20), X(20), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf0_pixels[]
 = { X(00), X(00), X(d8), X(20), X(d0), X(08), X(78), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf1_pixels[]
 = { X(00), X(00), X(68), X(b0), X(00), X(b0), X(c8), X(88), X(88), X(88), X(88), X(00), X(00) };
static u_int32_t dwsfont_0xf2_pixels[]
 = { X(00), X(00), X(c0), X(30), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf3_pixels[]
 = { X(00), X(00), X(18), X(60), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf4_pixels[]
 = { X(00), X(00), X(20), X(50), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf5_pixels[]
 = { X(00), X(00), X(68), X(b0), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf6_pixels[]
 = { X(00), X(00), X(00), X(50), X(00), X(70), X(88), X(88), X(88), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf7_pixels[]
 = { X(00), X(00), X(00), X(20), X(20), X(00), X(f8), X(00), X(20), X(20), X(00), X(00), X(00) };
static u_int32_t dwsfont_0xf8_pixels[]
 = { X(00), X(00), X(00), X(00), X(00), X(78), X(98), X(a8), X(c8), X(88), X(70), X(00), X(00) };
static u_int32_t dwsfont_0xf9_pixels[]
 = { X(00), X(00), X(c0), X(30), X(00), X(88), X(88), X(88), X(88), X(98), X(68), X(00), X(00) };
static u_int32_t dwsfont_0xfa_pixels[]
 = { X(00), X(00), X(18), X(60), X(00), X(88), X(88), X(88), X(88), X(98), X(68), X(00), X(00) };
static u_int32_t dwsfont_0xfb_pixels[]
 = { X(00), X(00), X(20), X(50), X(00), X(88), X(88), X(88), X(88), X(98), X(68), X(00), X(00) };
static u_int32_t dwsfont_0xfc_pixels[]
 = { X(00), X(00), X(00), X(50), X(00), X(88), X(88), X(88), X(88), X(98), X(68), X(00), X(00) };
static u_int32_t dwsfont_0xfd_pixels[]
 = { X(00), X(00), X(18), X(60), X(00), X(88), X(88), X(88), X(98), X(68), X(08), X(88), X(70) };
static u_int32_t dwsfont_0xfe_pixels[]
 = { X(00), X(00), X(80), X(80), X(f0), X(88), X(88), X(88), X(88), X(f0), X(80), X(80), X(00) };
static u_int32_t dwsfont_0xff_pixels[]
 = { X(00), X(00), X(50), X(00), X(00), X(88), X(88), X(88), X(98), X(68), X(08), X(88), X(70) };
#undef X
#define DWSFONT_CWIDTH 6
#define DWSFONT_CHEIGHT 13
#define DWSFONT_ASCENT 10
#endif

#ifdef DONT_HAVE_FONT_YET
static u_int32_t dwsfont_0x00_pixels[]
 = { 0x00000000, 0x00000000, 0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000,
     0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000,
     0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x01_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x06000000, 0x0f000000, 0x1f800000, 0x3fc00000, 0x7fe00000,
     0x7fe00000, 0x3fc00000, 0x1f800000, 0x0f000000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x02_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x55400000, 0x2aa00000, 0x55400000, 0x2aa00000, 0x55400000, 0x2aa00000,
     0x55400000, 0x2aa00000, 0x55400000, 0x2aa00000, 0x55400000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x03_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x44000000,
     0x44000000, 0x44000000, 0x7c000000, 0x44000000, 0x44000000, 0x44000000,
     0x03e00000, 0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000,
     0x00800000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x04_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7c000000,
     0x40000000, 0x40000000, 0x78000000, 0x40000000, 0x40000000, 0x40000000,
     0x03e00000, 0x02000000, 0x02000000, 0x03c00000, 0x02000000, 0x02000000,
     0x02000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x05_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x38000000,
     0x44000000, 0x40000000, 0x40000000, 0x40000000, 0x44000000, 0x38000000,
     0x03c00000, 0x02200000, 0x02200000, 0x03c00000, 0x02800000, 0x02400000,
     0x02200000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x06_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000000,
     0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x7c000000,
     0x03e00000, 0x02000000, 0x02000000, 0x03c00000, 0x02000000, 0x02000000,
     0x02000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x07_pixels[]
 = { 0x00000000, 0x00000000, 0x0e000000, 0x17000000, 0x23800000, 0x61800000,
     0x61800000, 0x71000000, 0x3a000000, 0x1c000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x08_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x7fe00000, 0x7fe00000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x00000000, 0x7ff00000, 0x7ff00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x09_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x44000000,
     0x64000000, 0x64000000, 0x54000000, 0x4c000000, 0x4c000000, 0x44000000,
     0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000,
     0x03e00000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x0a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x44000000,
     0x44000000, 0x44000000, 0x28000000, 0x28000000, 0x10000000, 0x10000000,
     0x03e00000, 0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000,
     0x00800000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x0b_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0xfe000000, 0xfe000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x0c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xfe000000, 0xfe000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x0d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07f00000, 0x07f00000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x0e_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x07f00000, 0x07f00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x0f_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0xfff00000, 0xfff00000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x10_pixels[]
 = { 0x00000000, 0xfff00000, 0xfff00000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x11_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0xfff00000, 0xfff00000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x12_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xfff00000, 0xfff00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x13_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0xfff00000, 0xfff00000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x14_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xfff00000, 0xfff00000, 0x00000000 };
static u_int32_t dwsfont_0x15_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x07f00000, 0x07f00000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x16_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0xfe000000, 0xfe000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x17_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0xfff00000, 0xfff00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x18_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xfff00000, 0xfff00000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x19_pixels[]
 = { 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x1a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00e00000, 0x07800000, 0x1e000000, 0x78000000, 0x78000000, 0x1e000000,
     0x07800000, 0x00e00000, 0x00000000, 0x7fe00000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x1b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x70000000, 0x1e000000, 0x07800000, 0x01e00000, 0x01e00000, 0x07800000,
     0x1e000000, 0x70000000, 0x00000000, 0x7fe00000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x1c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x3fe00000, 0x7fc00000, 0x19800000, 0x19800000, 0x19800000,
     0x19800000, 0x31800000, 0x31800000, 0x31c00000, 0x60c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x1d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00400000, 0x00c00000, 0x01800000, 0x7fc00000, 0x7fc00000, 0x06000000,
     0x0c000000, 0x7fc00000, 0x7fc00000, 0x30000000, 0x60000000, 0x40000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x1e_pixels[]
 = { 0x00000000, 0x06000000, 0x0c000000, 0x10000000, 0x10000000, 0x30000000,
     0x30000000, 0x30000000, 0x3e000000, 0x7c000000, 0x18000000, 0x18000000,
     0x18000000, 0x18000000, 0x3f200000, 0x3fe00000, 0x31c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x1f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06000000,
     0x06000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x20_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x21_pixels[]
 = { 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x22_pixels[]
 = { 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x19800000, 0x19800000,
     0x19800000, 0x19800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x23_pixels[]
 = { 0x00000000, 0x00000000, 0x03300000, 0x03300000, 0x03300000, 0x06600000,
     0x1ff00000, 0x1ff00000, 0x0cc00000, 0x0cc00000, 0x19800000, 0x19800000,
     0x7fc00000, 0x7fc00000, 0x33000000, 0x66000000, 0x66000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x24_pixels[]
 = { 0x00000000, 0x00000000, 0x06000000, 0x1f800000, 0x3fc00000, 0x66e00000,
     0x66600000, 0x66000000, 0x3e000000, 0x1f800000, 0x07c00000, 0x06600000,
     0x06600000, 0x66600000, 0x7fc00000, 0x3f800000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x25_pixels[]
 = { 0x00000000, 0x00000000, 0x38600000, 0x44c00000, 0x44c00000, 0x45800000,
     0x39800000, 0x03000000, 0x03000000, 0x06000000, 0x0c000000, 0x0c000000,
     0x19c00000, 0x1a200000, 0x32200000, 0x32200000, 0x61c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x26_pixels[]
 = { 0x00000000, 0x00000000, 0x07000000, 0x0f800000, 0x18c00000, 0x18c00000,
     0x18c00000, 0x0f800000, 0x1e000000, 0x3e000000, 0x77000000, 0x63600000,
     0x61e00000, 0x61c00000, 0x61800000, 0x3fe00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x27_pixels[]
 = { 0x00000000, 0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x06000000,
     0x06000000, 0x0c000000, 0x18000000, 0x10000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x28_pixels[]
 = { 0x00000000, 0x00000000, 0x00c00000, 0x01800000, 0x03800000, 0x03000000,
     0x07000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x07000000, 0x03000000, 0x03800000, 0x01800000, 0x00c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x29_pixels[]
 = { 0x00000000, 0x00000000, 0x30000000, 0x18000000, 0x1c000000, 0x0c000000,
     0x0e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x0e000000, 0x0c000000, 0x1c000000, 0x18000000, 0x30000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x2a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x0f000000, 0x06000000, 0x66600000, 0x76e00000, 0x19800000, 0x00000000,
     0x19800000, 0x76e00000, 0x66600000, 0x06000000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x2b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x7fe00000,
     0x7fe00000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x2c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x06000000,
     0x06000000, 0x0c000000, 0x18000000, 0x10000000 };
static u_int32_t dwsfont_0x2d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7fe00000,
     0x7fe00000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x2e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x0c000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x2f_pixels[]
 = { 0x00000000, 0x00000000, 0x00600000, 0x00c00000, 0x00c00000, 0x01800000,
     0x01800000, 0x03000000, 0x03000000, 0x06000000, 0x0c000000, 0x0c000000,
     0x18000000, 0x18000000, 0x30000000, 0x30000000, 0x60000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x30_pixels[]
 = { 0x00000000, 0x00000000, 0x07000000, 0x0f800000, 0x11800000, 0x10c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30800000, 0x18800000, 0x1f000000, 0x0e000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x31_pixels[]
 = { 0x00000000, 0x00000000, 0x02000000, 0x06000000, 0x0e000000, 0x1e000000,
     0x36000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x3fc00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x32_pixels[]
 = { 0x00000000, 0x00000000, 0x1f000000, 0x3f800000, 0x61c00000, 0x40c00000,
     0x00c00000, 0x00c00000, 0x00c00000, 0x01800000, 0x03000000, 0x06000000,
     0x0c000000, 0x18000000, 0x30200000, 0x7fe00000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x33_pixels[]
 = { 0x00000000, 0x00000000, 0x0f800000, 0x1fc00000, 0x20e00000, 0x40600000,
     0x00600000, 0x00e00000, 0x07c00000, 0x0fc00000, 0x00e00000, 0x00600000,
     0x00600000, 0x40600000, 0x60400000, 0x3f800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x34_pixels[]
 = { 0x00000000, 0x00000000, 0x01800000, 0x03800000, 0x03800000, 0x05800000,
     0x05800000, 0x09800000, 0x09800000, 0x11800000, 0x11800000, 0x21800000,
     0x3fe00000, 0x7fe00000, 0x01800000, 0x01800000, 0x01800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x35_pixels[]
 = { 0x00000000, 0x00000000, 0x0fc00000, 0x0fc00000, 0x10000000, 0x10000000,
     0x20000000, 0x3f800000, 0x31c00000, 0x00e00000, 0x00600000, 0x00600000,
     0x00600000, 0x40600000, 0x60600000, 0x30c00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x36_pixels[]
 = { 0x00000000, 0x00000000, 0x07000000, 0x0c000000, 0x18000000, 0x30000000,
     0x30000000, 0x60000000, 0x67800000, 0x6fc00000, 0x70e00000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x3f800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x37_pixels[]
 = { 0x00000000, 0x00000000, 0x1fe00000, 0x3fe00000, 0x60400000, 0x00400000,
     0x00c00000, 0x00800000, 0x00800000, 0x01800000, 0x01000000, 0x01000000,
     0x03000000, 0x02000000, 0x02000000, 0x06000000, 0x04000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x38_pixels[]
 = { 0x00000000, 0x00000000, 0x0f000000, 0x11800000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x18800000, 0x0d000000, 0x06000000, 0x0b000000, 0x11800000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x39_pixels[]
 = { 0x00000000, 0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000,
     0x60600000, 0x60600000, 0x70e00000, 0x3f600000, 0x1e600000, 0x00600000,
     0x00c00000, 0x00c00000, 0x01800000, 0x07000000, 0x3c000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x3a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x0c000000, 0x00000000,
     0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x0c000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x3b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x0c000000,
     0x00000000, 0x00000000, 0x0c000000, 0x1e000000, 0x1e000000, 0x06000000,
     0x06000000, 0x0c000000, 0x18000000, 0x10000000 };
static u_int32_t dwsfont_0x3c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00600000, 0x01c00000, 0x07000000, 0x1e000000, 0x78000000,
     0x78000000, 0x1e000000, 0x07000000, 0x01c00000, 0x00600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x3d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x7fc00000, 0x7fc00000, 0x00000000,
     0x00000000, 0x7fc00000, 0x7fc00000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x3e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x60000000, 0x38000000, 0x1e000000, 0x07800000, 0x01e00000,
     0x01e00000, 0x07800000, 0x1e000000, 0x38000000, 0x60000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x3f_pixels[]
 = { 0x00000000, 0x00000000, 0x0f000000, 0x1f800000, 0x39c00000, 0x20c00000,
     0x00c00000, 0x00c00000, 0x01800000, 0x03000000, 0x06000000, 0x0c000000,
     0x0c000000, 0x00000000, 0x00000000, 0x0c000000, 0x0c000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x40_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0f800000,
     0x3fc00000, 0x30600000, 0x60600000, 0x67200000, 0x6fa00000, 0x6ca00000,
     0x6ca00000, 0x67e00000, 0x60000000, 0x30000000, 0x3fe00000, 0x0fe00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x41_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x42_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xff000000, 0x60800000, 0x60c00000,
     0x60c00000, 0x60c00000, 0x61800000, 0x7f800000, 0x60c00000, 0x60600000,
     0x60600000, 0x60600000, 0x60600000, 0x60c00000, 0xff800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x43_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0fc00000, 0x10600000, 0x20200000,
     0x20000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
     0x60000000, 0x20000000, 0x30200000, 0x18400000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x44_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xff000000, 0x61c00000, 0x60c00000,
     0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x60400000, 0x61800000, 0xfe000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x45_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x46_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30000000, 0x30000000, 0x78000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x47_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0fc00000, 0x10600000, 0x20200000,
     0x20000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x61f00000,
     0x60600000, 0x20600000, 0x30600000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x48_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0f00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x60600000, 0x7fe00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x60600000, 0x60600000, 0xf0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x49_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x4a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x04000000, 0x38000000, 0x30000000 };
static u_int32_t dwsfont_0x4b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0e00000, 0x61800000, 0x63000000,
     0x66000000, 0x6c000000, 0x78000000, 0x78000000, 0x7c000000, 0x6e000000,
     0x67000000, 0x63800000, 0x61c00000, 0x60e00000, 0xf0700000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x4c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x78000000, 0x30000000, 0x30000000,
     0x30000000, 0x30000000, 0x30000000, 0x30000000, 0x30000000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x4d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xe0700000, 0x60e00000, 0x70e00000,
     0x70e00000, 0x70e00000, 0x59600000, 0x59600000, 0x59600000, 0x4d600000,
     0x4e600000, 0x4e600000, 0x44600000, 0x44600000, 0xe4f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x4e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xc0700000, 0x60200000, 0x70200000,
     0x78200000, 0x58200000, 0x4c200000, 0x46200000, 0x47200000, 0x43200000,
     0x41a00000, 0x40e00000, 0x40e00000, 0x40600000, 0xe0300000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x4f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x50_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x7f800000, 0x30c00000, 0x30600000,
     0x30600000, 0x30600000, 0x30c00000, 0x37800000, 0x30000000, 0x30000000,
     0x30000000, 0x30000000, 0x30000000, 0x30000000, 0x78000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x51_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x30400000, 0x38400000, 0x1f800000, 0x0e000000, 0x1f000000,
     0x23900000, 0x01e00000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x52_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xff000000, 0x61800000, 0x60c00000,
     0x60c00000, 0x60c00000, 0x60800000, 0x7f000000, 0x7c000000, 0x6e000000,
     0x67000000, 0x63800000, 0x61c00000, 0x60e00000, 0xf0700000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x53_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1fe00000, 0x30600000, 0x60200000,
     0x60200000, 0x70000000, 0x3c000000, 0x1e000000, 0x07800000, 0x01c00000,
     0x00e00000, 0x40600000, 0x40600000, 0x60c00000, 0x7f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x54_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x7fe00000, 0x46200000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x55_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0700000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x70400000, 0x3fc00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x56_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xe0e00000, 0x60400000, 0x30800000,
     0x30800000, 0x30800000, 0x19000000, 0x19000000, 0x19000000, 0x0c000000,
     0x0e000000, 0x0e000000, 0x04000000, 0x04000000, 0x04000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x57_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xfef00000, 0x66200000, 0x66200000,
     0x66200000, 0x76200000, 0x77400000, 0x33400000, 0x37400000, 0x3bc00000,
     0x3b800000, 0x19800000, 0x19800000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x58_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0700000, 0x60200000, 0x30400000,
     0x38800000, 0x18800000, 0x0d000000, 0x06000000, 0x06000000, 0x0b000000,
     0x11800000, 0x11c00000, 0x20c00000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x59_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0700000, 0x60200000, 0x30400000,
     0x18800000, 0x18800000, 0x0d000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x3fe00000, 0x20c00000, 0x00c00000,
     0x01800000, 0x01800000, 0x03000000, 0x03000000, 0x06000000, 0x06000000,
     0x0c000000, 0x0c000000, 0x18000000, 0x18200000, 0x3fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5b_pixels[]
 = { 0x00000000, 0x00000000, 0x07c00000, 0x07c00000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x07c00000, 0x07c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5c_pixels[]
 = { 0x00000000, 0x00000000, 0x60000000, 0x60000000, 0x30000000, 0x30000000,
     0x18000000, 0x18000000, 0x0c000000, 0x0c000000, 0x06000000, 0x03000000,
     0x03000000, 0x01800000, 0x01800000, 0x00c00000, 0x00c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5d_pixels[]
 = { 0x00000000, 0x00000000, 0x7c000000, 0x7c000000, 0x0c000000, 0x0c000000,
     0x0c000000, 0x0c000000, 0x0c000000, 0x0c000000, 0x0c000000, 0x0c000000,
     0x0c000000, 0x0c000000, 0x0c000000, 0x7c000000, 0x7c000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5e_pixels[]
 = { 0x00000000, 0x00000000, 0x04000000, 0x0e000000, 0x1b000000, 0x31800000,
     0x60c00000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x5f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0xfff00000, 0xfff00000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x60_pixels[]
 = { 0x00000000, 0x00000000, 0x01000000, 0x03000000, 0x06000000, 0x06000000,
     0x07800000, 0x07800000, 0x03000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x61_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x62_pixels[]
 = { 0x00000000, 0x00000000, 0x20000000, 0x60000000, 0xe0000000, 0x60000000,
     0x60000000, 0x67800000, 0x6fc00000, 0x70e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70600000, 0x78c00000, 0x4f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x63_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x1f800000, 0x31c00000, 0x20c00000, 0x60000000, 0x60000000,
     0x60000000, 0x60000000, 0x70400000, 0x30c00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x64_pixels[]
 = { 0x00000000, 0x00000000, 0x00600000, 0x00e00000, 0x00600000, 0x00600000,
     0x00600000, 0x0f600000, 0x31e00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70e00000, 0x39600000, 0x1e700000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x65_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0f000000, 0x30c00000, 0x60600000, 0x60600000, 0x7fe00000,
     0x60000000, 0x60000000, 0x30000000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x66_pixels[]
 = { 0x00000000, 0x00000000, 0x03800000, 0x04c00000, 0x04c00000, 0x0c000000,
     0x0c000000, 0x0c000000, 0x0c000000, 0x1f800000, 0x0c000000, 0x0c000000,
     0x0c000000, 0x0c000000, 0x0c000000, 0x0c000000, 0x1e000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x67_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x1f200000, 0x31e00000, 0x60c00000, 0x60c00000, 0x60c00000,
     0x31800000, 0x3f000000, 0x60000000, 0x7fc00000, 0x3fe00000, 0x20600000,
     0x40200000, 0x40200000, 0x7fc00000, 0x3f800000 };
static u_int32_t dwsfont_0x68_pixels[]
 = { 0x00000000, 0x00000000, 0x10000000, 0x30000000, 0x70000000, 0x30000000,
     0x30000000, 0x37800000, 0x39c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x79e00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x69_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x6a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00c00000, 0x00c00000, 0x00000000,
     0x00000000, 0x03c00000, 0x00c00000, 0x00c00000, 0x00c00000, 0x00c00000,
     0x00c00000, 0x00c00000, 0x00c00000, 0x00c00000, 0x00c00000, 0x20c00000,
     0x30c00000, 0x38800000, 0x1f000000, 0x0e000000 };
static u_int32_t dwsfont_0x6b_pixels[]
 = { 0x00000000, 0x00000000, 0x60000000, 0xe0000000, 0x60000000, 0x60000000,
     0x60000000, 0x61c00000, 0x63000000, 0x66000000, 0x7c000000, 0x78000000,
     0x7c000000, 0x6e000000, 0x67000000, 0x63800000, 0xf1e00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x6c_pixels[]
 = { 0x00000000, 0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x6d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xddc00000, 0x6ee00000, 0x66600000, 0x66600000, 0x66600000,
     0x66600000, 0x66600000, 0x66600000, 0x66600000, 0xef700000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x6e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x27800000, 0x79c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x79e00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x6f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x70_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xef800000, 0x71c00000, 0x60e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x60400000, 0x70800000, 0x7f000000, 0x60000000,
     0x60000000, 0x60000000, 0x60000000, 0xf0000000 };
static u_int32_t dwsfont_0x71_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0f200000, 0x11e00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70600000, 0x38e00000, 0x1fe00000, 0x00600000,
     0x00600000, 0x00600000, 0x00600000, 0x00f00000 };
static u_int32_t dwsfont_0x72_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x73800000, 0x34c00000, 0x38c00000, 0x30000000, 0x30000000,
     0x30000000, 0x30000000, 0x30000000, 0x30000000, 0x78000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x73_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x1fc00000, 0x30c00000, 0x30400000, 0x38000000, 0x1e000000,
     0x07800000, 0x01c00000, 0x20c00000, 0x30c00000, 0x3f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x74_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x04000000, 0x04000000,
     0x0c000000, 0x7fc00000, 0x0c000000, 0x0c000000, 0x0c000000, 0x0c000000,
     0x0c000000, 0x0c000000, 0x0c200000, 0x0e400000, 0x07800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x75_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x76_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xf0700000, 0x60200000, 0x30400000, 0x30400000, 0x18800000,
     0x18800000, 0x0d000000, 0x0d000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x77_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xff700000, 0x66200000, 0x66200000, 0x66200000, 0x37400000,
     0x3b400000, 0x3b400000, 0x19800000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x78_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xf8f00000, 0x70400000, 0x38800000, 0x1d000000, 0x0e000000,
     0x07000000, 0x0b800000, 0x11c00000, 0x20e00000, 0xf1f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x79_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0xf0f00000, 0x60200000, 0x30400000, 0x30400000, 0x18800000,
     0x18800000, 0x0d000000, 0x0d000000, 0x06000000, 0x06000000, 0x04000000,
     0x0c000000, 0x08000000, 0x78000000, 0x70000000 };
static u_int32_t dwsfont_0x7a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x7fe00000, 0x60e00000, 0x41c00000, 0x03800000, 0x07000000,
     0x0e000000, 0x1c000000, 0x38200000, 0x70600000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x7b_pixels[]
 = { 0x00000000, 0x00000000, 0x01c00000, 0x03000000, 0x03000000, 0x01800000,
     0x01800000, 0x01800000, 0x03000000, 0x07000000, 0x03000000, 0x01800000,
     0x01800000, 0x01800000, 0x03000000, 0x03000000, 0x01c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x7c_pixels[]
 = { 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0x7d_pixels[]
 = { 0x00000000, 0x00000000, 0x38000000, 0x0c000000, 0x0c000000, 0x18000000,
     0x18000000, 0x18000000, 0x0c000000, 0x0e000000, 0x0c000000, 0x18000000,
     0x18000000, 0x18000000, 0x0c000000, 0x0c000000, 0x38000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x7e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1c200000,
     0x3e600000, 0x36c00000, 0x67c00000, 0x43800000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x7f_pixels[]
 = { 0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000,
     0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000,
     0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000,
     0xaaa00000, 0x55500000, 0xaaa00000, 0x55500000 };
static u_int32_t dwsfont_0x80_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x81_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x82_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x83_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x84_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x85_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x86_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x87_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x88_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x89_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x8f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x90_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x91_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x92_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x93_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x94_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x95_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x96_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x97_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x98_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x99_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9a_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9b_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9c_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9d_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9e_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0x9f_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa0_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa1_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa2_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x01000000, 0x03000000,
     0x02000000, 0x1f000000, 0x37800000, 0x25800000, 0x64000000, 0x6c000000,
     0x68000000, 0x78800000, 0x39800000, 0x1f000000, 0x10000000, 0x30000000,
     0x20000000, 0x20000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa3_pixels[]
 = { 0x00000000, 0x06000000, 0x0c000000, 0x10000000, 0x10000000, 0x30000000,
     0x30000000, 0x30000000, 0x3e000000, 0x7c000000, 0x18000000, 0x18000000,
     0x18000000, 0x18000000, 0x3f200000, 0x3fe00000, 0x31c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa4_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x60200000,
     0x77400000, 0x3b800000, 0x11c00000, 0x30c00000, 0x30c00000, 0x38800000,
     0x1dc00000, 0x2ee00000, 0x40600000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa5_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xf0700000, 0x60200000, 0x30400000,
     0x18800000, 0x18800000, 0x0d000000, 0x06000000, 0x3fc00000, 0x06000000,
     0x3fc00000, 0x06000000, 0x06000000, 0x06000000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa6_pixels[]
 = { 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000 };
static u_int32_t dwsfont_0xa7_pixels[]
 = { 0x00000000, 0x00000000, 0x0fe00000, 0x18600000, 0x30200000, 0x38200000,
     0x1e000000, 0x1f800000, 0x31c00000, 0x60e00000, 0x70600000, 0x38c00000,
     0x1f800000, 0x07800000, 0x41c00000, 0x40c00000, 0x61800000, 0x7f000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa8_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xa9_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0f000000, 0x10800000, 0x20400000,
     0x2f400000, 0x59a00000, 0x59a00000, 0x58200000, 0x58200000, 0x59a00000,
     0x59a00000, 0x2f400000, 0x20400000, 0x10800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xaa_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0e000000, 0x1f000000, 0x13800000,
     0x0f800000, 0x19800000, 0x31800000, 0x3fc00000, 0x1ec00000, 0x00000000,
     0x00000000, 0x3fe00000, 0x7fc00000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xab_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x02200000, 0x04400000, 0x08800000, 0x11000000,
     0x33000000, 0x19800000, 0x0cc00000, 0x06600000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xac_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x7fe00000, 0x7fe00000, 0x00600000,
     0x00600000, 0x00600000, 0x00600000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xad_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1f800000,
     0x1f800000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xae_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0f000000, 0x10800000, 0x20400000,
     0x3f400000, 0x59a00000, 0x59a00000, 0x5fa00000, 0x5b200000, 0x5b200000,
     0x59a00000, 0x39c00000, 0x20400000, 0x10800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xaf_pixels[]
 = { 0x3fc00000, 0x3fc00000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb0_pixels[]
 = { 0x00000000, 0x00000000, 0x0e000000, 0x17000000, 0x23800000, 0x61800000,
     0x61800000, 0x71000000, 0x3a000000, 0x1c000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb1_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x7fe00000, 0x7fe00000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x00000000, 0x7ff00000, 0x7ff00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb2_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0e000000, 0x1f000000, 0x13000000,
     0x03000000, 0x06000000, 0x0c000000, 0x18000000, 0x1f000000, 0x1f000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb3_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0e000000, 0x1f000000, 0x13000000,
     0x03000000, 0x06000000, 0x03000000, 0x13000000, 0x1f000000, 0x0e000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb4_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb5_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x3e600000, 0x30000000,
     0x30000000, 0x30000000, 0x30000000, 0x60000000 };
static u_int32_t dwsfont_0xb6_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1fc00000, 0x3ec00000, 0x7ec00000,
     0x7ec00000, 0x7ec00000, 0x3ec00000, 0x1ec00000, 0x06c00000, 0x06c00000,
     0x06c00000, 0x06c00000, 0x06c00000, 0x06c00000, 0x06c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb7_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000,
     0x0f000000, 0x06000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xb8_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000,
     0x03000000, 0x01800000, 0x09800000, 0x07000000 };
static u_int32_t dwsfont_0xb9_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0e000000, 0x0e000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x0f000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xba_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x07000000, 0x0b800000, 0x11c00000,
     0x30c00000, 0x30c00000, 0x38800000, 0x1d000000, 0x0e000000, 0x00000000,
     0x00000000, 0x3fe00000, 0x7fc00000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xbb_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x66000000, 0x33000000, 0x19800000, 0x0cc00000,
     0x08800000, 0x11000000, 0x22000000, 0x44000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xbc_pixels[]
 = { 0x00000000, 0x18000000, 0x38000000, 0x38000000, 0x18000000, 0x18200000,
     0x18600000, 0x18c00000, 0x19800000, 0x3f400000, 0x06c00000, 0x0dc00000,
     0x19c00000, 0x32c00000, 0x64c00000, 0x47e00000, 0x00c00000, 0x00c00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xbd_pixels[]
 = { 0x00000000, 0x30000000, 0x70000000, 0x70000000, 0x30200000, 0x30600000,
     0x30c00000, 0x31800000, 0x33000000, 0x7fc00000, 0x0fe00000, 0x1b600000,
     0x32600000, 0x60c00000, 0x41800000, 0x03000000, 0x03e00000, 0x03e00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xbe_pixels[]
 = { 0x00000000, 0x38000000, 0x7c000000, 0x4c000000, 0x0c000000, 0x18200000,
     0x0c600000, 0x4cc00000, 0x7d800000, 0x3b400000, 0x06c00000, 0x0dc00000,
     0x19c00000, 0x32c00000, 0x64c00000, 0x47e00000, 0x00c00000, 0x00c00000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xbf_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x03000000, 0x00000000,
     0x00000000, 0x03000000, 0x03000000, 0x06000000, 0x0c000000, 0x18000000,
     0x30000000, 0x30000000, 0x30400000, 0x39c00000, 0x1f800000, 0x0f000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc0_pixels[]
 = { 0x1c000000, 0x0f000000, 0x03800000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc1_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc2_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc3_pixels[]
 = { 0x0cc00000, 0x1f800000, 0x33000000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc4_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0x06000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc5_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0x0f000000, 0x06000000, 0x0b000000,
     0x0b000000, 0x09000000, 0x11800000, 0x11800000, 0x10800000, 0x3fc00000,
     0x20c00000, 0x20400000, 0x40600000, 0x40600000, 0xe0f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc6_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0fe00000, 0x0e200000, 0x16200000,
     0x16000000, 0x16000000, 0x16400000, 0x27c00000, 0x26400000, 0x3e000000,
     0x26000000, 0x46000000, 0x46100000, 0x46100000, 0xe7f00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc7_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0fc00000, 0x10600000, 0x20200000,
     0x20000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
     0x60000000, 0x20000000, 0x30200000, 0x18400000, 0x0f800000, 0x02000000,
     0x03000000, 0x01800000, 0x09800000, 0x07000000 };
static u_int32_t dwsfont_0xc8_pixels[]
 = { 0x1c000000, 0x0f000000, 0x03800000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xc9_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xca_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xcb_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0x7fc00000, 0x30400000, 0x30400000,
     0x30000000, 0x30000000, 0x30800000, 0x3f800000, 0x30800000, 0x30000000,
     0x30000000, 0x30000000, 0x30200000, 0x30200000, 0x7fe00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xcc_pixels[]
 = { 0x1c000000, 0x0f000000, 0x03800000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xcd_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xce_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xcf_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0x1f800000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd0_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0xff000000, 0x61c00000, 0x60c00000,
     0x60600000, 0x60600000, 0x60600000, 0xf8600000, 0xf8600000, 0x60600000,
     0x60600000, 0x60600000, 0x60400000, 0x61800000, 0xfe000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd1_pixels[]
 = { 0x0cc00000, 0x1f800000, 0x33000000, 0xc0700000, 0x60200000, 0x70200000,
     0x78200000, 0x58200000, 0x4c200000, 0x46200000, 0x47200000, 0x43200000,
     0x41a00000, 0x40e00000, 0x40e00000, 0x40600000, 0xe0300000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd2_pixels[]
 = { 0x1c000000, 0x0f000000, 0x03800000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd3_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd4_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd5_pixels[]
 = { 0x0cc00000, 0x1f800000, 0x33000000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd6_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0x0f000000, 0x11c00000, 0x20c00000,
     0x20600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000, 0x60600000,
     0x60600000, 0x20400000, 0x30400000, 0x18800000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd7_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x60600000, 0x30c00000, 0x19800000, 0x0f000000, 0x06000000,
     0x0f000000, 0x19800000, 0x30c00000, 0x60600000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd8_pixels[]
 = { 0x00000000, 0x00000000, 0x00600000, 0x0fc00000, 0x11c00000, 0x21c00000,
     0x21e00000, 0x63600000, 0x63600000, 0x66600000, 0x6c600000, 0x6c600000,
     0x78600000, 0x38400000, 0x30400000, 0x38800000, 0x6f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xd9_pixels[]
 = { 0x1c000000, 0x0f000000, 0x03800000, 0xf0700000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x70400000, 0x3fc00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xda_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0xf0700000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x70400000, 0x3fc00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xdb_pixels[]
 = { 0x06000000, 0x0f000000, 0x19800000, 0xf0700000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x70400000, 0x3fc00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xdc_pixels[]
 = { 0x19800000, 0x19800000, 0x00000000, 0xf0700000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000, 0x60200000,
     0x60200000, 0x60200000, 0x70400000, 0x3fc00000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xdd_pixels[]
 = { 0x03800000, 0x0f000000, 0x1c000000, 0xf0700000, 0x60200000, 0x30400000,
     0x18800000, 0x18800000, 0x0d000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x0f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xde_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x78000000, 0x30000000, 0x30000000,
     0x3f800000, 0x30c00000, 0x30600000, 0x30600000, 0x30600000, 0x30600000,
     0x30c00000, 0x3f800000, 0x30000000, 0x30000000, 0x78000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xdf_pixels[]
 = { 0x00000000, 0x00000000, 0x0f000000, 0x19800000, 0x19800000, 0x31800000,
     0x31800000, 0x33800000, 0x36000000, 0x36000000, 0x36000000, 0x33800000,
     0x31c00000, 0x30e00000, 0x34600000, 0x36600000, 0x77c00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe0_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x0f000000, 0x03800000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe1_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe2_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000, 0x19800000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe3_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0cc00000, 0x1f800000, 0x33000000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe4_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe5_pixels[]
 = { 0x00000000, 0x06000000, 0x0f000000, 0x19800000, 0x0f000000, 0x06000000,
     0x00000000, 0x0f800000, 0x18c00000, 0x10c00000, 0x03c00000, 0x1cc00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1ee00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe6_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x1f800000, 0x36400000, 0x26600000, 0x0e600000, 0x3fe00000,
     0x66000000, 0x66000000, 0x66000000, 0x67600000, 0x3fc00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe7_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x1f800000, 0x31c00000, 0x20c00000, 0x60000000, 0x60000000,
     0x60000000, 0x60000000, 0x70400000, 0x30c00000, 0x1f800000, 0x02000000,
     0x03000000, 0x01800000, 0x09800000, 0x07000000 };
static u_int32_t dwsfont_0xe8_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x0f000000, 0x03800000,
     0x00000000, 0x0f000000, 0x30c00000, 0x60600000, 0x60600000, 0x7fe00000,
     0x60000000, 0x60000000, 0x30000000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xe9_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0x0f000000, 0x30c00000, 0x60600000, 0x60600000, 0x7fe00000,
     0x60000000, 0x60000000, 0x30000000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xea_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000, 0x19800000,
     0x00000000, 0x0f000000, 0x30c00000, 0x60600000, 0x60600000, 0x7fe00000,
     0x60000000, 0x60000000, 0x30000000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xeb_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x0f000000, 0x30c00000, 0x60600000, 0x60600000, 0x7fe00000,
     0x60000000, 0x60000000, 0x30000000, 0x18600000, 0x0f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xec_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x0f000000, 0x03800000,
     0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xed_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xee_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000, 0x19800000,
     0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xef_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x1e000000, 0x06000000, 0x06000000, 0x06000000, 0x06000000,
     0x06000000, 0x06000000, 0x06000000, 0x06000000, 0x1f800000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf0_pixels[]
 = { 0x00000000, 0x00000000, 0x30c00000, 0x1f800000, 0x06000000, 0x1f000000,
     0x31800000, 0x01c00000, 0x0fc00000, 0x10e00000, 0x20e00000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf1_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0cc00000, 0x1f800000, 0x33000000,
     0x00000000, 0x27800000, 0x79c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x79e00000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf2_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x0f000000, 0x03800000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf3_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf4_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000, 0x19800000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf5_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x0cc00000, 0x1f800000, 0x33000000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf6_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x0f800000, 0x11c00000, 0x20e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x70400000, 0x38800000, 0x1f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf7_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x06000000, 0x06000000, 0x00000000, 0x00000000, 0x7fe00000,
     0x7fe00000, 0x00000000, 0x00000000, 0x06000000, 0x06000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf8_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x0fe00000, 0x11c00000, 0x21e00000, 0x63600000, 0x66600000,
     0x66600000, 0x6c600000, 0x78400000, 0x38800000, 0x7f000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xf9_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x0f000000, 0x03800000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xfa_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xfb_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x06000000, 0x0f000000, 0x19800000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xfc_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0x79e00000, 0x30c00000, 0x30c00000, 0x30c00000, 0x30c00000,
     0x30c00000, 0x30c00000, 0x30c00000, 0x39c00000, 0x1e600000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static u_int32_t dwsfont_0xfd_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x03800000, 0x0f000000, 0x1c000000,
     0x00000000, 0xf0f00000, 0x60200000, 0x30400000, 0x30400000, 0x18800000,
     0x18800000, 0x0d000000, 0x0d000000, 0x06000000, 0x06000000, 0x04000000,
     0x0c000000, 0x08000000, 0x78000000, 0x70000000 };
static u_int32_t dwsfont_0xfe_pixels[]
 = { 0x00000000, 0x00000000, 0xe0000000, 0x60000000, 0x60000000, 0x60000000,
     0x60000000, 0x6f800000, 0x71c00000, 0x60e00000, 0x60600000, 0x60600000,
     0x60600000, 0x60600000, 0x60400000, 0x70800000, 0x7f000000, 0x60000000,
     0x60000000, 0x60000000, 0x60000000, 0xf0000000 };
static u_int32_t dwsfont_0xff_pixels[]
 = { 0x00000000, 0x00000000, 0x00000000, 0x19800000, 0x19800000, 0x00000000,
     0x00000000, 0xf0f00000, 0x60200000, 0x30400000, 0x30400000, 0x18800000,
     0x18800000, 0x0d000000, 0x0d000000, 0x06000000, 0x06000000, 0x04000000,
     0x0c000000, 0x08000000, 0x78000000, 0x70000000 };
#define DWSFONT_CWIDTH 12
#define DWSFONT_CHEIGHT 22
#define DWSFONT_ASCENT 16
#endif

static struct raster dwsfont_0x00 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x00_pixels, 0 };
static struct raster dwsfont_0x01 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x01_pixels, 0 };
static struct raster dwsfont_0x02 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x02_pixels, 0 };
static struct raster dwsfont_0x03 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x03_pixels, 0 };
static struct raster dwsfont_0x04 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x04_pixels, 0 };
static struct raster dwsfont_0x05 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x05_pixels, 0 };
static struct raster dwsfont_0x06 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x06_pixels, 0 };
static struct raster dwsfont_0x07 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x07_pixels, 0 };
static struct raster dwsfont_0x08 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x08_pixels, 0 };
static struct raster dwsfont_0x09 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x09_pixels, 0 };
static struct raster dwsfont_0x0a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0a_pixels, 0 };
static struct raster dwsfont_0x0b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0b_pixels, 0 };
static struct raster dwsfont_0x0c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0c_pixels, 0 };
static struct raster dwsfont_0x0d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0d_pixels, 0 };
static struct raster dwsfont_0x0e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0e_pixels, 0 };
static struct raster dwsfont_0x0f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x0f_pixels, 0 };
static struct raster dwsfont_0x10 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x10_pixels, 0 };
static struct raster dwsfont_0x11 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x11_pixels, 0 };
static struct raster dwsfont_0x12 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x12_pixels, 0 };
static struct raster dwsfont_0x13 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x13_pixels, 0 };
static struct raster dwsfont_0x14 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x14_pixels, 0 };
static struct raster dwsfont_0x15 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x15_pixels, 0 };
static struct raster dwsfont_0x16 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x16_pixels, 0 };
static struct raster dwsfont_0x17 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x17_pixels, 0 };
static struct raster dwsfont_0x18 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x18_pixels, 0 };
static struct raster dwsfont_0x19 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x19_pixels, 0 };
static struct raster dwsfont_0x1a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1a_pixels, 0 };
static struct raster dwsfont_0x1b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1b_pixels, 0 };
static struct raster dwsfont_0x1c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1c_pixels, 0 };
static struct raster dwsfont_0x1d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1d_pixels, 0 };
static struct raster dwsfont_0x1e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1e_pixels, 0 };
static struct raster dwsfont_0x1f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x1f_pixels, 0 };
static struct raster dwsfont_0x20 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x20_pixels, 0 };
static struct raster dwsfont_0x21 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x21_pixels, 0 };
static struct raster dwsfont_0x22 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x22_pixels, 0 };
static struct raster dwsfont_0x23 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x23_pixels, 0 };
static struct raster dwsfont_0x24 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x24_pixels, 0 };
static struct raster dwsfont_0x25 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x25_pixels, 0 };
static struct raster dwsfont_0x26 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x26_pixels, 0 };
static struct raster dwsfont_0x27 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x27_pixels, 0 };
static struct raster dwsfont_0x28 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x28_pixels, 0 };
static struct raster dwsfont_0x29 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x29_pixels, 0 };
static struct raster dwsfont_0x2a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2a_pixels, 0 };
static struct raster dwsfont_0x2b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2b_pixels, 0 };
static struct raster dwsfont_0x2c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2c_pixels, 0 };
static struct raster dwsfont_0x2d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2d_pixels, 0 };
static struct raster dwsfont_0x2e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2e_pixels, 0 };
static struct raster dwsfont_0x2f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x2f_pixels, 0 };
static struct raster dwsfont_0x30 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x30_pixels, 0 };
static struct raster dwsfont_0x31 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x31_pixels, 0 };
static struct raster dwsfont_0x32 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x32_pixels, 0 };
static struct raster dwsfont_0x33 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x33_pixels, 0 };
static struct raster dwsfont_0x34 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x34_pixels, 0 };
static struct raster dwsfont_0x35 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x35_pixels, 0 };
static struct raster dwsfont_0x36 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x36_pixels, 0 };
static struct raster dwsfont_0x37 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x37_pixels, 0 };
static struct raster dwsfont_0x38 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x38_pixels, 0 };
static struct raster dwsfont_0x39 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x39_pixels, 0 };
static struct raster dwsfont_0x3a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3a_pixels, 0 };
static struct raster dwsfont_0x3b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3b_pixels, 0 };
static struct raster dwsfont_0x3c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3c_pixels, 0 };
static struct raster dwsfont_0x3d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3d_pixels, 0 };
static struct raster dwsfont_0x3e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3e_pixels, 0 };
static struct raster dwsfont_0x3f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x3f_pixels, 0 };
static struct raster dwsfont_0x40 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x40_pixels, 0 };
static struct raster dwsfont_0x41 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x41_pixels, 0 };
static struct raster dwsfont_0x42 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x42_pixels, 0 };
static struct raster dwsfont_0x43 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x43_pixels, 0 };
static struct raster dwsfont_0x44 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x44_pixels, 0 };
static struct raster dwsfont_0x45 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x45_pixels, 0 };
static struct raster dwsfont_0x46 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x46_pixels, 0 };
static struct raster dwsfont_0x47 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x47_pixels, 0 };
static struct raster dwsfont_0x48 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x48_pixels, 0 };
static struct raster dwsfont_0x49 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x49_pixels, 0 };
static struct raster dwsfont_0x4a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4a_pixels, 0 };
static struct raster dwsfont_0x4b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4b_pixels, 0 };
static struct raster dwsfont_0x4c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4c_pixels, 0 };
static struct raster dwsfont_0x4d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4d_pixels, 0 };
static struct raster dwsfont_0x4e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4e_pixels, 0 };
static struct raster dwsfont_0x4f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x4f_pixels, 0 };
static struct raster dwsfont_0x50 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x50_pixels, 0 };
static struct raster dwsfont_0x51 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x51_pixels, 0 };
static struct raster dwsfont_0x52 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x52_pixels, 0 };
static struct raster dwsfont_0x53 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x53_pixels, 0 };
static struct raster dwsfont_0x54 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x54_pixels, 0 };
static struct raster dwsfont_0x55 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x55_pixels, 0 };
static struct raster dwsfont_0x56 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x56_pixels, 0 };
static struct raster dwsfont_0x57 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x57_pixels, 0 };
static struct raster dwsfont_0x58 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x58_pixels, 0 };
static struct raster dwsfont_0x59 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x59_pixels, 0 };
static struct raster dwsfont_0x5a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5a_pixels, 0 };
static struct raster dwsfont_0x5b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5b_pixels, 0 };
static struct raster dwsfont_0x5c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5c_pixels, 0 };
static struct raster dwsfont_0x5d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5d_pixels, 0 };
static struct raster dwsfont_0x5e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5e_pixels, 0 };
static struct raster dwsfont_0x5f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x5f_pixels, 0 };
static struct raster dwsfont_0x60 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x60_pixels, 0 };
static struct raster dwsfont_0x61 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x61_pixels, 0 };
static struct raster dwsfont_0x62 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x62_pixels, 0 };
static struct raster dwsfont_0x63 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x63_pixels, 0 };
static struct raster dwsfont_0x64 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x64_pixels, 0 };
static struct raster dwsfont_0x65 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x65_pixels, 0 };
static struct raster dwsfont_0x66 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x66_pixels, 0 };
static struct raster dwsfont_0x67 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x67_pixels, 0 };
static struct raster dwsfont_0x68 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x68_pixels, 0 };
static struct raster dwsfont_0x69 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x69_pixels, 0 };
static struct raster dwsfont_0x6a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6a_pixels, 0 };
static struct raster dwsfont_0x6b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6b_pixels, 0 };
static struct raster dwsfont_0x6c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6c_pixels, 0 };
static struct raster dwsfont_0x6d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6d_pixels, 0 };
static struct raster dwsfont_0x6e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6e_pixels, 0 };
static struct raster dwsfont_0x6f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x6f_pixels, 0 };
static struct raster dwsfont_0x70 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x70_pixels, 0 };
static struct raster dwsfont_0x71 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x71_pixels, 0 };
static struct raster dwsfont_0x72 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x72_pixels, 0 };
static struct raster dwsfont_0x73 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x73_pixels, 0 };
static struct raster dwsfont_0x74 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x74_pixels, 0 };
static struct raster dwsfont_0x75 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x75_pixels, 0 };
static struct raster dwsfont_0x76 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x76_pixels, 0 };
static struct raster dwsfont_0x77 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x77_pixels, 0 };
static struct raster dwsfont_0x78 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x78_pixels, 0 };
static struct raster dwsfont_0x79 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x79_pixels, 0 };
static struct raster dwsfont_0x7a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7a_pixels, 0 };
static struct raster dwsfont_0x7b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7b_pixels, 0 };
static struct raster dwsfont_0x7c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7c_pixels, 0 };
static struct raster dwsfont_0x7d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7d_pixels, 0 };
static struct raster dwsfont_0x7e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7e_pixels, 0 };
static struct raster dwsfont_0x7f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x7f_pixels, 0 };
static struct raster dwsfont_0x80 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x80_pixels, 0 };
static struct raster dwsfont_0x81 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x81_pixels, 0 };
static struct raster dwsfont_0x82 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x82_pixels, 0 };
static struct raster dwsfont_0x83 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x83_pixels, 0 };
static struct raster dwsfont_0x84 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x84_pixels, 0 };
static struct raster dwsfont_0x85 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x85_pixels, 0 };
static struct raster dwsfont_0x86 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x86_pixels, 0 };
static struct raster dwsfont_0x87 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x87_pixels, 0 };
static struct raster dwsfont_0x88 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x88_pixels, 0 };
static struct raster dwsfont_0x89 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x89_pixels, 0 };
static struct raster dwsfont_0x8a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8a_pixels, 0 };
static struct raster dwsfont_0x8b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8b_pixels, 0 };
static struct raster dwsfont_0x8c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8c_pixels, 0 };
static struct raster dwsfont_0x8d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8d_pixels, 0 };
static struct raster dwsfont_0x8e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8e_pixels, 0 };
static struct raster dwsfont_0x8f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x8f_pixels, 0 };
static struct raster dwsfont_0x90 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x90_pixels, 0 };
static struct raster dwsfont_0x91 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x91_pixels, 0 };
static struct raster dwsfont_0x92 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x92_pixels, 0 };
static struct raster dwsfont_0x93 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x93_pixels, 0 };
static struct raster dwsfont_0x94 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x94_pixels, 0 };
static struct raster dwsfont_0x95 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x95_pixels, 0 };
static struct raster dwsfont_0x96 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x96_pixels, 0 };
static struct raster dwsfont_0x97 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x97_pixels, 0 };
static struct raster dwsfont_0x98 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x98_pixels, 0 };
static struct raster dwsfont_0x99 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x99_pixels, 0 };
static struct raster dwsfont_0x9a = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9a_pixels, 0 };
static struct raster dwsfont_0x9b = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9b_pixels, 0 };
static struct raster dwsfont_0x9c = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9c_pixels, 0 };
static struct raster dwsfont_0x9d = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9d_pixels, 0 };
static struct raster dwsfont_0x9e = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9e_pixels, 0 };
static struct raster dwsfont_0x9f = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0x9f_pixels, 0 };
static struct raster dwsfont_0xa0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa0_pixels, 0 };
static struct raster dwsfont_0xa1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa1_pixels, 0 };
static struct raster dwsfont_0xa2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa2_pixels, 0 };
static struct raster dwsfont_0xa3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa3_pixels, 0 };
static struct raster dwsfont_0xa4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa4_pixels, 0 };
static struct raster dwsfont_0xa5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa5_pixels, 0 };
static struct raster dwsfont_0xa6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa6_pixels, 0 };
static struct raster dwsfont_0xa7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa7_pixels, 0 };
static struct raster dwsfont_0xa8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa8_pixels, 0 };
static struct raster dwsfont_0xa9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xa9_pixels, 0 };
static struct raster dwsfont_0xaa = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xaa_pixels, 0 };
static struct raster dwsfont_0xab = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xab_pixels, 0 };
static struct raster dwsfont_0xac = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xac_pixels, 0 };
static struct raster dwsfont_0xad = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xad_pixels, 0 };
static struct raster dwsfont_0xae = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xae_pixels, 0 };
static struct raster dwsfont_0xaf = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xaf_pixels, 0 };
static struct raster dwsfont_0xb0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb0_pixels, 0 };
static struct raster dwsfont_0xb1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb1_pixels, 0 };
static struct raster dwsfont_0xb2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb2_pixels, 0 };
static struct raster dwsfont_0xb3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb3_pixels, 0 };
static struct raster dwsfont_0xb4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb4_pixels, 0 };
static struct raster dwsfont_0xb5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb5_pixels, 0 };
static struct raster dwsfont_0xb6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb6_pixels, 0 };
static struct raster dwsfont_0xb7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb7_pixels, 0 };
static struct raster dwsfont_0xb8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb8_pixels, 0 };
static struct raster dwsfont_0xb9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xb9_pixels, 0 };
static struct raster dwsfont_0xba = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xba_pixels, 0 };
static struct raster dwsfont_0xbb = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xbb_pixels, 0 };
static struct raster dwsfont_0xbc = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xbc_pixels, 0 };
static struct raster dwsfont_0xbd = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xbd_pixels, 0 };
static struct raster dwsfont_0xbe = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xbe_pixels, 0 };
static struct raster dwsfont_0xbf = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xbf_pixels, 0 };
static struct raster dwsfont_0xc0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc0_pixels, 0 };
static struct raster dwsfont_0xc1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc1_pixels, 0 };
static struct raster dwsfont_0xc2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc2_pixels, 0 };
static struct raster dwsfont_0xc3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc3_pixels, 0 };
static struct raster dwsfont_0xc4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc4_pixels, 0 };
static struct raster dwsfont_0xc5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc5_pixels, 0 };
static struct raster dwsfont_0xc6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc6_pixels, 0 };
static struct raster dwsfont_0xc7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc7_pixels, 0 };
static struct raster dwsfont_0xc8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc8_pixels, 0 };
static struct raster dwsfont_0xc9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xc9_pixels, 0 };
static struct raster dwsfont_0xca = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xca_pixels, 0 };
static struct raster dwsfont_0xcb = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xcb_pixels, 0 };
static struct raster dwsfont_0xcc = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xcc_pixels, 0 };
static struct raster dwsfont_0xcd = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xcd_pixels, 0 };
static struct raster dwsfont_0xce = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xce_pixels, 0 };
static struct raster dwsfont_0xcf = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xcf_pixels, 0 };
static struct raster dwsfont_0xd0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd0_pixels, 0 };
static struct raster dwsfont_0xd1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd1_pixels, 0 };
static struct raster dwsfont_0xd2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd2_pixels, 0 };
static struct raster dwsfont_0xd3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd3_pixels, 0 };
static struct raster dwsfont_0xd4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd4_pixels, 0 };
static struct raster dwsfont_0xd5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd5_pixels, 0 };
static struct raster dwsfont_0xd6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd6_pixels, 0 };
static struct raster dwsfont_0xd7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd7_pixels, 0 };
static struct raster dwsfont_0xd8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd8_pixels, 0 };
static struct raster dwsfont_0xd9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xd9_pixels, 0 };
static struct raster dwsfont_0xda = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xda_pixels, 0 };
static struct raster dwsfont_0xdb = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xdb_pixels, 0 };
static struct raster dwsfont_0xdc = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xdc_pixels, 0 };
static struct raster dwsfont_0xdd = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xdd_pixels, 0 };
static struct raster dwsfont_0xde = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xde_pixels, 0 };
static struct raster dwsfont_0xdf = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xdf_pixels, 0 };
static struct raster dwsfont_0xe0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe0_pixels, 0 };
static struct raster dwsfont_0xe1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe1_pixels, 0 };
static struct raster dwsfont_0xe2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe2_pixels, 0 };
static struct raster dwsfont_0xe3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe3_pixels, 0 };
static struct raster dwsfont_0xe4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe4_pixels, 0 };
static struct raster dwsfont_0xe5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe5_pixels, 0 };
static struct raster dwsfont_0xe6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe6_pixels, 0 };
static struct raster dwsfont_0xe7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe7_pixels, 0 };
static struct raster dwsfont_0xe8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe8_pixels, 0 };
static struct raster dwsfont_0xe9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xe9_pixels, 0 };
static struct raster dwsfont_0xea = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xea_pixels, 0 };
static struct raster dwsfont_0xeb = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xeb_pixels, 0 };
static struct raster dwsfont_0xec = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xec_pixels, 0 };
static struct raster dwsfont_0xed = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xed_pixels, 0 };
static struct raster dwsfont_0xee = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xee_pixels, 0 };
static struct raster dwsfont_0xef = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xef_pixels, 0 };
static struct raster dwsfont_0xf0 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf0_pixels, 0 };
static struct raster dwsfont_0xf1 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf1_pixels, 0 };
static struct raster dwsfont_0xf2 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf2_pixels, 0 };
static struct raster dwsfont_0xf3 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf3_pixels, 0 };
static struct raster dwsfont_0xf4 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf4_pixels, 0 };
static struct raster dwsfont_0xf5 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf5_pixels, 0 };
static struct raster dwsfont_0xf6 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf6_pixels, 0 };
static struct raster dwsfont_0xf7 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf7_pixels, 0 };
static struct raster dwsfont_0xf8 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf8_pixels, 0 };
static struct raster dwsfont_0xf9 = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xf9_pixels, 0 };
static struct raster dwsfont_0xfa = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xfa_pixels, 0 };
static struct raster dwsfont_0xfb = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xfb_pixels, 0 };
static struct raster dwsfont_0xfc = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xfc_pixels, 0 };
static struct raster dwsfont_0xfd = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xfd_pixels, 0 };
static struct raster dwsfont_0xfe = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xfe_pixels, 0 };
static struct raster dwsfont_0xff = { DWSFONT_CWIDTH, DWSFONT_CHEIGHT, 1, 1, dwsfont_0xff_pixels, 0 };

struct raster_font default_wsfont = {
    DWSFONT_CWIDTH, DWSFONT_CHEIGHT, DWSFONT_ASCENT, RASFONT_FIXEDWIDTH|RASFONT_NOVERTICALMOVEMENT,
    { { &dwsfont_0x00, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x01, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x02, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x03, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x04, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x05, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x06, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x07, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x08, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x09, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x0f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x10, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x11, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x12, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x13, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x14, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x15, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x16, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x17, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x18, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x19, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x1f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x20, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x21, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x22, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x23, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x24, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x25, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x26, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x27, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x28, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x29, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x2f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x30, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x31, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x32, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x33, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x34, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x35, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x36, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x37, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x38, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x39, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x3f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x40, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x41, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x42, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x43, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x44, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x45, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x46, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x47, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x48, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x49, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x4f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x50, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x51, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x52, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x53, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x54, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x55, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x56, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x57, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x58, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x59, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x5f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x60, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x61, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x62, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x63, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x64, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x65, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x66, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x67, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x68, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x69, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x6f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x70, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x71, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x72, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x73, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x74, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x75, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x76, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x77, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x78, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x79, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x7f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x80, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x81, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x82, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x83, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x84, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x85, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x86, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x87, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x88, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x89, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x8f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x90, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x91, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x92, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x93, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x94, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x95, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x96, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x97, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x98, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x99, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9a, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9b, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9c, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9d, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9e, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0x9f, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xa9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xaa, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xab, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xac, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xad, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xae, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xaf, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xb9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xba, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xbb, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xbc, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xbd, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xbe, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xbf, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xc9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xca, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xcb, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xcc, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xcd, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xce, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xcf, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xd9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xda, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xdb, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xdc, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xdd, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xde, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xdf, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xe9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xea, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xeb, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xec, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xed, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xee, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xef, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf0, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf1, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf2, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf3, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf4, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf5, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf6, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf7, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf8, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xf9, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xfa, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xfb, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xfc, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xfd, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xfe, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 },
      { &dwsfont_0xff, 0, -DWSFONT_ASCENT, DWSFONT_CWIDTH, 0 } },
#ifdef COLORFONT_CACHE
    (struct raster_fontcache*) -1
#endif /*COLORFONT_CACHE*/
};
