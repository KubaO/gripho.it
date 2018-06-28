#ifndef CONIO_H
#define CONIO_H

#include <cstdint>
#include <i86.h>

class CONIO {
  class Private;
  int argc;
  char argv0[1] = "";
  char *argv[2] = { argv0, nullptr };
  char mem[0x100000];
  Private *d = {};
  enum {
    KEYMODS     = 0x0417,
    KEYBUF_NEXT = 0x041A,
    KEYBUF_FREE = 0x041C,
    KEYBUF      = 0x041E,
    KEYBUF_END  = KEYBUF + 32,
    TICKS       = 0x046C
  };

  char ports[0x10000];
  enum {
    P_VGA_ISR1  = 0x03DA  // 3:vsync, 0:display active (not in retrace)
  };

  uint16_t &uint16(int addr) { return (uint16_t&)mem[addr]; }

public:
  CONIO();
  ~CONIO();
  static CONIO& io() {
    static CONIO instance;
    return instance;
  }

  int inp(unsigned port);
  void outp(unsigned port);

  char *vm(int addr);

  int getch();
  void clearKeyBuffer();
  void addKey(int key);
  void addExtKey(int key);
  void addQtKey(int key, int modifiers);

  int16_t *B8000(uintptr_t n=0);
  void int10h(const REGS *in, REGS *out);
  void updateScreen();
  void setMode(int);
};


// I/O Ports

inline int inp(unsigned port) { return CONIO::io().inp(port); }
inline void outp(unsigned port) { CONIO::io().outp(port); }

// Memory

inline char *v_m(int addr) { return CONIO::io().vm(addr); }

// Int Dispatch

inline void int386(int i, const REGS *in, REGS *out) {
  if (i == 0x10)
    CONIO::io().int10h(in, out);
}

// Keyboard

inline int getch() { return CONIO::io().getch(); }

// Video

inline int16_t *B8000(uintptr_t n=0) { return CONIO::io().B8000(n); }

inline int16_t *CONIO::B8000(uintptr_t n) {
  updateScreen();
  return (int16_t*)(mem + 0xB8000 + n);
}

#endif
