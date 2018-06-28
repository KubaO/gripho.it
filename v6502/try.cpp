#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include <i86.h>
#include "asm_6502.h"

const char *dis[256] =
{"BRK","ORAxi","?","?","?","ORAz","ASLz","?",
 "PHP","ORA#","ASLA","?","?","ORA$","ASL$","?",
 "BPLr","ORAiy","?","?","?","ORAzx","ASLzx","?",
 "CLC","ORAy","?","?","?","ORAx","ASLx","?",
 "JSR$","ANDxi","?","?","BITz","ANDz","ROLz","?",
 "PLP","AND#","ROLA","?","BIT$","AND$","ROL$","?",
 "BMIr","ANDiy","?","?","?","ANDzx","ROLzx","?",
 "SEC","ANDy","?","?","?","ANDx","ROLx","?",
 "RTI","EORxi","?","?","?","EORz","LSRz","?",
 "PHA","EOR#","LSRA","?","JMP$","EOR$","LSR$","?",
 "BVCr","EORiy","?","?","?","EORzx","LSRzx","?",
 "CLI","EORy","?","?","?","EORx","LSRx","?",
 "RTS","ADCxi","?","?","?","ADCz","RORz","?",
 "PLA","ADC#","RORA","?","JMPI","ADC$","ROR$","?",
 "BVSr","ADCiy","?","?","?","ADCzx","RORzx","?",
 "SEI","ADCy","?","?","?","ADCx","RORx","?",
 "?","STAxi","?","?","STYz","STAz","STXz","?",
 "DEY","?","TXA","?","STY$","STA$","STX$","?",
 "BCCr","STAiy","?","?","STYzx","STAzx","STXzy","?",
 "TYA","STAy","TXS","?","?","STAx","?","?",
 "LDY#","LDAxi","LDX#","?","LDYz","LDAz","LDXz","?",
 "TAY","LDA#","TAX","?","LDY$","LDA$","LDX$","?",
 "BCSr","LDAiy","?","?","LDYzx","LDAzx","LDXzx","?",
 "CLV","LDAy","TSX","?","LDYx","LDAx","LDXy","?",
 "CPY#","CMPxi","?","?","CPYz","CMPz","DECz","?",
 "INY","CMP#","DEX","?","CPY$","CMP$","DEC$","?",
 "BNEr","CMPiy","?","?","?","CMPzx","DECzx","?",
 "CLD","CMPy","?","?","?","CMPx","DECx","?",
 "CPX#","SBCxi","?","?","CPXz","SBCz","INCz","?",
 "INX","SBC#","NOP","?","CPX$","SBC$","INC$","?",
 "BEQr","SBCiy","?","?","?","SBCzx","INCzx","?",
 "SED","SBCy","?","?","?","SBCx","INCx","?"};

int Disasm(int pc,char *buffer,unsigned char *p)
{
   const char *d=dis[*p++];
   sprintf(buffer,"%04X: ",pc);
   buffer+=6;
   if (strcmp(d,"?")==0)
   {
      strcpy(buffer,"???");
      return 1;
   }
   else if (strcmp(d+3,"")==0)
   {
      strcpy(buffer,d); buffer[4]='\0';
      return 1 ;
   }
   else if (strcmp(d+3,"r")==0)
   {
      int delta=*p++;
      if (delta>=128) delta-=256;
      strcpy(buffer,d);
      sprintf(buffer+3," $%04X",pc+2+delta);
      return 2;
   }
   else if (strcmp(d+3,"$")==0)
   {
      int l,h;
      l=*p++; h=*p++; l=(l+(h<<8));
      strcpy(buffer,d);
      sprintf(buffer+3," $%04X",l);
      return 3;
   }
   else if (strcmp(d+3,"I")==0)
   {
      int l,h;
      l=*p++; h=*p++; l=(l+(h<<8));
      strcpy(buffer,d);
      sprintf(buffer+3," ($%04X)",l);
      return 3;
   }
   else if (strcmp(d+3,"x")==0)
   {
      int l,h;
      l=*p++; h=*p++; l=(l+(h<<8));
      strcpy(buffer,d);
      sprintf(buffer+3," $%04X,x",l);
      return 3;
   }
   else if (strcmp(d+3,"y")==0)
   {
      int l,h;
      l=*p++; h=*p++; l=(l+(h<<8));
      strcpy(buffer,d);
      sprintf(buffer+3," $%04X,y",l);
      return 3;
   }
   else if (strcmp(d+3,"#")==0)
   {
      strcpy(buffer,d);
      sprintf(buffer+3," #$%02X",*p++);
      return 2;
   }
   else if (strcmp(d+3,"xi")==0)
   {
      int a;
      a=*p++;
      strcpy(buffer,d);
      sprintf(buffer+3," ($%02X,x)",a);
      return 2;
   }
   else if (strcmp(d+3,"iy")==0)
   {
      int a;
      a=*p++;
      strcpy(buffer,d);
      sprintf(buffer+3," ($%02X),y",a);
      return 2;
   }
   else if (strcmp(d+3,"z")==0)
   {
      int a;
      a=*p++;
      strcpy(buffer,d);
      sprintf(buffer+3," $%02X",a);
      return 2;
   }
   else if (strcmp(d+3,"zx")==0)
   {
      int a;
      a=*p++;
      strcpy(buffer,d);
      sprintf(buffer+3," $%02X,x",a);
      return 2;
   }
   else if (strcmp(d+3,"zy")==0)
   {
      int a;
      a=*p++;
      strcpy(buffer,d);
      sprintf(buffer+3," $%02X,y",a);
      return 2;
   }
   else if (strcmp(d+3,"A")==0)
   {
      strcpy(buffer,d);
      sprintf(buffer+3," A");
      return 1;
   }
   printf("Uh? (%s),d\n", "");
   exit(1);
}

