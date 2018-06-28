#include <conio.h>
#include <QtGui>

struct DrawUnit {
   QChar ch;
   QPoint pos;
   QColor bg, fg;
};

class Display : public QRasterWindow {
   QSize m_textSize;
   QSize m_glyphSize;
   QPointF m_glyphPos;
   int m_charsPerLine, m_lines;
   QMap<QChar, QImage> m_glyphs;
   QFont m_font{"Monaco", 14};
   QFontMetricsF m_fm{m_font};
   inline int xStep() const { return m_glyphSize.width(); }
   inline int yStep() const { return m_glyphSize.height(); }
   const QImage &glyph(QChar ch);
   void drawChar(QPainter &, const DrawUnit &);

   void keyPressEvent(QKeyEvent *ev) override;
   void paintEvent(QPaintEvent *ev) override;
public:
   Display(QWindow *parent = {});
   void setSize(int width, int height);
};

// CONIO

class CONIO::Private {
public:
   QElapsedTimer timer;
   Display display;
};

CONIO::CONIO()
{
   new QGuiApplication(argc, argv);
   d = new Private;
   d->display.show();
   d->timer.start();
   clearKeyBuffer();
}

CONIO::~CONIO() {
   delete d;
   delete qApp;
}

// I/O Ports

int CONIO::inp(unsigned port) {
   if (port == P_VGA_ISR1) {
      ports[port] ^= 0x09;
   }
   return (port < sizeof(ports)) ? ports[port] : -1;
}

void CONIO::outp(unsigned port) {
}

// Memory

char *CONIO::vm(int addr) {
   if (addr < 0x500) {
      if (addr == KEYBUF_FREE) {
         QCoreApplication::processEvents();
         QThread::yieldCurrentThread();
      }
      else if (addr == TICKS) {
         *(int32_t*)(mem+TICKS) = d->timer.elapsed() / 55;
         QThread::yieldCurrentThread();
      }
   }
   else if (addr >= 0xB8000 && addr < 0xC0000) {
      return (char*)io().B8000(addr - 0xB8000);
   }
   return mem + addr;
}

// Int Dispatch


// Keyboard

int CONIO::getch() {
   while (true) {
      if (uint16(KEYBUF_NEXT) != uint16(KEYBUF_FREE)) {
         uint8_t key = mem[uint16(KEYBUF_NEXT)++];
         if (uint16(KEYBUF_NEXT) == uint16(KEYBUF_FREE))
            clearKeyBuffer();
         return key;
      }
      QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
   }
}

void CONIO::clearKeyBuffer() {
   uint16(KEYBUF_NEXT) = KEYBUF;
   uint16(KEYBUF_FREE) = KEYBUF;
}

void CONIO::addKey(int key) {
   auto end = KEYBUF_END + (key & 0xFF) ? 0 : -1;
   if (uint16(KEYBUF_FREE) < end) {
      mem[uint16(KEYBUF_FREE)++] = key & 0xFF;
      if (!(key & 0xFF))
         mem[uint16(KEYBUF_FREE)++] = (key >> 8);
   }
}

void CONIO::addExtKey(int key) {
   addKey(key << 8);
}

void CONIO::addQtKey(int key, int modifiers) {
   static const struct { int qt, key; } lookup[] = {
   {Qt::Key_Tab | Qt::SHIFT, 0x0f00},
   {Qt::Key_Insert, 0x5200},
   {Qt::Key_Delete, 0x5300},
   {Qt::Key_Print, 0x7200},
   {Qt::Key_Down, 0x5000},
   {Qt::Key_Left, 0x4b00},
   {Qt::Key_Right, 0x4d00},
   {Qt::Key_Up, 0x4800},
   {Qt::Key_End, 0x4F00},
   {Qt::Key_Home, 0x4700},
   {Qt::Key_PageDown, 0x5100},
   {Qt::Key_PageUp, 0x4900},
   {Qt::Key_Left | Qt::CTRL, 0x7300},
   {Qt::Key_Right | Qt::CTRL, 0x7400},
   {Qt::Key_End | Qt::CTRL, 0x7500},
   {Qt::Key_Home | Qt::CTRL, 0x7700},
   {Qt::Key_PageDown | Qt::CTRL, 0x7600},
   {Qt::Key_PageUp | Qt::CTRL, 0x8400},
   {Qt::Key_F11, 0x8500},
   {Qt::Key_F12, 0x8600},
   {Qt::Key_F11 | Qt::SHIFT, 0x8700},
   {Qt::Key_F12 | Qt::SHIFT, 0x8800},
   {Qt::Key_F11 | Qt::CTRL, 0x8900},
   {Qt::Key_F12 | Qt::CTRL, 0x8a00},
   {Qt::Key_F11 | Qt::ALT, 0x8b00},
   {Qt::Key_F12 | Qt::ALT, 0x8c00},
   {0, 0}
};
   static const int altLetters[] = {
      0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31,
      0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c
   };
   static const int altDigits[] = {
      0x81, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80
   };

   int const kmod = key | modifiers;
   if (key >= Qt::Key_F1 && key <= Qt::Key_F10) {
      key -= Qt::Key_F1;
      if (modifiers == 0)
         addExtKey(key+0x3b);
      else if (modifiers == Qt::ShiftModifier)
         addExtKey(key+0x54);
      else if (modifiers == Qt::ControlModifier)
         addExtKey(key+0x5e);
      else if (modifiers == Qt::AltModifier)
         addExtKey(key+0x68);
   }
   else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
      key -= Qt::Key_A;
      if (modifiers == Qt::AltModifier)
         addExtKey(altLetters[key]);
   }
   else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
      key -= Qt::Key_0;
      if (modifiers == Qt::AltModifier)
         addExtKey(altDigits[key]);
   }
   else if (kmod >= Qt::Key_Space && kmod <= Qt::Key_AsciiTilde) {
      addKey(key);
   }
   else {
      for (auto &l : lookup)
         if (l.qt == kmod) {
            addKey(l.key);
            break;
         } else if (!l.qt)
            break;
   }
}

