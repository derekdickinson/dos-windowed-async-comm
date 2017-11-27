#include <bios.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>

/* Make sure these functions are implemented by functions, the macro
implementations are not reliable. */
#undef  inportb
#undef  outportb

#define FALSE 0
#define TRUE  1

#define UPRQ   0x1051
#define LWRQ   0x1071
#define UPRR   0x1352
#define LWRR   0x1372
#define UPRS   0x1F53
#define LWRS   0x1F73
#define ENTR   0x1C0D
#define UPARR  0x4800
#define DNARR  0x5000
#define LFARR  0x4B00
#define RGARR  0x4D00
#define ESCKE  0x011B

#define COM1 TRUE

#if COM1

#define COMBASE   0x3F8
#define TXHOLD    0x3F8
#define RXDATA    0x3F8
#define BAUDLOW   0x3F8
#define BAUDHIGH  0x3F9
#define INTENA    0x3F9
#define INTIDREG  0x3FA
#define LINECTRL  0x3FB
#define MDMCTRL   0x3FC
#define LINESTUS  0x3FD
#define MDMSTUS   0x3FE
#define COM_INT   0x0C

#else

#define COMBASE   0x2F8
#define TXHOLD    0x2F8
#define RXDATA    0x2F8
#define BAUDLOW   0x2F8
#define BAUDHIGH  0x2F9
#define INTENA    0x2F9
#define INTIDREG  0x2FA
#define LINECTRL  0x2FB
#define MDMCTRL   0x2FC
#define LINESTUS  0x2FD
#define MDMSTUS   0x2FE
#define COM_INT   0x0b

#endif

#define MINLIN   5
#define MAXLIN   10

#define BUFFSIZE 1024

#define NUMBUFS      16
#define WINSIZ        8
#define WINSIZ1       9

#define MAXSEQ       15

#define STARTRCV      2

#define TXIDLE        0
#define STARTSEND     2
#define PACKDONE     20

typedef unsigned int uint;
typedef unsigned char boolean;
typedef unsigned char byte;

#define HOSTIDLE ( (txstate==PACKDONE) && (acktimeactive) &&\
                   ( (*nowtime-acktime) > acktimeout) )

#define HOSTREADY ( (txstate==PACKDONE) && (sendata!=sendhead) &&\
                    ( diff(sendhead,sendtail) < WINSIZ ) )

#define MODLESS(i) ( ( i-1 ) & 0x0f )

#define INC(i)     i++; i&=0x0f

#define DEC(i)     i--; i&=0x0f

#define SETTXVALS(i)  sendseq=i;              sendack=rcvtail;\
                      outbuf=outbufs[i];      sendlen=sendlens[i];\
                      buftimers[i]=*nowtime;  timeractive[i]=TRUE

#define STRTSEND  txstate=STARTSEND;        outportb( MDMCTRL, 0x0B );\
                  outportb( INTENA, 0x03 ); acktimeactive=FALSE

#define DUMPARR(i,st) fastform(i,15,\
          "%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",\
          st[0],st[1],st[2],st[3],st[4],st[5],st[6],st[7],st[8],st[9],st[10],\
          st[11],st[12],st[13],st[14],st[15])

#define PUTPRAMS  fastform(5,31,"%d   ",blksize); fastform(6,31,"%u     ",divs[rateind].speed);\
                  fasrite(7,31,"\xff\x2f ");\
                  if (dosend) fastform(7,31,"%s",sendname); else fasrite(7,31,"None");\
                  fasrite(8,31,"\xff\x2f ");\
                  fastform(8,31,"%s",rcvname); fastform(9,31,"%ld      ",acktimeout);\
                  fastform(10,31,"%ld      ",timeout)

/* from scrnlib.asm */
void fasrite(byte ro, byte col,char *st);
void gorocol(byte ro,byte col);
void invblk(byte ro, byte strt, byte stop);
void attclr(byte ro, byte strt, byte stop);

/* from inthandl.asm */
void setseg(void);
void interrupt int_handler();
void interrupt (*oldint)();

