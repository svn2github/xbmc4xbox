#ifndef _RAR_CRC_
#define _RAR_CRC_

extern uint CRCTab[256];

void InitCRC();
uint CRC(uint StartCRC,void *Addr,uint Size);
ushort OldCRC(ushort StartCRC,void *Addr,uint Size);

#endif
