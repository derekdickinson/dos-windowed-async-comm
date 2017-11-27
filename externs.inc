EXTRN   _fasrite:NEAR

EXTRN   _framerr:BYTE
EXTRN   _crcerrs:BYTE
EXTRN   _resent:BYTE
EXTRN   _outofwind:BYTE

EXTRN   _charin:BYTE
EXTRN   _crcalc:WORD
EXTRN   _crcin:WORD

EXTRN   _outbuf:WORD
EXTRN   _inbuf:WORD

EXTRN   _cseg_intl:WORD ; Code segment of interrupt handler
EXTRN   _mystack:WORD   ; Local stack pointer.
EXTRN   _videobase:WORD

EXTRN   _txstate:WORD   ; global byte variable (unsigned char)
EXTRN   _rxstate:WORD

EXTRN   _sendseq:BYTE
EXTRN   _sendack:BYTE
EXTRN   _rcvack:BYTE
EXTRN   _firstime:BYTE

EXTRN   _rcvhead:WORD
EXTRN   _rcvtail:WORD
EXTRN   _rcvdata:WORD

EXTRN   _inbufs:BYTE  
EXTRN   _dummybuf:BYTE

EXTRN   _startimer:BYTE

EXTRN   _arrived:BYTE 

EXTRN  _mdmstack:WORD

EXTRN   _sendlen:WORD
EXTRN   _rcvlens:WORD

