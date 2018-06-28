#ifndef I86_H
#define I86_H

#include <cstdint>

union REGS {
  struct {
    uint16_t ax;
  } w;
  struct {
    uint8_t al, ah;
  } b;
};

#include <conio.h>

#endif
