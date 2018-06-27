/*
** This program is an example of the "Free Span Buffer" algorithm. For more
** infos check
**
**                http://www.netservers.com/~agriff
**
** or mailto:agriff@tin.it 
*/

#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//
// Video address and offscreen buffer
//
unsigned char *Video = (unsigned char *)0xA0000;
unsigned char buf[320*200];

//
// BMP file loader
//
typedef struct BMPHEADER
  { char  bfType[2];       /* File type -> "BM" */
    int   bfSize;          /* File size */
    int   bfResvd;
    int   bfOffBits;       /* Offset of bits in file */

    int   biSize;          /* Size of info header = 40 bytes */
    int   biWidth;         /* Bitmap width */
    int   biHeight;        /* Bitmap height */
    short biPlanes;        /* Number of planes -> 1 */
    short biBitCount;      /* Bits per pixel -> 8 */
    int   biCompression;   /* Compressione method -> 0=uncompressed */
    int   biSizeImage;     /* Compressed image size -> 0=uncompressed */
    int   biXPelsPerMeter; /* Horizontal resolution */
    int   biYPelsPerMeter; /* Vertical resolution */
    int   biClrUsed;       /* Used colors count -> 0=use biBitCount */
    int   biImportant;     /* Number of "important" colors -> 0=all */
  } BMPHeader;

int ImgWidth,ImgHeight;
unsigned char *Image;
int white,black;

int LoadTexture(char *Name)
{
  FILE *f;
  BMPHeader h;
  int whitel,blackl,i,j,y,x,cols;
  int RGB[256];

  if ((f=fopen(Name,"rb"))==NULL)
    return 0;
  if (fread(&h,1,sizeof(h),f)!=sizeof(h) ||
      h.bfType[0]!='B' || h.bfType[1]!='M' ||
      h.biSize!=40 ||
      h.biPlanes!=1 || h.biBitCount!=8 || h.biCompression!=0)
  {
    fclose(f); return 0;
  }

  ImgWidth=h.biWidth; ImgHeight=h.biHeight;

  cols=(h.biClrUsed==0 ? 256 : h.biClrUsed);
  if (fread(RGB,1,cols*4,f)!=(unsigned)cols*4)
  {
    fclose(f); return 0;
  }
  Image=(unsigned char *)malloc(ImgWidth*ImgHeight+4);

  //
  // Define the palette
  //
  white=black=whitel=blackl=-1;
  for (i=0; i<cols; i++)
  {
    int r,g,b;
    j=RGB[i];
    r=(j>>16)&255;
    g=(j>>8)&255;
    b=j&255;
    j=r+2*g+b;
    if (white==-1 || whitel<j) { white=i; whitel=j; }
    if (black==-1 || blackl>j) { black=i; blackl=j; }
    outp(0x3C8,i);
    outp(0x3C9,r*63/255);
    outp(0x3C9,g*63/255);
    outp(0x3C9,b*63/255);
  }

  //
  // Load actual data
  //
  fseek(f,h.bfOffBits,SEEK_SET);
  x=((ImgWidth+3)&~3);
  for (y=0; y<ImgHeight; y++)
  {
    if (fread(Image+ImgWidth*(ImgHeight-1-y),1,x,f)!=x)
    {
      fclose(f);
      free(Image);
      return 0;
    }
  }
  fclose(f);
  return 1;
}

//
// Read a pixel from the image adding a 2 pixel border
// (just to slow down the pixel computation)
//
int GetPixel(int x,int y)
{
  if (x<2 || x>ImgWidth-3 || y<2 || y>ImgHeight-3)
    return black;
  return Image[y*ImgWidth+x];
}

//
// Simple draw (painter)
//
void DrawImage(int xa,int ya)
{
  unsigned char *wp,*wp1;
  int x,y,x0,y0,x1,y1,c;
  x0=xa; y0=ya; x1=x0+ImgWidth; y1=y0+ImgHeight;
  if (x0<0) x0=0;
  if (y0<0) y0=0;
  if (x1>320) x1=320;
  if (y1>200) y1=200;
  if (x0<x1 && y0<y1)
  {
    wp=buf+y0*320+x0;
    y=y0-ya;
    while (y0<y1)
    {
      x=x0-xa;
      wp1=wp; c=x1-x0;
      do
      {
        *wp1++=GetPixel(x++,y);
      } while (--c);
      wp+=320; y++;
      y0++;
    }
  }
}


//
// Z-Buffer draw 
//

unsigned char ZBuf[320*200];