/* Main status screen, my video routine uses runlength compression with the
"FF" character as escape. */
char startscrn[]=
"É\xFF\x4EÍ»"
"º Spring 1988 CS-690 Project, Derek Dickinson\xFF\x22 º"
"Ì\xFF\x4EÍ¹\xff\1\xf"
"º Use Arrow keys and the Enter key to change these values (Esc to exit).\xFF\7 º\xff\1\xe"
"º Block Size (bytes)\xFF\x8 :\xFF\x32 º"
"º Data Rate (bps)\xFF\xB :\xFF\x32 º"
"º Send Filename\xFF\xD :\xFF\x32 º"
"º Receive Filename\xFF\xA :\xFF\x32 º"
"º Host Idle timeout (ticks) :\xFF\x32 º"
"º Sendframe Timeout (ticks) :\xFF\x32 º"
"Ì\xFF\x4EÍ¹\xff\1\xf"
"º\xFF\xF 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15 º\xff\1\xe"
"º Frame Error\xFF\x42 º"
"º CRC Error\xFF\x44 º"
"º Multy Rec.\xFF\x43 º"
"º Out of Wind.\xFF\x41 º"
"º Arrived\xFF\x46 º"
"º Timeractive\xFF\x42 º"
"º Retrans.\xFF\x45 º"
"Ì\xFF\x4EÍ¹\xff\1\xf"
"º  sh³st³sd³  sl³sa³ss³ds³tx³rh³rt³rd³ra³rx³st³aa³acktimer³ft³   hr³   hi³ re  º"
"º\xFF\x4E º"
"Ì\xFF\x4EÍ¹"
"º Data received : 0\xFF\x16 ³ Data Transmitted : 0\xFF\x10 º"
"È\xFF\x4EÍ¼";

typedef struct
{
  uint speed;
  uint divisor;
} SPEEDIV;

#define MAXRATIND 16

/* Divisor and associated speed. */
SPEEDIV divs[]=
{
  {     50, 0x900 },  {    75, 0x600 },  {   110, 0x600 },  {    150, 0x300 },
  {    300, 0x180 },  {   600, 0x0C0 },  {  1200, 0x060 },  {   1800, 0x040 },
  {   2000, 0x03A },  {  2400, 0x030 },  {  3600, 0x020 },  {   4800, 0x018 },
  {   7200, 0x010 },  {  9600, 0x00C },  { 19200, 0x006 },  { 0x9600, 0x003 },
  { 0xDAC0, 0x002 }
};

/* Configurable communications parameters. */
uint blksize=1024;
byte rateind=14;

char sendname[200]="sendfile.dat",
      rcvname[200]="rcvfile.dat";

unsigned long timeout=150,
              acktimeout=20;

FILE *sendfile,*rcvfile;

/* These variables are indexes into the arrays of buffers. */
volatile uint rcvhead=8,   /* Head of receive window. */
              rcvtail=0,   /* Tail of receive window. */
              rcvdata=0,   /* Next location from which to read data. */
              sendhead=0,  /* Head of Send window. */
              sendtail=0,  /* tail of Send window. */
              sendata=0;   /* Next location to write data into. */

/* These are counts of the data sent and received.  They are not critical
to the function of the program but are written to the screen to keep the
user entertained. */
unsigned long int datain=0,dataout=0;

uint mdmstack[800],  /* A local stack for the interrupt handler. */
     cseg_intl;      /* The code segment will be stored here.    */


volatile uint rxstate=0, /* State variables used to keep track of     */
              txstate=0; /* communication status between interrupts.  */

/* The original 8259 interrupt mask.  This value is restored at exit. */
uint startmask;

/* Used by the interrupt handler to inform the background code to start
timing distance between Acks. */
volatile boolean startimer=FALSE;

boolean dosend=FALSE,    /* Set when a file is in process of being txed. */
        firstime=TRUE;   /* This variable causes the interrupt handler to
                            take the next incoming frame as the start of
                            the receive window.  This is only necessary
                            when the package is first booted up. */

byte sendseq=0,      /* The sequence number of frame to be sent. */
     sendack=0,      /* The acknowledgement of frame to be sent. */
     rcvack=MAXSEQ;  /* The last ack number received. */

