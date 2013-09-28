// hidbufs.h ----------------------------------------------------------
//
// Copyright (c) 2001 to 2013  te
//
// Licence: Creative Commons Attribution-ShareAlike 3.0 Unported License.
//          http://creativecommons.org/licenses/by-sa/3.0/
//---------------------------------------------------------------------
#ifndef hidbufsH
#define hidbufsH

#include <stdint.h>
#include <memory.h>
#include <assert.h>

#define HID_RD_BUFF_SIZE      65
#define HID_WR_BUFF_SIZE      64

//---------------------------------------------------------------------
class THidWrBuf {
  private:
    uint8_t Fbuf[HID_WR_BUFF_SIZE];
    int     Flen;

  public:
    THidWrBuf(int v) { clear(); byte(v); }
    THidWrBuf()      { clear(); }

    THidWrBuf& byte(int v) {
      assert(Flen < HID_WR_BUFF_SIZE);
      Fbuf[Flen] = (uint8_t)v;
      Flen++;
      return *this;
      }
    THidWrBuf& wordLE(int v) {
      uint32_t x = v;
      byte(x);
      byte(x >> 8);
      return *this;
      }
    THidWrBuf& wordBE(int v) {
      uint32_t x = v;
      byte(x >> 8);
      byte(x);
      return *this;
      }
    THidWrBuf& dwordLE(int v) {
      uint32_t x = v;
      wordLE(x);
      wordLE(x >> 16);
      return *this;
      }
    THidWrBuf& dwordBE(int v) {
      uint32_t x = v;
      wordBE(x >> 16);
      wordBE(x);
      return *this;
      }

    uint8_t *data() { return Fbuf; }
    int     length(){ return Flen; }
    THidWrBuf& clear() {
      Flen = 0;
      memset(Fbuf, 0, sizeof(Fbuf));
      return *this;
      }
  };

//---------------------------------------------------------------------
class THidRdBuf {
  private:
    uint8_t Fbuf[HID_RD_BUFF_SIZE];
    unsigned int FrdIx;

    void _init() {
      Flen = 0;
      FrdIx = 0;
      memset(Fbuf, 0, sizeof(Fbuf));
      }

  public:
    int     Flen;

    THidRdBuf(int r) { _init(); Fbuf[0] = (uint8_t)r; }
    THidRdBuf()      { _init(); }
    uint8_t *data() { return Fbuf; }
    void setLength(int len) { Flen = len; }

    uint8_t byte() {
      return (FrdIx < sizeof(Fbuf)) ? Fbuf[FrdIx++] : 0;
      }
    uint16_t wordBE() {
      uint16_t v = byte() << 8;
      v |= byte();
      return v;
      }
    uint32_t dwordBE() {
      uint32_t v = wordBE() << 16;
      v |= wordBE();
      return v;
      }
  };

#endif
// EOF ----------------------------------------------------------------
