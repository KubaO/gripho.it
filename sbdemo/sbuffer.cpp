/*
** This program is an example of the "Free Span Buffer" algorithm. For more
** infos check
**
**                http://www.netservers.com/~agriff
**
** or mailto:agriff@tin.it 
*/
#include <QtGui>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

int lineStep(const QImage &img, int subWidth) {
   Q_ASSERT((img.bytesPerLine() * 8) % img.depth() == 0);
   return (img.bytesPerLine() * 8) / img.depth() - subWidth;
}

template <typename T = QRgb>
const T *scanLine (const QImage &dst, const QPoint pos) {
   return reinterpret_cast<const T*>(dst.scanLine(pos.y()) + pos.x()*sizeof(T));
}

template <typename T = QRgb>
T *scanLine (QImage &dst, const QPoint pos) {
   return const_cast<T*>(scanLine(const_cast<const QImage &>(dst), pos));
}

template <typename T = int>
class ZBuffer {
   int m_width, m_height, m_lineLength;
   std::vector<T> m_buf;
public:
   using value_type = T;
   static T defaultZ() { return std::numeric_limits<T>::max(); }
   explicit ZBuffer(int width, int height, int lineLength, T value = defaultZ()) :
      m_width(width), m_height(height), m_lineLength(lineLength),
      m_buf(m_lineLength*m_height, value) {}
   explicit ZBuffer(int width, int height, T value = defaultZ()) :
      ZBuffer(width, height, width, value) {}
   inline T *scanLine(const QPoint pos) {
      return m_buf.data() + m_lineLength * pos.y() + pos.x();
   }
   inline int lineStep(int subWidth) const {
      return m_lineLength - subWidth;
   }
   void clear() {
      std::fill(m_buf.begin(), m_buf.end(), defaultZ());
   }
};

QImage WithBorder(QImage img, int width, const QColor &color = Qt::black) {
   QPainter p(&img);
   QPen pen(color);
   pen.setWidth(width);
   p.setPen(pen);
   qreal a = width/2;
   p.drawRect(QRectF(img.rect()).adjusted(a-1,a-1,-a,-a));
   return img;
}

class ImagePainter {
protected:
   const QImage src;
   QImage &dst;
   virtual void draw(const QPoint &topLeft, const QRect &dstRect, const QRect &srcRect) = 0;
public:
   ImagePainter(const QImage &src, QImage &dst) : src(src), dst(dst) {}
   virtual void begin() {}
   virtual void end() {}
   virtual ~ImagePainter() {}
   void draw(const QPoint &center) {
      auto const p = center - src.rect().center();
      auto const dstRect = QRect(p, src.size()).intersected(dst.rect());
      if (!dstRect.isEmpty()) {
         auto const srcRect = src.rect().intersected({-p, dst.size()});
         draw(p, dstRect, srcRect);
      }
   }
};

class DrawPainter : public ImagePainter {
   void draw(const QPoint &, const QRect &dstRect, const QRect &srcRect) override {
      QPainter p(&dst);
      p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
      p.drawImage(dstRect, src, srcRect);
   }
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      dst.fill(Qt::transparent);
   }
   void end() override {
      QPainter p(&dst);
      p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
      p.fillRect(dst.rect(), Qt::black);
   }
};

class ZBufPainter : public ImagePainter {
   ZBuffer<quint8> zbuf{dst.width(), dst.height()};
   int z;
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      z = 0;
      zbuf.clear();
      dst.fill(Qt::black);
   }
   void draw(const QPoint &, const QRect &dstRect, const QRect &srcRect) override {
      auto *sp = scanLine(src, srcRect.topLeft());
      auto *dp = scanLine(dst, dstRect.topLeft());
      auto *zp = zbuf.scanLine(dstRect.topLeft());
      const int sStep = lineStep(src, srcRect.width());
      const int dStep = lineStep(dst, dstRect.width());
      const int zStep = zbuf.lineStep(dstRect.width());
      for (int i = dstRect.height(); i; i--) {
         for (int j = dstRect.width(); j; j--) {
            if (*zp > z) {
               *zp = z;
               *dp = *sp;
            }
            sp++; zp++; dp++; 
         }
         sp += sStep; dp += dStep; zp += zStep;
      }
      ++z;
   }
};

