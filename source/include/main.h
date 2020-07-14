#ifndef _MAIN_H_
#define _MAIN_H_

#include "stm32f10x.h"
//-----------------------------------------?
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

//GPIOC
#define LED_13_PIN              13

//GPIOA
#define SENSOR_A_PORT           GPIOA
#define SENSOR_PIN_A            0
#define STORAGE_PIN_A           1

#define SENSOR_B_PORT           GPIOA
#define SENSOR_PIN_B            2
#define STORAGE_PIN_B           3

#define SIM800_DTR_PORT         GPIOB
#define DTR_SIM800              10

#define SIM800_PORT             GPIOA
#define RST_SIM800              8
#define TX_FOR_SIM800           9
#define RX_FOR_SIM800           10

#define ERROR_ACTION(CODE,POS)		do{}while(0)
#define UNIX_DATA_2020      1577836800
#define CRITICAL_BATTERY    30 //in %
#define MIN_BATTERY         5  //in %
#define TRY_SYN_GSM_TIME    4   // how many tray syncronise GSM time

typedef struct 
{
 uint32_t   strCRC;
 char       PonyName[24];
 uint16_t   SensorA_without; 
 uint16_t   SensorB_without;
 uint8_t    Sensordelta;
 uint8_t    FreeSensorTime;
 uint16_t   RegularReportTime;
 char       AdminDTMFpassw[8];
 char       AdminPhoneNum_1[16];
 char       AdminPhoneNum_2[16];
 char       AdminPhoneNum_3[16];
 char       APNgprs[28];
 char       SMTPserv[24];
 char       SMTPname[28];
 char       SMTPpassw[20];
 char       EMAILdevice[24];
 char       EmailHost[36];
 char       NameHost[12];
} sSys_Param;

extern sSys_Param SysParam;
extern uint8_t flagSysParamRead;
extern uint16_t smaValueA,smaValueB;
extern uint16_t deviceFlags;

//Pending Flags
#define  FPENDING_SAVE_CONFIG       (uint16_t)1
#define  FPENDING_SMS_ADMIN         (uint16_t)2
#define  FPENDING_SMS_HARDSTATE     (uint16_t)4
#define  FPENDING_EMAIL_REPORT      (uint16_t)8
#define  FPENDING_SMS_GPRS_SETTING  (uint16_t)16
#define  FPENDING_SMS_HOST_SETTING  (uint16_t)32
//regular
#define  REGULAR_EMAIL_REP_REQEST   (uint16_t)64
#define  REGULAR_EMAIL_REP_DONE     (uint16_t)128
  //errors flagg
#define  GPRS_CONNECT_ERROR         (uint16_t)256
#define  TIME_SYNC_ERROR            (uint16_t)512
#define  LOW_BATTERY_ENAERGY        (uint16_t)1024
#define  BROKEN_SENSOR              (uint16_t)2048    
#define  BROKEN_SENSOR_ALARMED      (uint16_t)4096
#define  PENDING_SENSOR_INFO        (uint16_t)8192
#define  FPENDING_EMAIL_YESTERDAY_REPORT    (uint16_t)16384

//Sms tuning Header flags
#define  NONE_HEADER_FLAG	    0
#define  APN_HEADER_FLAG	    1
#define  EMAILHOST_HEADER_FLAG	2
#define  HORSENAME_HEADER_FLAG	3
#define  SENSETIV_HEADER_FLAG   4
#define  REPORT_TIME_HEADER     5
#define  FREE_TIME_HEADER       6

void vBlinker (void *pvParameters);
void HumanHelp();
void vSIM800 (void *pvParameters);
uint8_t Discuss(uint16_t *PendingFlags);
void ParkDevice();
//void Standby();

#endif //MAIN_H_