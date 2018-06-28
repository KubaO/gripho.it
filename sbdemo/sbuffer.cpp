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
   QVector<T> m_buf;
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
      m_buf.fill(defaultZ());
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

class FreeSpanDraw : public ImagePainter {
   struct Span
   {
      int x0;
      int x1;
   };
   QVector<QVector<Span>> Spans = QVector<QVector<Span>>(dst.height());


   void DrawPart(int y, int x0, int x1, const QPoint &dPos)
   {
      auto *dp = scanLine(dst, {x0, y});
      auto *sp = scanLine(src, QPoint{x0, y}-dPos);
      std::copy(sp, sp+(x1-x0), dp);
   }
   void DrawSegment(int y, int xa, int xb, const QPoint &dp)
   {
      auto &spans = Spans[y];
      for (auto is = spans.begin(); is != spans.end(); is++)
      {
         if (is->x1 <= xa) // Case 1
            continue;

         if (is->x0 < xa)
         {
            if (is->x1 <= xb) // Case 2
            {
               DrawPart(y, xa, is->x1, dp);
               is->x1 = xa;
            }
            else // Case 3
            {
               DrawPart(y, xa, xb, dp);
               std::swap(xa, is->x1);
               spans.insert(++is, {xb, xa});
               return;
            }
         }
         else
         {
            if (is->x0 >= xb) // Case 6
               return;

            if (is->x1 <= xb) // Case 4
            {
               DrawPart(y, is->x0, is->x1, dp);
               is = spans.erase(is, std::next(is));
               if (is == spans.end())
                  return;
            }
            else // Case 5
            {
               DrawPart(y, is->x0, xb, dp);
               is->x0 = xb;
            }
         }
      }
   }
   void draw(const QPoint &p, const QRect &dr, const QRect &) override {
      for (int y = dr.y(); y < dr.y() + dr.height(); ++y)
         DrawSegment(y, dr.x(), dr.x() + dr.width(), p);
   }
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      for (auto &spans : Spans) {
         spans.resize(1);
         spans[0] = {0, dst.width()};
      }
   }
   void end() override {
      constexpr QRgb black = qRgb(0, 0, 0);
      for (int i=dst.height()-1; i>=0; i--)
         for (auto &s : qAsConst(Spans[i]))
            std::fill_n(scanLine(dst, {s.x0, i}), s.x1-s.x0, black);
   }
};

QImage makeOverlay() {
   QStaticText text("1=Painter<br>2=Z-Buf<br>3=FS-Buf");
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
      int m = k->key() - Qt::Key_0;
      if (m >= 0 && m < 10)
         emit reqMethod(m);
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
   Q_SIGNAL void reqMethod(int);
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
   QVector<State> state{100};
   QBasicTimer timer;
   QElapsedTimer el;

   void timerEvent(QTimerEvent *ev) override {
      if (ev->timerId() == timer.timerId() && painter)
         tick();
   }
   void tick() {
      qreal const t = el.restart() / (qreal)10;
      for (auto &s : state)
         s.advance(t, dst.rect());

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
      timer.start(10, this);
      el.start();
   }
   void setMethod(int m) {
      if (m >= 1 && m <= painters.size())
         painter = painters[m-1];
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

   QObject::connect(&disp, &Display::reqMethod, &demo, &Demo::setMethod);
   QObject::connect(&demo, &Demo::hasImage, &disp, &Display::setImage);
   disp.setOverlay(makeOverlay());
   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