struct Span {
   int x0 = 0, x1 = 0;
   Span() = default;
   Span(int x0, int x1) : x0(x0), x1(x1) {}
   constexpr bool isEmpty() const { return x1 == x0; }
   constexpr int size() const { return x1 - x0; }
   bool operator<(const Span &o) const { return x1 <= o.x0; }
};

struct SpanDiffInter {
   Span inter;    // a intersection b
   Span diff[2];  // a difference b
   bool after;    // a after b
   explicit SpanDiffInter(const Span &a, const Span &b) {
      Q_ASSERT(!(a.x1 <= b.x0));      // aa  bb  a1<=b0
      if ((after = (b.x1 <= a.x0))) { // bb  aa  a0>=b1
         diff[0] = a;
      } else if (a.x0 < b.x0) {
         if (a.x1 <= b.x1) {          // aa##bb  a0<b0,  a1<b1
            diff[0] = {a.x0, b.x0};   // aa##    a0<b0,  a1==b1
            inter   = {b.x0, a.x1};
         } else {                     // aa##aa  a0<b0,  a1>b1
            diff[0] = {a.x0, b.x0};
            inter   = {b.x0, b.x1};
            diff[1] = {b.x1, a.x1};
         }
      } else {
         if (a.x1 > b.x1) {
            inter   = {a.x0, b.x1};   //   ##aa  a0==b0, a1>b1
            diff[0] = {b.x1, a.x1};   // bb##aa  a0>b0,  a1>b1
         } else {
            inter   = {a.x0, a.x1};   //   ##bb  a0==b0, a1<b1
         }                            //   ##    a0==b0, a1==b1
      }                               // bb##bb  a0>b0,  a1<b1
   }                                  // bb##    a0>b0,  a1==b1
};

class FreeSpanDraw : public ImagePainter {
   std::vector<std::vector<Span>> Spans;

   void DrawPart(int y, const Span &s, const QPoint &dPos)
   {
      auto *dp = scanLine(dst, {s.x0, y});
      auto *sp = scanLine(src, QPoint{s.x0, y}-dPos);
      std::copy(sp, sp+s.size(), dp);
   }
   void DrawSegment(int y, const Span &d, const QPoint &dp)
   {
      auto &spans = Spans[y];
      for (auto is = std::lower_bound(spans.begin(), spans.end(), d); is != spans.end(); )
      {
         SpanDiffInter const ss{*is, d};
         if (ss.after) break;
         if (!ss.inter.isEmpty())
            DrawPart(y, ss.inter, dp);
         if (ss.diff[0].isEmpty())
            is = spans.erase(is);
         else {
            *is++ = ss.diff[0];
            if (!ss.diff[1].isEmpty()) {
               is = spans.insert(is, ss.diff[1]);
               is++;
            }
         }
      }
   }
   void draw(const QPoint &p, const QRect &dr, const QRect &) override {
      for (int y = dr.y(); y < dr.y() + dr.height(); ++y)
         DrawSegment(y, {dr.x(), dr.x() + dr.width()}, p);
   }
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      Spans.resize(dst.height());
      for (auto &spans : Spans) {
         spans.resize(1);
         spans[0] = {0, dst.width()};
      }
   }
   void end() override {
      for (int i=dst.height()-1; i>=0; i--)
         for (auto  &s : qAsConst(Spans)[i])
            std::fill_n(scanLine(dst, {s.x0, i}), s.size(), qRgb(0,0,0));
   }
};

QImage makeOverlay() {
   QStaticText text("1=Painter<br>2=Z-Buf<br>3=FS-Buf<br>Space=Pause");
   text.setTextFormat(Qt::RichText);
   QImage ret(text.size().toSize(), QImage::Format_ARGB32_Premultiplied);
   ret.fill(Qt::transparent);
   QPainter p(&ret);
   p.setPen(Qt::white);
   p.drawStaticText(0, 0, text);
   return ret;
}

