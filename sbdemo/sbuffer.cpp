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
public:
   ImagePainter(const QImage &src, QImage &dst) : src(src), dst(dst) {}
   virtual void begin() {}
   virtual void draw(const QPoint p) = 0;
   virtual void end() {}
   virtual ~ImagePainter() {}
};

class DrawPainter : public ImagePainter {
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      dst.fill(Qt::transparent);
   }
   void draw(const QPoint pt) override {
      QPainter p(&dst);
      p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
      p.drawImage(pt, src);
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
   void draw(const QPoint p) override {
      const QRect dstRect = src.rect().adjusted(p.x(), p.y(), p.x(), p.y()).intersected(dst.rect());
      if (dstRect.isEmpty())
         return;

      QRect srcRect = dstRect.adjusted(-p.x(), -p.y(), -p.x(), -p.y());
      srcRect = srcRect.normalized();

      auto *dp = scanLine(dst, dstRect.topLeft());
      auto *zp = zbuf.scanLine(dstRect.topLeft());
      int const dStep = lineStep(dst, srcRect.width());
      int const zStep = zbuf.lineStep(srcRect.width());
      for (int y = srcRect.top(); y < srcRect.bottom()+1; ++y) {
         for (int x = srcRect.left(); x < srcRect.right()+1; ++x) {
            if (*zp > z) {
               *zp = z;
               *dp = src.pixel(x, y);
            }
            dp ++; zp++;
         }
         dp += dStep; zp += zStep;
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
   int sbxa, sbya;

   void DrawPart(int y, int x0, int x1)
   {
      auto *wp = scanLine(dst, {x0, y});
      auto *rp = scanLine(src, {x0-sbxa, y-sbya});
      std::copy(rp, rp+(x1-x0), wp);
   }
   void DrawSegment(int y, int xa, int xb)
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
               DrawPart(y, xa, is->x1);
               is->x1 = xa;
            }
            else // Case 3
            {
               DrawPart(y, xa, xb);
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
               DrawPart(y, is->x0, is->x1);
               is = spans.erase(is, std::next(is));
               if (is == spans.end())
                  return;
            }
            else // Case 5
            {
               DrawPart(y, is->x0, xb);
               is->x0 = xb;
            }
         }
      }
   }
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      for (auto &spans : Spans) {
         spans.resize(1);
         spans[0] = {0, dst.width()};
      }
   }
   void draw(const QPoint p) override {
      int x0,y0,x1,y1;
      sbxa=p.x(); sbya=p.y();
      x0=p.x(); y0=p.y(); x1=x0+src.width(); y1=y0+src.height();
      if (x0<0) x0=0;
      if (y0<0) y0=0;
      if (x1>dst.width()) x1=dst.width();
      if (y1>dst.height()) y1=dst.height();
      if (x0<x1)
         while (y0<y1)
         {
            DrawSegment(y0, x0, x1);
            y0++;
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
   QStaticText text;
   text.setTextFormat(Qt::RichText);
   text.setText("1=Painter<br>2=Z-Buf<br>3=FS-Buf");
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
   QPoint overlayPos;
protected:
   void paintEvent(QPaintEvent *) override {
      QPainter p(this);
      p.fillRect(QRect({}, size()), Qt::black);
      p.drawImage(0, 0, img);
      p.drawImage(overlayPos, overlay);
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
   void setOverlay(const QImage &ovly, const QPoint p) {
      overlay = ovly;
      overlayPos = p;
      resize(size().expandedTo(overlay.size()));
      update();
   }
   Q_SIGNAL void reqMethod(int);
};

void bounce(qreal &x, qreal &v, qreal const left, qreal const right) {
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
      bounce(pos.rx(), vel.rx(), rect.left(), rect.right());
      bounce(pos.ry(), vel.ry(), rect.top(), rect.bottom());
   }
};

int main(int argc, char *argv[])
{
   QGuiApplication app(argc, argv);
   QImage Src("../sbdemo/monkey.bmp");
   if (Src.isNull())
      qFatal("Cannot load monkey.bmp.");
   QImage const Image = std::move(Src).convertToFormat(QImage::Format_ARGB32_Premultiplied);
   QImage const BImage = WithBorder(Image, 3, Qt::red);

   QImage dst{320, 200, QImage::Format_ARGB32_Premultiplied};
   DrawPainter draw(Image, dst);
   ZBufPainter zbuf(BImage, dst);
   FreeSpanDraw span(BImage, dst);
   std::array<ImagePainter*, 3> painters{&draw, &zbuf, &span};
   Display disp;

   State state[10];

   auto const offset = Image.rect().center();
   for (auto &s : state) {
      s.pos = QPointF(rand()%dst.width(), rand()%dst.height()) - offset;
      s.vel = {(rand()%1024-512)/256.0, (rand()%1024-512)/256.0};
   }
   const QRectF posRect = dst.rect().adjusted(-offset.x(), -offset.y(), -offset.x(), -offset.y());

   int method=0;
   QObject::connect(&disp, &Display::reqMethod, [&](size_t m){
      if (m >= 1 && m <= painters.size()) method = m-1;
   });

   QTimer timer;
   timer.setInterval(10);
   timer.start();
   QElapsedTimer el;
   el.start();
   QObject::connect(&timer, &QTimer::timeout, [&]{
      qreal const t = el.restart() / (qreal)10;
      for (auto &s : state)
         s.advance(t, posRect);

      auto &painter = *painters[method];
      painter.begin();
      for (auto &s : state)
         painter.draw(s.pos.toPoint());
      painter.end();
      disp.setImage(dst);
   });

   disp.setOverlay(makeOverlay(), {2, 2});
   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
