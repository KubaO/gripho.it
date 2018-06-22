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

//
// Video address and offscreen buffer
//

QImage Image;
QImage buf{320, 200, QImage::Format_ARGB32_Premultiplied};

constexpr QRgb white = qRgb(255, 255, 225);
constexpr QRgb black = qRgb(0, 0, 0);

//
// Read a pixel from the image adding a 2 pixel border
// (just to slow down the pixel computation)
//
QRgb GetPixel(int x, int y)
{
   if (x<2 || x>Image.width()-3 || y<2 || y>Image.height()-3)
      return Qt::black;
   return Image.pixel(x, y);
}

//
// Simple draw (painter)
//
void DrawImage(int xa,int ya)
{
   QPainter p(&buf);
   p.drawImage(xa, ya, Image);
}

//
// Z-Buffer draw 
//

unsigned char ZBuf[320*200];

void DrawZBImage(int xa,int ya,int z)
{
   auto const size = Image.size();
   int y,x0,y0,x1,y1;
   x0=xa; y0=ya; x1=x0+size.width(); y1=y0+size.height();
   if (x0<0) x0=0;
   if (y0<0) y0=0;
   if (x1>320) x1=320;
   if (y1>200) y1=200;
   if (x0<x1 && y0<y1)
   {
      auto *wp = ((QRgb*)buf.scanLine(y0)) + x0;
      auto *zp = ZBuf+y0*320+x0;
      y=y0-ya;
      while (y0<y1)
      {
         auto *wp1=wp;
         auto *zp1=zp;
         int c=x1-x0;
         int x=x0-xa;
         do {
            if (*zp1>z)
            {
               *zp1=z; *wp1=GetPixel(x,y);
            }
            wp1++; zp1++; x++;
         } while (--c);
         wp+=320; y++; zp+=320;
         y0++;
      }
   }
}

//
// Free Span Buffer draw
//

struct Span
{
   int x0;
   int x1;
   Span *next;
};

#define MAXYRES 1024

Span *FirstSpan[MAXYRES];
Span *FirstUnusedSpan;

Span *NewSpan()
{
   Span *s = FirstUnusedSpan;
   if (!s)
      s = new Span;
   else
      FirstUnusedSpan = s->next;
   return s;
}

void FreeSpan(Span *s)
{
   s->next=FirstUnusedSpan;
   FirstUnusedSpan=s;
}

int sbxa,sbya;

void DrawPart(int y, int x0, int x1)
{
   QRgb *wp = ((QRgb*)buf.scanLine(y)) + x0;
   x1 -= x0;
   int ix = x0-sbxa;
   int iy = y-sbya;
   do
   {
      *wp++ = GetPixel(ix++,iy);
   } while (--x1);
}

void DrawSegment(int y,int xa,int xb)
{
   for (Span *old=NULL, *current=FirstSpan[y];
        current!=NULL;
        old=current, current=current->next)
   {
      if (current->x1<=xa) // Case 1
         continue;

      if (current->x0<xa)
      {
         if (current->x1<=xb) // Case 2
         {
            DrawPart(y,xa,current->x1);
            current->x1=xa;
         }
         else // Case 3
         {
            DrawPart(y,xa,xb);
            Span *n=NewSpan();
            n->next=current->next;
            current->next=n;
            n->x1=current->x1;
            current->x1=xa; n->x0=xb;
            return;
         }
      }
      else
      {
         if (current->x0>=xb) // Case 6
            return;

         if (current->x1<=xb) // Case 4
         {
            DrawPart(y,current->x0,current->x1);
            Span *n=current->next; FreeSpan(current);
            if (old) old->next=n;
            else FirstSpan[y]=n;
            current=n;
            if (current==NULL) return;
         }
         else // Case 5
         {
            DrawPart(y,current->x0,xb);
            current->x0=xb;
         }
      }
   }
}

void DrawFSBImage(int xa,int ya)
{
   int x0,y0,x1,y1;
   sbxa=xa; sbya=ya;
   x0=xa; y0=ya; x1=x0+Image.width(); y1=y0+Image.height();
   if (x0<0) x0=0;
   if (y0<0) y0=0;
   if (x1>320) x1=320;
   if (y1>200) y1=200;
   if (x0<x1 && y0<y1)
   {
      while (y0<y1)
      {
         DrawSegment(y0,x0,x1);
         y0++;
      }
   }
}


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

class Display : public QRasterWindow {
   Q_OBJECT
   QImage img{320*2, 200*2, QImage::Format_ARGB32_Premultiplied};
protected:
   void paintEvent(QPaintEvent *) override {
      QPainter p(this);
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
   Image.load("../sbdemo/monkey.bmp");
   if (Image.isNull())
      qFatal("Cannot load monkey.bmp.");

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
         buf.fill(Qt::black);
         for (int i=NPICS-1; i>=0; i--)
         {
            DrawImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs);
         }
         break;
      case 1:
         memset(ZBuf, 255, buf.width() * buf.height());
         buf.fill(Qt::black);
         for (int i=0; i<NPICS; i++)
         {
            DrawZBImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs,i);
         }
         break;
      case 2:
         for (int i=0; i<200; i++)
         {
            Span *s;
            while ((s=FirstSpan[i])!=NULL)
            {
               FirstSpan[i]=s->next;
               FreeSpan(s);
            }
            s=NewSpan();
            s->next=NULL; FirstSpan[i]=s;
            s->x0=0; s->x1=320;
         }
         for (int i=0; i<NPICS; i++)
         {
            DrawFSBImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs);
         }
         for (int i=0; i<200; i++)
         {
            Span *s;
            for (s=FirstSpan[i]; s!=NULL; s=s->next)
            {
               //memset(Video+i*320+s->x0,black,s->x1-s->x0);
               memset(buf.scanLine(i)+s->x0, black, s->x1-s->x0);
            }
         }
         break;
      }
      for (int i=0; Info[i][0]!='\0'; i++)
         for (int j=0; Info[i][j]!='\0'; j++)
            if (Info[i][j]=='x')
               buf.setPixel(j+1, 4+i, Qt::white);
      disp.setImage(buf);
   });

   disp.show();
   return app.exec();
}

#include "sbuffer.moc"