class Display : public QRasterWindow {
   Q_OBJECT
   QImage img, overlay;
protected:
   void paintEvent(QPaintEvent *) override {
      QPainter p(this);
      p.setCompositionMode(QPainter::CompositionMode_Source);
      p.drawImage(0, 0, img);
      p.setCompositionMode(QPainter::CompositionMode_SourceOver);
      p.drawImage(2, 2, overlay);
   }
   void keyPressEvent(QKeyEvent *k) override {
      emit hasKey(k->key());
   }
public:
   void setImage(const QImage &input) {
      img = input.scaled(input.size() * 2);
      resize(size().expandedTo(img.size()));
      update();
   }
   void setOverlay(const QImage &ovly) {
      overlay = ovly;
      resize(size().expandedTo(overlay.size()));
      update();
   }
   Q_SIGNAL void hasKey(int);
};

inline void bounce(qreal &x, qreal &v, qreal const left, qreal const right) {
   qreal out;
   if ((out = (x-left)) < 0 || (out = (x-right)) > 0) {
      int n = out / (right - left);
      x -= n * (right - left);
      if (!(abs(n) % 2)) v = -v;
   }
   if ((out = (x-left)) < 0 || (out = (x-right)) > 0) {
      x -= out*2;
   }
}

struct State {
   QPointF pos, vel;
   void advance(qreal t, const QRectF &rect) {
      pos += vel * t;
      bounce(pos.rx(), vel.rx(), rect.x(), rect.x() + rect.width());
      bounce(pos.ry(), vel.ry(), rect.y(), rect.y() + rect.height());
   }
};

class Demo : public QObject {
   Q_OBJECT
   QImage dst{320, 200, QImage::Format_ARGB32_Premultiplied};
   const QImage image;
   const QImage borderImage = WithBorder(image, 3, Qt::red);
   DrawPainter draw{image, dst};
   ZBufPainter zbuf{borderImage, dst};
   FreeSpanDraw span{borderImage, dst};
   const std::array<ImagePainter*, 3> painters{&draw, &zbuf, &span};
   ImagePainter *painter = painters.front();
   QVector<State> state{10};
   QBasicTimer timer;
   QElapsedTimer el;

   void timerEvent(QTimerEvent *ev) override {
      if (ev->timerId() == timer.timerId() && painter) {
         qreal const t = el.restart() / (qreal)10;
         for (auto &s : state)
            s.advance(t, dst.rect());
         update();
      }
   }
   void update() {
      painter->begin();
      for (auto &s : state)
         painter->draw(s.pos.toPoint());
      painter->end();
      emit hasImage(dst);
   }
public:
   Demo(const QImage &img, QObject *parent = {}) : QObject(parent), image(img)
   {
      for (auto &s : state) {
         s.pos = QPointF(rand()%dst.width(), rand()%dst.height());
         s.vel = {(rand()%1024-512)/256.0, (rand()%1024-512)/256.0};
      }
      toggleRunning();
   }
   void onKey(int key) {
      int m = key - Qt::Key_0;
      if (m >= 0 && m < 10)
         setMethod(m);
      else if (key == Qt::Key_Space)
         toggleRunning();
   }
   void setMethod(int m) {
      if (m >= 1 && m <= painters.size())
         painter = painters[m-1];
      update();
   }
   void toggleRunning() {
      if (timer.isActive())
         timer.stop();
      else {
         timer.start(10, this);
         el.start();
      }
   }
   Q_SIGNAL void hasImage(const QImage &);
};

int main(int argc, char *argv[])
{
   QGuiApplication app(argc, argv);
   QImage src("../sbdemo/monkey.bmp");
   if (src.isNull())
      qFatal("Cannot load monkey.bmp.");

   Demo demo(std::move(src).convertToFormat(QImage::Format_ARGB32_Premultiplied));
   Display disp;

   QObject::connect(&disp, &Display::hasKey, &demo, &Demo::onKey);
   QObject::connect(&demo, &Demo::hasImage, &disp, &Display::setImage);
   disp.setOverlay(makeOverlay());
   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