void DrawZBImage(int xa,int ya,int z)
{
  unsigned char *wp,*zp,*wp1,*zp1;
  int x,y,x0,y0,x1,y1,c;
  x0=xa; y0=ya; x1=x0+ImgWidth; y1=y0+ImgHeight;
  if (x0<0) x0=0;
  if (y0<0) y0=0;
  if (x1>320) x1=320;
  if (y1>200) y1=200;
  if (x0<x1 && y0<y1)
  {
    wp=buf+y0*320+x0;
    zp=ZBuf+y0*320+x0;
    y=y0-ya;
    while (y0<y1)
    {
      wp1=wp; zp1=zp; c=x1-x0; x=x0-xa;
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

typedef struct SPAN
{
  int x0;
  int x1;
  struct SPAN *next;
} Span;

#define MAXYRES 1024

Span *FirstSpan[MAXYRES];
Span *FirstUnusedSpan;

void *Malloc(int size)
{
  void *p;
  if ((p=malloc(size))==NULL)
  {
    printf("Out of memory!");
    exit(1);
  }
  return p;
}

Span *NewSpan(void)
{
  Span *s;
  if ((s=FirstUnusedSpan)==NULL)
    s=(Span *)Malloc(sizeof(Span));
  else
    FirstUnusedSpan=s->next;
  return s;
}

void FreeSpan(Span *s)
{
  s->next=FirstUnusedSpan;
  FirstUnusedSpan=s;
}

int sbxa,sbya;

void DrawPart(int y,int x0,int x1)
{
  int iy,ix;
  unsigned char *wp;
  wp=buf+y*320+x0;
  x1-=x0;
  ix=x0-sbxa;
  iy=y-sbya;
  do
  {
    *wp++=GetPixel(ix++,iy);
  } while (--x1);
}

void DrawSegment(int y,int xa,int xb)
{
  Span *old,*current,*n;
  for (old=NULL,current=FirstSpan[y];
       current!=NULL;
       old=current,current=current->next)
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
	n=NewSpan();
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
	n=current->next; FreeSpan(current);
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
  x0=xa; y0=ya; x1=x0+ImgWidth; y1=y0+ImgHeight;
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

#define NPICS 200
int x[NPICS],y[NPICS],xv[NPICS],yv[NPICS];

unsigned char *Info[]={
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

void main(void)
{
  union REGS r;
  int i,xoffs,yoffs,method;

  r.w.ax=0x13; int386(0x10,&r,&r);
  if (!LoadTexture("monkey.bmp"))
  {
    r.w.ax=0x03; int386(0x10,&r,&r);
    puts("Error opening monkey.bmp");
    exit(1);
  }
  
  for (i=0; i<NPICS; i++)
  {
    x[i]=(rand()%320)<<8; y[i]=(rand()%200)<<8;
    xv[i]=rand()%1024-512; yv[i]=rand()%1024-512;
  }
  
  xoffs=ImgWidth/2;
  yoffs=ImgHeight/2;
  method=0;
  while (method!=-1)
  {
    for (i=0; i<NPICS; i++)
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
        memset(buf,black,sizeof(buf));
	for (i=NPICS-1; i>=0; i--)
	{
	  DrawImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs);
	}
	break;
      case 1:
        memset(ZBuf,255,sizeof(buf));
        memset(buf,black,sizeof(buf));
	for (i=0; i<NPICS; i++)
	{
	  DrawZBImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs,i);
	}
	break;
      case 2:
        for (i=0; i<200; i++)
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
	for (i=0; i<NPICS; i++)
	{
	  DrawFSBImage((x[i]>>8)-xoffs,(y[i]>>8)-yoffs);
	}
	for (i=0; i<200; i++)
	{
	  Span *s;
	  for (s=FirstSpan[i]; s!=NULL; s=s->next)
	  {
	    memset(Video+i*320+s->x0,black,s->x1-s->x0);
	  }
	}
	break;
    }
    for (i=0; Info[i][0]!='\0'; i++)
    {
      int j;
      for (j=0; Info[i][j]!='\0'; j++)
      {
        if (Info[i][j]=='x')
	{
	  buf[i*320+j+320*4+4]=white;
	}
      }
    }
    memcpy(Video,buf,sizeof(buf));
    if (kbhit())
    {
      if ((i=getch())==0) i=-getch();
      switch(i)
      {
        case '0' : method=0; break;
	case '1' : method=1; break;
	case '2' : method=2; break;
	case 27  : method=-1; break;
      }
    }
  }
  r.w.ax=0x03; int386(0x10,&r,&r);
}
