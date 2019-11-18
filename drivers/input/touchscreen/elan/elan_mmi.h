#ifndef _LINUX_ELAN_MMI_H
#define _LINUX_ELAN_MMI_H


struct dynamic_anang_para {
	uint16_t origph1;
	uint16_t origph2;
	uint16_t origph3;
};

enum adc_type {
	NORMAL_ADC	= 0xCD,
	SHORT_ADC	= 0x34
};


enum data_type {
	NORMAL_TYPE	= 0x00,
	SHORT_TYPE	= 0xC0,
};

enum ack_type {
	ACK_OK			=	0,
	ERR_TIMEOUT		=	-1,
	ERR_I2CWRITE	=	-2,
	ERR_I2CREAD		=	-3,
	ERR_I2CPOLL		=	-4,
	ERR_DEVSTATUS	=	-5,
	ERR_DEVACK		=	-6,

	ERR_DISABLALG	=	-7,
	ERR_BUFEMPTY	=	-8,
	ERR_BUFFALLOC	=	-9,
	ERR_DYNAMICANALOG	=	-10,
	ERR_RECALIBRATE	=	-11,

	ERR_NORMALADC	= -12,
	ERR_SKIPFRAME	= -13,
	ERR_GETNORMALADC	=	-14,
	ERR_TESTMODE		= -15,
	ERR_RXOPEN			=	-16,
	ERR_TESTFRAME		=	-17,
	ERR_RXOPENTEST		= -18,

	ERR_SETPHX			= -19,
	ERR_LBTEST			= -20,
	

	ERR_SHORTMODE		= -21,
	ERR_GETSHORTDATA	= -22,
	ERR_SHORTDATA		= -23,
	ERR_SHORTTEST		= -24,
	ERR_MEANLBTEST		= -25,

	ERR_TXOPENTEST		= -26,
	ERR_TXOPEN			= -27,

	ERR_TXRXSHORT		= -28,

	ERR_GETRAWDATA	=	-120,
	ERR_TPMODUINIT	=	-121,	

};



#define NOISE_ADC_PRINT    0
#define SHORT_DATA_PRINT    1
#define RXTX_DIFF_PRINT    2

#endif




