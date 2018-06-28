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
   virtual void draw(const QPoint p, int n) = 0;
   virtual void end() {}
   virtual ~ImagePainter() {}
};

class DrawPainter : public ImagePainter {
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      dst.fill(Qt::transparent);
   }
   void draw(const QPoint pt, int) override {
      QPainter p(&dst);
      p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
      p.drawImage(pt, src);
   }
};

class ZBufPainter : public ImagePainter {
   ZBuffer<quint8> zbuf{dst.width(), dst.height()};
public:
   using ImagePainter::ImagePainter;
   void begin() override {
      zbuf.clear();
      dst.fill(Qt::black);
   }
   void draw(const QPoint p, int z) override {
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
   }
   void clear() {
      zbuf.clear();
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
   void draw(const QPoint p, int) override {
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

const char *Info[]={
   "  xxx        xxxx   xxx  xxx x   x xxxxx xxxx xxxx",
   " x  xx xxxx  x   x x   x  x  xx  x   x   x    x   x",
   " x x x       xxxx  xxxxx  x  x x x   x   xxx  xxxx",
   " xx  x xxxx  x     x   x  x  x  xx   x   x    x   x",
   "  xxx        x     x   x xxx x   x   x   xxxx x   x",
   " ",
   "   x         xxxxx     xxxx  x   x xxxx",
   "  xx   xxxx     x      x   x x   x x",
   "   x           x   xxx xxxx  x   x xxx",
   "   x   xxxx   x        x   x x   x x",
   "  xxx        xxxxx     xxxx   xxx  x",
   " ",
   " xxxx        xxxxx  xxxx     xxxx  x   x xxxx",
   "     x xxxx  x     x         x   x x   x x",
   "  xxx        xxx    xxx  xxx xxxx  x   x xxx",
   " x     xxxx  x         x     x   x x   x x",
   " xxxxx       x     xxxx      xxxx   xxx  x",
   ""
};

void DrawAsciiArt(QImage &dst, const char *info[], int x, int y) {
   for (int i=0; info[i][0]!='\0'; i++) {
      auto *dp = scanLine(dst, {x, y+i});
      for (int j=0; info[i][j]!='\0'; j++, dp++)
         if (info[i][j]=='x')
            *dp = qRgba(255, 255, 255, 255);
   }
}

class Display : public QRasterWindow {
   Q_OBJECT
   QImage img{320*2, 200*2, QImage::Format_ARGB32_Premultiplied};
protected:
   void paintEvent(QPaintEvent *) override {
      QPainter p(this);
      p.fillRect(QRect({}, size()), Qt::black);
      p.drawImage(0, 0, img);
   }
   void keyPressEvent(QKeyEvent *k) override {
      switch (k->key()) {
      case Qt::Key_0: return reqMethod(0);
      case Qt::Key_1: return reqMethod(1);
      case Qt::Key_2: return reqMethod(2);
      }
   }
public:
   Display() {
      resize(img.size());
   }
   void setImage(const QImage &input) {
      img = input.scaled(img.size());
      update();
   }
   Q_SIGNAL void reqMethod(int);
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

   constexpr int NPICS = 10;
   QPointF pos[NPICS], vel[NPICS];

   auto const offset = Image.rect().center();
   for (auto &p : pos)
      p = QPointF(rand()%dst.width(), rand()%dst.height()) - offset;
   for (auto &v : vel)
      v = {(rand()%1024-512)/256.0, (rand()%1024-512)/256.0};
   const QRectF posRect = dst.rect().adjusted(-offset.x(), -offset.y(), -offset.x(), -offset.y());

   int method=0;
   QObject::connect(&disp, &Display::reqMethod, [&](int m){ method = m; });

   QTimer t;
   t.setInterval(10);
   t.start();
   QObject::connect(&t, &QTimer::timeout, [&]{
      for (int i=0; i<NPICS; i++)
      {
         auto &p = pos[i], &v = vel[i];
         p += v;
         if (!posRect.contains(p.x(), 0))
            v.setX(-v.x());
         if (!posRect.contains(0, p.y()))
            v.setY(-v.y());
      }

      auto &painter = *painters[method];
      painter.begin();
      for (int i=0; i<NPICS; i++)
         painter.draw(pos[i].toPoint(), i);
      painter.end();
      DrawAsciiArt(dst, Info, 1, 4);
      disp.setImage(dst);
   });

   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
