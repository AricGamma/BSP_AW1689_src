#ifndef FEATURES_H
#define FEATURES_H

#define GSL_DEBUG
//#define GSL_TIMER
#define SUPPORT_WINKEY
#define I2C_TRANSMIT_LEN 16
//#define SCANTIME_CONSTANT
#ifdef SCANTIME_CONSTANT
#define SCANTIME_STEP 90
#endif
#define SCANTIME_SYSTEM

//#define SET_MULTITOUCH_5POINT
//#define WINKEY_WAKEUP_SYSTEM

//X=short y=long in driver,opposite in sileadtouch suite.
#define PORTRIAT_RESOLUTION	

//#define LANSCAPE_RESOLUTION

//#define GSL1680
//#define GSL2681
//#define GSL3670
//#define GSL3673
#define GSL3675
//#define GSL3676
//#define GSL3680
//#define GSL3692
//#define GSL5680

//remark: GSL2682 can use GSL2681 driver.
#ifdef GSL3670
#define GSL9XX_VDDIO_1800
#endif

/*
<=3673 800*1280
>3673  1200*1920
*/
#ifdef PORTRIAT_RESOLUTION
#if defined(GSL1680)||defined(GSL2681)||defined(GSL3670)||defined(GSL3673)
		#define PHYSICAL_MAX_X 425
		#define PHYSICAL_MAX_Y 677
		//logical
		#define LOGICAL_MAX_X 720
		#define LOGICAL_MAX_Y 1280
	#elif defined(GSL3675)||defined(GSL3676)||defined(GSL3680)||defined(GSL3692)||defined(GSL5680)
		#define PHYSICAL_MAX_X 543  //138mm  
		#define PHYSICAL_MAX_Y 866  //220mm
		//logical
		#define LOGICAL_MAX_X 800
		#define LOGICAL_MAX_Y 1280
	#else
		#error no defined ic
	#endif
#elif defined(LANSCAPE_RESOLUTION)
#if defined(GSL1680)||defined(GSL2681)||defined(GSL3670)||defined(GSL3673)
		#define PHYSICAL_MAX_X 677
		#define PHYSICAL_MAX_Y 425
		//logical
		#define LOGICAL_MAX_X 1280
		#define LOGICAL_MAX_Y 800
	#elif defined(GSL3675)||defined(GSL3676)||defined(GSL3680)||defined(GSL3692)||defined(GSL5680)
		#define PHYSICAL_MAX_X 866 //220mm
		#define PHYSICAL_MAX_Y 543 //138mm 
		//logical
		#define LOGICAL_MAX_X 1200
		#define LOGICAL_MAX_Y 1920
	#else
		#error no defined ic
	#endif
#else
	#error not defined portrait or landscape
#endif



#ifdef GSL_DEBUG
#define DbgPrintGSL  DbgPrint
#else
#define DbgPrintGSL  //
#endif
#endif	//FEATURES_H