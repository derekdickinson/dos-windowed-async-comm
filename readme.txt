This is a brief document to give a general understanding of the way the
program operates.  All bets are off in terms of English quality on this
one.

The program is (weakly) modeled after the example (Pg. 162) in Tanenbaum.
There are several major differences which I will point out here to make
the code easier to read.

   1.  Naks are not supported.  The program relies entirely on the buffer
       timers.

   2.  In Tanenbaum, the buffers used to send and receive data are not the
       "Input" and "Output" buffers.  Data is copied from "Input[?]" into
       "s.info" to transmit and from "r.info" into "Output[?]" when received.
       This is not a problem when trying to illustrate a protocol but when
       trying to perform the data movement in a timely fashion it is a
       problem.  To help alleviate it, I used 16 input and 16 output
       buffers.  The buffers correspond to the sequence numbers being
       used.  Since only 8 buffers (maximum) can be in the
       transmission/reception window the remaining buffers can be used to
       transfer data to and from disk.  This technique requires that a
       producer-consumer relationship be maintained to avoid overwriting
       of data.  The "sendata" and "rcvdata" variables are used for this
       purpose.

   3.  The acknowledgements are "Next frame expected" rather than "Last good
       frame received".  This is more consistent with popular protocols.  It
       doesn't seem to matter which is used.

   4.  The breakup of jobs into functions is significantly different.
       This is partly a matter of my programming style and partly due to
       the logical breakup of function into the interrupt handler and
       the C code.

