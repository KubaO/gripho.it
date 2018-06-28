/*
** This program is an example of the "Free Span Buffer" algorithm. For more
** infos check
**
**                http://www.netservers.com/~agriff
**
** or mailto:agriff@tin.it 
*/
#include <QtGui>
#include <cmath>
#include <cstdint>
#include <cstring>

int lineStep(const QImage &img, int subWidth) {
   Q_ASSERT((img.bytesPerLine() * 8) % img.depth() == 0);
   return (img.bytesPerLine() * 8) / img.depth() - subWidth;
}

template <typename T = QRgb>
const T *imgScanLine (const QImage &dst, const QPoint pos) {
   return reinterpret_cast<const T*>(dst.scanLine(pos.y()) + pos.x()*sizeof(T));
}

template <typename T = QRgb>
T *imgScanLine (QImage &dst, const QPoint pos) {
   return const_cast<T*>(imgScanLine(const_cast<const QImage &>(dst), pos));
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

QImage WithBorder(QImage img, int width) {
   QPainter p(&img);
   QPen pen(Qt::black);
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
   virtual ~ImagePainter() {}
};

class DrawPainter : public ImagePainter {
public:
   using ImagePainter::ImagePainter;
   void draw(int x, int y) {
      QPainter p(&dst);
      p.drawImage(x, y, src);
   }
};

class ZBufPainter : public ImagePainter {
   ZBuffer<quint8> zbuf{dst.width(), dst.height()};
public:
   using ImagePainter::ImagePainter;
   void draw(int x, int y, int z) {
      QRect dstRect = src.rect().adjusted(x, y, x, y);
      dstRect = dstRect.intersected(dst.rect());
      if (dstRect.isEmpty())
         return;

      QRect srcRect = dstRect.adjusted(-x, -y, -x, -y);
      srcRect = srcRect.normalized();

      auto *dp = imgScanLine(dst, dstRect.topLeft());
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
      Span *next;
   };
   QVector<Span*> FirstSpan{dst.height()};
   Span* FirstUnusedSpan = {};
   int sbxa, sbya;

   Span *NewSpan(int x0, int x1, Span *next)
   {
      Span *s = FirstUnusedSpan;
      if (!s)
         s = new Span{x0, x1, next};
      else {
         FirstUnusedSpan = s->next;
         s->x0 = x0;
         s->x1 = x1;
         s->next = next;
      }
      return s;
   }
   void FreeSpan(Span *s)
   {
      s->next = FirstUnusedSpan;
      FirstUnusedSpan = s;
   }
   void FreeSpans(Span *&s0)
   {
      while (Span *s = s0)
      {
         s0 = s->next;
         FreeSpan(s);
      }

   }
   void DrawPart(int y, int x0, int x1)
   {
      auto *wp = imgScanLine(dst, {x0, y});
      auto *rp = imgScanLine(src, {x0-sbxa, y-sbya});
      memcpy(wp, rp, (x1-x0)*sizeof(QRgb));
   }
   void DrawSegment(int y, int xa, int xb)
   {
      for (Span *old={}, *current=FirstSpan[y];
           current;
           old=current, current=current->next)
      {
         if (current->x1 <= xa) // Case 1
            continue;

         if (current->x0 < xa)
         {
            if (current->x1 <= xb) // Case 2
            {
               DrawPart(y, xa, current->x1);
               current->x1=xa;
            }
            else // Case 3
            {
               DrawPart(y, xa, xb);
               Span *n=NewSpan(xb, current->x1, current->next);
               current->next = n;
               current->x1 = xa;
               return;
            }
         }
         else
         {
            if (current->x0 >= xb) // Case 6
               return;

            if (current->x1 <= xb) // Case 4
            {
               DrawPart(y, current->x0, current->x1);
               Span *n=current->next; FreeSpan(current);
               if (old) old->next=n;
               else FirstSpan[y]=n;
               current=n;
               if (current==NULL) return;
            }
            else // Case 5
            {
               DrawPart(y, current->x0, xb);
               current->x0=xb;
            }
         }
      }
   }
public:
   using ImagePainter::ImagePainter;
   void draw(int xa, int ya) {
      int x0,y0,x1,y1;
      sbxa=xa; sbya=ya;
      x0=xa; y0=ya; x1=x0+src.width(); y1=y0+src.height();
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
   void Init() {
      for (auto &span : FirstSpan) {
         FreeSpans(span);
         span = NewSpan(0, dst.width(), nullptr);
      }
   }
   void End() {
      constexpr QRgb black = qRgb(0, 0, 0);
      for (int i=dst.height()-1; i>=0; i--)
         for (Span *s=FirstSpan[i]; s; s=s->next)
            std::fill_n(imgScanLine(dst, {s->x0, i}), s->x1-s->x0, black);
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
      auto *dp = imgScanLine(dst, {x, y+i});
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
   QImage const BImage = WithBorder(Image, 3);

   QImage dst{320, 200, QImage::Format_ARGB32_Premultiplied};
   DrawPainter draw(Image, dst);
   ZBufPainter zbuf(BImage, dst);
   FreeSpanDraw span(BImage, dst);
   Display disp;

   constexpr int NPICS = 200;
   int x[NPICS],y[NPICS],xv[NPICS],yv[NPICS];

   for (int i=0; i<NPICS; i++)
   {
      x[i]=(rand()%320)<<8; y[i]=(rand()%200)<<8;
      xv[i]=rand()%1024-512; yv[i]=rand()%1024-512;
   }

   int xoffs=Image.width()/2;
   int yoffs=Image.height()/2;
   int method=0;
   QObject::connect(&disp, &Display::reqMethod, [&](int m){ method = m; });

   QTimer t;
   t.setInterval(10);
   t.start();
   QObject::connect(&t, &QTimer::timeout, [&]{
      for (int i=0; i<NPICS; i++)
      {
         x[i]+=xv[i];
         if (x[i]<0 || x[i]>320<<8)
            xv[i]=-xv[i];
         y[i]+=yv[i];
         if (y[i]<0 || y[i]>200<<8)
            yv[i]=-yv[i];
      }

      switch(method)
      {
      case 0:
         dst.fill(Qt::black);
         for (int i=NPICS-1; i>=0; i--)
            draw.draw((x[i]>>8)-xoffs, (y[i]>>8)-yoffs);
         break;
      case 1:
         zbuf.clear();
         dst.fill(Qt::black);
         for (int i=0; i<NPICS; i++)
            zbuf.draw((x[i]>>8)-xoffs, (y[i]>>8)-yoffs, i);
         break;
      case 2:
         span.Init();
         for (int i=0; i<NPICS; i++)
            span.draw((x[i]>>8)-xoffs, (y[i]>>8)-yoffs);
         span.End();
         break;
      }
      DrawAsciiArt(dst, Info, 1, 4);
      disp.setImage(dst);
   });

   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