/* These are all counts used to indicate how the transmission is proceeding. */
byte framerr[16],    /* framerr[i]= numbers of framing errors with sequence # i. */
     crcerrs[16],    /* crc (really checksums) errors. */
     outofwind[16],  /* value received out of the receive window. */
     resent[16],     /* Value received twice. */
     retrans[16],    /* Value transmitted twice. */
     retran=0;       /* Number of retransmissions. */

uint hostready=0,    /* Number of normal transmissions. */
     hostidle=0;     /* Number of idle acknowledgements. */

/* The below four arrays would normally be accessed in a structure.  They
are not because the interrupt handler can calculate the addresses more
rapidly if they are seperate. */

volatile char outbufs[NUMBUFS][BUFFSIZE];  /* output buffers. */
uint sendlens[NUMBUFS];                    /* Their lengths. */
unsigned long buftimers[NUMBUFS];          /* Their timers. */
boolean timeractive[NUMBUFS];              /* Is above timer active? */

/* Throwaway buffer if an invalid frame is received (retransmission or out
of window). */
volatile char dummybuf[BUFFSIZE];

/* The length of the buffer to be transmitted. */
uint sendlen;

/* Input buffers and associated values.  */
volatile char inbufs[NUMBUFS][BUFFSIZE];
uint rcvlens[NUMBUFS];
boolean arrived[NUMBUFS]=
  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };

/* Host Idle acknowledgement timer and boolean indicator. */
unsigned long acktime=0L;
boolean acktimeactive=FALSE;

/* Buffer pointers used in the interrupt handler to minimize the addressing
calculations required. */
volatile char *outbuf=outbufs[0],*inbuf=inbufs[0];

/* Variables used in scrnlib.asm */
extern int video_base, inv_mask, clr_mask, video_attr, vidmode;

/* Use video mode to determine how and where to write direct video writes. */
void getvidmode(void)
{
  union REGS inregs,outregs;

  inregs.h.ah=0x0f;
  int86(0x10,&inregs,&outregs);
  vidmode=outregs.h.al;

  switch (vidmode)
  {
    case 2:{video_base=0xb800;
            inv_mask=0xf870;
            clr_mask=0x8807;
            video_attr=0x07; break;}
    case 3:{video_base=0xb800;
            inv_mask=0x8f20;
            clr_mask=0x8f00;
            video_attr=0x0e; break;}
    case 7:{video_base=0xb000;
            inv_mask=0xf870;
            clr_mask=0x8807;
            video_attr=0x07; break;}
    default:{
      video_base=0xb800;
      inv_mask=0x8f20;
      clr_mask=0x8f00;
      video_attr=0x0e;
      inregs.h.ah=0;
      inregs.h.al=vidmode=3;
      int86(0x10,&inregs,&outregs);
    }
  }

}

/* This is used for writing formatted output directly to the screen.
The "fasrite" function is a direct video write routine in scrnlib.asm. */
void fastform(byte ro,byte col,char *st, ...)
{
  va_list argptr;       /* Argument list type. */
  char dummyst[300];

  va_start(argptr,st);          /* Initialize list. */
  vsprintf(dummyst,st,argptr);  /* Use as argument for vsprintf. */
  va_end(argptr);               /* Clean up result. */

  fasrite(ro,col,dummyst);      /* Write resulting string to screen. */
}

/* Enables 8259 mask register for COM interrupt. */
void ena8259(int n)
{
  int mask,inportz;

  mask=0x0004 << (n-0x000a);
  inportz=inportb(0x21);            /* get interrupts */
  inportz&=~(mask);
  disable();
  outportb(0x21,inportz);
  enable();
}

/* Save system setup. */
void save_setup(void)
{
  startmask=inportb(0x21);
  oldint=getvect(COM_INT); /* get previous interrupt vector */
}

void restore_setup(void)
{
  /* restore previous interrupt vector */
  setvect(COM_INT,oldint);
  outportb(0x21,startmask);
}

void stopints(void)
{
  outportb( INTENA, 0x00);
  outportb( MDMCTRL, 0x00);
}

/*---------------------------------------------------------------------------*/