So much for Tanenbaum.  Now a brief rundown on what functions are performed
where.

   Interrupt Handler (inthandl.asm + includes)
     Receive
       1. Reception of characters and placing them in the buffers.
       2. Determining the framing and retrieving the sequence numbers of
          the messages.
       3. Updating all the variables for the next reception.
       4. Calculation of checksum and check for error.
       5. The only function NOT performed in the interrupt handler for
          receive is taking the data from the buffer and putting it on
          disk.  This stops the slower background activity from causing
          missed frames (unless the data isn't stored fast enough).
     Transmission
       1. Sending of characters from buffer to 8250.
       2. Calculation of checksum.
       3. Most functions are performed in the C code since transmission is
          less time critical than reception.

   The C code. ( comsync.c )
     1. Initialization and setup of interrupt handlers, 8250 and 8259 chips
        and many internal variables.
     2. All screen I/O and keyboard input.
     3. All disk I/O.
     4. Initiation of all transmissions.
     5. Managing timers (buffer and the acktimer).

Framing:
  The framing used is crudely modeled after Bisync:

Data frame:

   DLE | STX | sequence #'s | length  | data | ETX | CRC |
   10h   02h    1-byte        2-bytes          03h   2-bytes

Ack frame:

   DLE | STX | sequence # | 00h 00h
   10h   02h    1-byte      2-bytes

The CRC bytes are only checksums right now, maybe CRC's when I get around
to it.  The checksum is on the data only.



Basic Structure of the interrupt handler
   The logical structure of the interrupt handler is merely a set of
   jump tables.

   The first jump table is "intjmp".  It is executed whenever an
   interrupt occurs and causes a branch to the appropriate routine.

   It can go one of four places.

     mdmint: A modem control signal has changed.  Since I don't enable
       modem interrupts this should never happen.  Nevertheless, I put
       a label out there and have code to merely return control to my
       program here.  This is not merely paranoia, the older 8250 have
       a bug which can cause spurios interrupts.

     txempt: The transmitter holding register is empty.  This indicates
       the 8250 can be given more data.

     rxchar: The 8250 has a character.

     rcpint: This and error/special condition interrupt.  Since I don't
       believe in having errors this interrupt is disabled.

   The two tables for mdmint and rcpint are merely for future expansion.
   The minimum necessary to handle the interrupt and return is there
   (I enabled the interrupt for some testing and there were no random
   jumps into memory).

   The send and receive interrupts each have another jump table which is
   indexed by a state variable.  This is used to implement the framing of
   the packets.  The state variables are set in the interrupt to the
   state required for the next interrupt.  All states are multiples of 2
   so they can be used without modification as jump table indexes.


The different files involved.

  INTASYNC.ASM : This is the file containing the interrupt handler and
    some support functions.  This gives the general structure of the code
    but most of the details are hidden in macros.  The include files
    contains macros which are categorized by tasks.

  RCV.INC : You guessed it.  Yep its got the receive macros all right.

  SEND.INC : Right again.

  EXTERNS.INC : External declarations for variables which are declared
    int the C code.

  STATINTS.INC : The macros concerned with the handling of modem and
    receive error interrupts.  Since these interrupts are not enabled
    this is not the most exciting file.

  EQUATES.INC : The constants (equates) are defined in this include.

  STAND.INC : Some standard code for beginning and ending an interrupt
    handler.

  DATA.INC : The declarations for data used in the interrupt handler but
    not in the C code.

  SCRNLIB.ASM : Some direct video I/O routines which I have been using for
    some time.  They are especially useful when fast I/O is needed.

  COMSYNC.C
    The main program.  This contains all of the C code.

  COMSYNC.PRJ : The Turbo C project file.



How to run the program and what various display parameters are:

  The program is used to transfer data between two PC's via the serial
  ports.  A null modem cable or actual modems can be used.

  The program has only one screen.  The top part contains parameters which
  can be set and the bottom part contains status information.  Parameters
  can be changed by using the cursor control keys and/or the arrow keys.
  One of the parameters will be displayed in reverse video or yellow on
  green for color monitors (if this isn't the case try quitting the package
  and setting the video mode to "bw80").
    The parameter in reverse video can be changed by using the left and
  right arrow keys.  The Enter key works the same as the left arrow key.
  In the case of the send and receive filenames pressing enter over them
  will allow a filename to be entered.
    The Up and Down arrow keys change which parameter is in reverse video.
  Now a rundown on each parameter.

    Block Size : Just what you would expect the size of each packet in
      bytes.

    Data Rate  : No suprise here either.  Make sure both sides match, I
      don't pass information on rate back and forth.

    Send Filename : The name of the file to send.  Entering the name will
      initiate the transmission of the file if it exists.  If it doesn't
      then "None" will promptly be displayed.

    Receive filename: The name of the file to which data will be written.
      When a new name is entered the old file is closed and a new one is
      opened.  There is no automatic closing of a file when the last block
      from the other side was sent.  Therefore, if the name isn't changed
      between two receptions of a file then the files will be concatenated.

    Host Idle Timeout: Same as in Tanenbaum.  The time the line must be
      idle before a non-piggybacked acknowledgement.  Since the lind is
      full duplex there is little reason to make this number large.

    Sendframe Timeout: The time that must pass without acknowledgment
      before a packet will be retransmitted.

  The last two timers are in units of "ticks" of the system clock which
  occurs 18.2 times a second unless someone has gone wild on the system
  before I got on it.  The optimum values for the timers are functions of
  several things including block size and data rate.

  The rest of the screen contains status information.  Some of the
  information is broken down by sequence numbers and other information
  is merely individual value.  All these values will be explained here:

    Frame Error: This is a receiver error which occurs when an ETX is not
      where the length field would indicate it should be.  This will
      normally happen when the receive buffer has been overrun.

    CRC Error: Actually a checksum error.  Obviously receive.

    Multy Rec. : Multiple receive, the frame was received more than once.

    Out of Wind : Out of window, the frame was received when it wasn't in the
      receive window.

    Arrived : Value indicating that a frame in the active window has been
      received.  This will only be non-zero if one or more frames is
      received out of order.  Otherwise the bit will be set and then
      immediately cleared when the window is advanced.

    Timeractive : This is the transmit buffer timer.  These are true for
      all buffers in the transmit window.

    Retrans. : Number of retransmissions for a given sequence number.

  The line below contains the values of several other internal variables:

    sh : Send Head, the head of the sending window.
    st : Send Tail, the tail of the sending window.
    sd : Send Data, the next buffer in which to place transmit data.
    sl : Send Length, the length of the buffer being transmitted.
    sa : Send Ack, the next ack to be sent.
    ss : Send Sequence, the sequence number of the packet being transmitted.
    ds : DoSend boolean, true when a file is being transmitted.
    tx : The transmit state variable.
    rh : Receive Head, the head of the receive window.
    rt : Receive Tail, the tail of the receive window.
    rd : Receive Data, the next location from which to read data.
    ra : Receive Ack, the last ack received.
    rx : Receive state variable.
    st : Startimer, boolean telling C code to start the acktimer.
    aa : Acktimer Active, boolean indicating actimer is running.
    acktimer : The value of the acktimer.
    ft : Firstime, used to set receive window to next packet received.
    hr : Count of Hostready transmissions.
    hi : Count of Hostidle transmissions.
    re : Count of retransmissions.
    Data Received: Total number of bytes received.
    Data Transmitted : Number of bytes transmitted, this file.


That should be enough to get you oriented.  The code is sporadicly
commented and the variable names are intended to be mneumonic.