// Video

void CONIO::int10h(const REGS *in, REGS *out) {
   if (in->b.ah == 0x00)
      setMode(in->b.al);
}

void CONIO::updateScreen() {
   d->display.update();
}

void CONIO::setMode(int mode) {
   if (mode == 0x01)
      d->display.setSize(40, 25);
   else if (mode == 0x03)
      d->display.setSize(80, 25);
}

// Display

static auto const CP437 = QStringLiteral(
         " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼▶◀↕‼¶§▬↨↑↓→←∟↔▲▼"
         " !\"#$%&'()*+,-./0123456789:;<=>?"
         "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
         "`abcdefghijklmnopqrstuvwxyz{|}~ "
         "ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ"
         "áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
         "└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
         "αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ ");

static QChar decode(char ch) { return CP437[uchar(ch)]; }

const QImage &Display::glyph(QChar ch) {
   auto &glyph = m_glyphs[ch];
   if (glyph.isNull()) {
      QPointF extent = m_fm.boundingRect(ch).translated(m_glyphPos).bottomRight();
      glyph = QImage(m_glyphSize, QImage::Format_ARGB32_Premultiplied);
      glyph.fill(Qt::transparent);
      QPainter p{&glyph};
      p.setPen(Qt::white);
      p.setFont(m_font);
      p.translate(m_glyphPos);
      p.scale(std::min(1.0, (m_glyphSize.width()-1)/extent.x()),
              std::min(1.0, (m_glyphSize.height()-1)/extent.y()));
      p.drawText(QPointF{}, {ch});
   }
   return glyph;
}

void Display::drawChar(QPainter &p, const DrawUnit &u) {
   auto &glyph = this->glyph(u.ch);
   const QRect rect(u.pos, glyph.size());
   p.setCompositionMode(QPainter::CompositionMode_Source);
   p.drawImage(u.pos, glyph);
   p.setCompositionMode(QPainter::CompositionMode_SourceOut);
   p.fillRect(rect, u.bg);
   p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
   p.fillRect(rect, u.fg);
}

static DrawUnit decodeCell(uint16_t cell) {
   static const QColor vgaColors[] = {
      0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa, 0xffaa0000, 0xffaa00aa,
      0xffaa5500, 0xffaaaaaa, 0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff,
      0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff
   };
   DrawUnit u;
   u.ch = decode(cell & 0x00FF);
   u.fg = vgaColors[(cell & 0x0F00) >> 8];
   u.bg = vgaColors[(cell & 0x7000) >> 12];
   return u;
}

Display::Display(QWindow *parent) : QRasterWindow(parent) {
   QRectF glyphRectF{0., 0., 1., 1.};
   for (int i = 0x20; i < 0xE0; ++i)
      glyphRectF = glyphRectF.united(m_fm.boundingRect(CP437[i]));
   m_glyphPos = -glyphRectF.topLeft();
   m_glyphSize = QSize(std::ceil(glyphRectF.width()), std::ceil(glyphRectF.height()));
   setSize(40, 25);
}

void Display::keyPressEvent(QKeyEvent *ev) {
   CONIO::io().addQtKey(ev->key(), ev->modifiers());
}

void Display::paintEvent(QPaintEvent *) {
   QPainter p(this);
   auto *buf = B8000();
   for (int i = 0; i < m_textSize.height(); ++i) {
      for (int j = 0; j < m_textSize.width(); ++j) {
         auto unit = decodeCell(*buf++);
         unit.pos = {j*xStep(), i*yStep()};
         drawChar(p, unit);
      }
   }
}

void Display::setSize(int width, int height) {
   m_textSize = {width, height};
   resize(size().expandedTo({xStep()*width, yStep()*height}));
   update();
}