void initialize(void)
{
  txstate=0;
  rxstate=0;
  setvect(COM_INT,int_handler);       /* install interrupt vector */

  ena8259(COM_INT);                   /* Enable 8259 interrupt */

  outportb( LINECTRL, 0x80 );         /* Select baud control registers */
  outportb( BAUDHIGH, (divs[rateind].divisor >> 16)  );
  outportb( BAUDLOW,  (divs[rateind].divisor & 0x00ff) );

  outportb( LINECTRL, 0x03 );         /* 8bits, 1 stop, no parity */

  outportb( INTENA, 0x01 );         /* Enable receive interrupt.? */

  while ( (inportb(INTIDREG) & 0x01)==0 )
  {
    inportb( LINESTUS );
    inportb( RXDATA );
    inportb( MDMSTUS );
    if (bioskey(1)) break;
  }

  outportb( MDMCTRL, 0x09 );          /* Enable interrupt 3-state buffer
                                         and turn on DTR */
}

/*---------------------------------------------------------------------------*/

void putscrn(void)
{
  fasrite(1,1,startscrn);
  PUTPRAMS;
  invblk( MINLIN, 31, 78);
}

void openrcv(void)
{

  if ( (rcvfile=fopen("rcvfile.dat","wb"))==NULL )
  {
    printf("Unable to open file \"rcvfile.dat\"");
    exit(1);
  }
}

void newrate(void)
{
  if (rateind>MAXRATIND) rateind=MAXRATIND;
  if (rateind<0) rateind=0;
  outportb( INTENA, 0x00); /* Disable COM interrupts. */

  outportb( LINECTRL, 0x80 );         /* Select baud control registers */
  outportb( BAUDHIGH, (divs[rateind].divisor >> 16)  );
  outportb( BAUDLOW,  (divs[rateind].divisor & 0x00ff) );

  outportb( LINECTRL, 0x03 );         /* 8bits, 1 stop, no parity */

  outportb( INTENA, 0x01 );         /* Enable receive interrupt.? */
}

void startsend(void)
{
  fasrite(7,31,"\xff\x30 ");
  gorocol(7,31);
  gets(sendname);
  if ( (sendfile=fopen(sendname,"rb"))==NULL )
  {
    gorocol(26,1);
    return;
  }
  gorocol(26,1);
  dosend=TRUE;
  sendlens[sendhead]=fread(outbufs[sendhead],1,blksize,sendfile);
  dataout=sendlens[sendhead];
  sendata=(sendhead+1) & 0x0f;
}

void newrcv(void)
{
  fclose(rcvfile);

  while (TRUE)
  {
    fasrite(8,31,"\xff\x20 ");
    gorocol(8,31);
    gets(rcvname);
    if ( (rcvfile=fopen(rcvname,"wb"))==NULL )
    {
      fastform(8,31,"Unable to open file \"%s\"",rcvname);
      return;
    }
    else break;
  }
  gorocol(26,1);
}

boolean between( char tail, char mid, char head )
{
  byte i=0;

  if (tail<=mid) i++;
  if (mid<head)  i++;
  if (head<tail) i++;
  return(i>1);
}

int diff(int head, int tail)
{
  if (head<tail) return( head+MAXSEQ-tail );
  else return( head-tail );
}