int curchar;

void SpecialWrite()
{
   if (false)
      printf("** Special write M[%04X]=%02X **\n",
             asm_6502_info->special_eai(),
             asm_6502_info->special_value);
   if (asm_6502_info->special_eai()==0xC010)
   {
      curchar&=0x7F;
   }
}

void SpecialRead()
{
   if (false) {
      printf("** Special read M[%04X] **\n",
             asm_6502_info->special_eai());
      asm_6502_info->special_value=*asm_6502_info->special_ea;
   }
   if (asm_6502_info->special_eai()==0xC000)
   {
      asm_6502_info->special_value=curchar;
   }
   else if (asm_6502_info->special_eai()==0xC010)
   {
      curchar&=0x7F;
   }
}

int main()
{
   FILE *f;
   int romsize;
   int speed=500000;
   int time,ot,wide=1;

   Virtual_6502 *v6502;
   v6502=New6502();

   v6502->special_write=SpecialWrite;
   v6502->special_read=SpecialRead;
   v6502->special_start=v6502->address_space+0xC000;
   v6502->special_end=v6502->special_start+0x0100;

   if ((f=fopen("apple2.img","rb"))!=NULL)
   {
      fread(&v6502->A,1,1,f);
      fread(&v6502->X,1,1,f);
      fread(&v6502->Y,1,1,f);
      fread(&v6502->S,1,1,f);
      fread(&v6502->P,1,2,f);
      fread(v6502->address_space,256,256,f);
      v6502->rom_start=v6502->address_space+0xC000;
   }
   else
   {
      if ((f=fopen("apple2.rom","rb"))==NULL)
      {
         printf("Unable to open rom image\n");
         exit(1);
      }
      fseek(f,0,2); romsize=ftell(f);
      if (romsize>65536)
      {
         printf("Bad rom image\n");
         exit(1);
      }
      fseek(f,0,0);

      if (fread(v6502->address_space+65536-romsize,1,romsize,f)!=romsize)
      {
         printf("Error loading rom image\n");
         exit(1);
      }

      v6502->rom_start=v6502->address_space+65536-romsize;
      v6502->PC=v6502->address_space[0xFFFC]+(v6502->address_space[0xFFFD]<<8);
   }

   {
      union REGS r;
      r.w.ax=0x01; int386(0x10,&r,&r);
   }


   time=0;
   ot=*(int *)v_m(0x46C);
   for(;;)
   {
      if (!wide)
      {
         char buf[80];
         static char Hex[] = "0123456789ABCDEF";
         int pc,i,j,k;
         pc=v6502->PC;
         for (i=0; i<25-16-1; i++)
         {
            k=Disasm(pc,buf,v6502->address_space+pc);
            for (j=0; j<39 && buf[j]; j++)
            {
               *B8000(i*80+41+j)=buf[j]|0x0F00;
            }
            while (j<39)
            {
               *B8000(i*80+41+j)=0x0F20;
               j++;
            }
            pc+=k;
         }
         sprintf(buf,"A=$%02X",v6502->A);
         for (j=0; buf[j]; j++)
         {
            *B8000(75+j)=buf[j]|0x0F00;
         }
         sprintf(buf,"X=$%02X",v6502->X);
         for (j=0; buf[j]; j++)
         {
            *B8000(80+75+j)=buf[j]|0x0F00;
         }
         sprintf(buf,"Y=$%02X",v6502->Y);
         for (j=0; buf[j]; j++)
         {
            *B8000(160+75+j)=buf[j]|0x0F00;
         }
         sprintf(buf,"%c%c%c%c%c%c%c",
                 (v6502->P&0x80 ? 'N' : '-'),
                 (v6502->P&0x40 ? 'O' : '-'),
                 (v6502->P&0x10 ? 'B' : '-'),
                 (v6502->P&0x08 ? 'D' : '-'),
                 (v6502->P&0x04 ? 'I' : '-'),
                 (v6502->P&0x02 ? 'Z' : '-'),
                 (v6502->P&0x01 ? 'C' : '-'));
         for (j=0; buf[j]; j++)
         {
            *B8000(320+73+j)=buf[j]|0x0F00;
         }

         for (j=0; j<256; j++)
         {
            *B8000(80*(25-16+(j>>4))+41+(j&15)*2+((j&15)>>1))=
                  Hex[v6502->address_space[j]>>4]|0x0F00;
            *B8000(80*(25-16+(j>>4))+41+(j&15)*2+((j&15)>>1)+1)=
                  Hex[v6502->address_space[j]&15]|0x0F00;
         }

      }

      {
         unsigned char *rp;
         unsigned short *wp;
         int i,j,f;

         f=((*(char *)0x46C)&0x04)?0x7000:0x0700;

         for (i=0; i<24; i++)
         {
            wp=(unsigned short *)B8000(i*(wide ? 80 : 160));
            rp=v6502->address_space+
                  (0x400+((i<<7)&0x0300)+
                   (((i&0x18)+((i&1)<<7))|
                    ((i&0x18)<<2)));
            for (j=0; j<40; j++)
            {
               int c;
               c=rp[j];
               if (c<32)
               {
                  *wp++=(c+64)|0x7000;
               }
               else if (c<64)
               {
                  *wp++=c|0x7000;
               }
               else if (c<96)
               {
                  *wp++=c|f;
               }
               else if (c<128)
               {
                  *wp++=(c-64)|f;
               }
               else
               {
                  *wp++=(c-128)|0x0700;
               }
            }
         }

         if (*(short *)v_m(0x041A)!=*(short *)v_m(0x041C))
         {
            if ((i=getch())==0) i=-getch();
            switch(i)
            {
            case -68:
               wide=!wide;
            {
               union REGS r;
               r.w.ax=0x03-wide*2; int386(0x10,&r,&r);
            }
               break;
            case -59:
               ot=(*(int *)v_m(0x46C)) - ot;
            {
               union REGS r;
               r.w.ax=0x03; int386(0x10,&r,&r);
            }
               printf("Relative average speed = %0.3f\n",
                      (time/100000.0)/(ot/18.181818));

               printf("\ntime=%i\not=%i\n",time,ot);
            {
               FILE *f;
               if ((f=fopen("apple2.img","wb"))==NULL)
               {
                  puts("Error creating image file");
               }
               else
               {
                  fwrite(&v6502->A,1,1,f);
                  fwrite(&v6502->X,1,1,f);
                  fwrite(&v6502->Y,1,1,f);
                  fwrite(&v6502->S,1,1,f);
                  fwrite(&v6502->P,1,2,f);
                  fwrite(v6502->address_space,256,256,f);
                  fclose(f);
                  puts("Image file saved");
               }
            }
               exit(1);
               break;
            case -60:
               v6502->PC=v6502->address_space[0xFFFC]+
                     (v6502->address_space[0xFFFD]<<8);
               break;
            case -61:
               speed=-abs(speed);
               break;
            case -62:
               speed=abs(speed);
               break;
            case -63:
               speed=(speed<0 ? -100000 : 100000);
               break;
            case -64:
               speed=(speed<0 ? -1 : 1);
               break;
            default:
               if (i>0)
               {
                  curchar=i|0x80;
               }
               break;
            }
         }
      }
      if (speed>0)
      {
         ;while (inp(0x3DA)&8);
         ;while (!(inp(0x3DA)&8));
      }
      else
      {
         while(*(short *)v_m(0x041A)==*(short *)v_m(0x041C))
         {
         }
      }
      time+=Execute6502(v6502,abs(speed))/10;
   }
}
