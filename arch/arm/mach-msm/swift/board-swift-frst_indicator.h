#ifndef _BOARD_SWIFT_FRST_INDICATOR_H
#define	_BOARD_SWIFT_FRST_INDICATOR_H

#define	FRST_INDICATOR_MAGIC	't'

typedef struct
{
	unsigned long size;
	unsigned char buff[128];
} __attribute__((packed)) frst_indicator_info;

typedef struct {
	unsigned char arm9_written_flag;
	unsigned char arm11_written_flag;
	unsigned char frststatus;
	unsigned char frstongoing;
} __attribute__ ((packed)) FactoryReset_Indicator;

#define FRSTINDICATOR_DEV_NAME         "frstindicator"

#define FRSTINDICATOR_DEV_MAJOR		240
#define FRSTINDICATOR_DEV_MINOR		10


#define FRST_INDICATOR_READ	_IOR(FRST_INDICATOR_MAGIC, 0, FactoryReset_Indicator)
#define FRST_INDICATOR_WRITE	_IOW(FRST_INDICATOR_MAGIC, 1, FactoryReset_Indicator)

#define FRST_INDICATOR_MAXNR	2

#endif