void goproto(void)
{
  char far *keyhead,far *keytail;
  unsigned long far *nowtime;
  byte i,chline=MINLIN;

  putscrn();

  nowtime=MK_FP(0,0x046c);  /* The systems clocks location in low memory. */

  keyhead=MK_FP(0,0x041a);  /* Keyboard buffer, head and tail locations. */
  keytail=MK_FP(0,0x041c);

  txstate=PACKDONE;
  rxstate=STARTRCV;
  sendata=sendhead;

  while (TRUE)
  {
    if (*keyhead!=*keytail) /* Faster version of kbhit(). */
    {
      boolean done=FALSE;

      attclr( chline, 31, 78 );
      switch (bioskey(0))
      {
        case UPRS : case LWRS : startsend();             break;
        case UPRR : case LWRR : newrcv();                break;
        case UPRQ : case LWRQ : case ESCKE: done=TRUE;   break;
        case UPARR: if (--chline<MINLIN) chline=MAXLIN;  break;
        case DNARR: if (++chline>MAXLIN) chline=MINLIN;  break;
        case LFARR:
          switch (chline)
          {
            case  5: if ((blksize-=32)<32) blksize=32; break;
            case  6: rateind--; newrate();             break;
            case  7: startsend();                      break;
            case  8: newrcv();                         break;
            case  9: acktimeout--;                     break;
            case 10: timeout--;                        break;
          }
          break;
        case RGARR: case ENTR :
          switch (chline)
          {
            case  5: if ((blksize+=32)>1024) blksize=1024; break;
            case  6: rateind++; newrate();                 break;
            case  7: startsend();                          break;
            case  8: newrcv();                             break;
            case  9: acktimeout++;                         break;
            case 10: timeout++;                            break;
          }
          break;
      }
      PUTPRAMS;
      invblk( chline, 31, 78 );
      if (done) break;
    }

    while ( rcvdata != rcvtail )
    {
      fwrite(inbufs[rcvdata], 1, rcvlens[rcvdata], rcvfile );
      datain+=rcvlens[rcvdata];
      INC(rcvdata);
    }

    if (!firstime)
    {
      while ( between(sendtail,(rcvack-1) & 0x0f,sendhead) )
      {
        timeractive[sendtail]=FALSE;
        INC(sendtail);
      }
    }

    if (startimer)
    {
      if (!acktimeactive) acktime=*nowtime;
      acktimeactive=TRUE; startimer=FALSE;
    }

    if (dosend)
    {
      while ( sendata!=MODLESS(sendtail) )
      {
        if ( (sendlens[sendata]=fread(outbufs[sendata],1,blksize,sendfile ))
              < blksize)
        {
          dataout+=sendlens[sendata];
          if (sendlens[MODLESS(sendata)]!=0) INC( sendata );
          fclose(sendfile); dosend=FALSE;
          break;
        }
        dataout+=blksize;
        INC( sendata );
      }
    }

    if (txstate==PACKDONE)
    {
      for ( i=0; i<=MAXSEQ ; i++)
      {
        if ( (timeractive[i]) && ((*nowtime-buftimers[i]) > timeout ) )
        {
          retran++;  retrans[i]++;
          SETTXVALS(i); /* This macro sets up some variables for the send. */
          STRTSEND; /* This macro initiates the transmission. */
          break;
        }
      }
    }

    if (HOSTREADY)
    {
      hostready++;
      SETTXVALS(sendhead);  /* This macro sets up some variables for the send. */
      INC( sendhead );
      STRTSEND;      /* This macro initiates the transmission. */
    }

    if ( HOSTIDLE )
    {
      hostidle++; sendlen=0; sendack=rcvtail;
      STRTSEND;
    }

    /* Now update the screen. */
    DUMPARR(13,framerr);
    DUMPARR(14,crcerrs);
    DUMPARR(15,resent);
    DUMPARR(16,outofwind);
    DUMPARR(17,arrived);
    DUMPARR(18,timeractive);
    DUMPARR(19,retrans);
    fastform(22,4,"%2d³%2d³%2d³%4d³%2d³%2d³%2d³%2d³%2d³%2d³%2d³%2d³%2d³%2d³%2d³%8ld³%2d³%5d³%5d³%3d",
      sendhead,sendtail,sendata,sendlen,sendack,sendseq,dosend,txstate,rcvhead,
      rcvtail,rcvdata,rcvack,rxstate,startimer,acktimeactive,acktime,firstime,
      hostready,hostidle,retran);
    fastform(24,19,"%lu       ",datain);    fastform(24,63,"%lu       ",dataout);

  } /* while (TRUE) */
}

void main(void)
{
  getvidmode();
  clrscr();
  gorocol(26,1);

  openrcv();
  setseg();

  save_setup();
  if (0!=(atexit(restore_setup) ) ) exit(1);
  if (0!=(atexit(stopints) ) ) exit(1);

  initialize();

  goproto();

  fcloseall();

  clrscr();

  gorocol(23,1);

  stopints();
  restore_setup();

}
