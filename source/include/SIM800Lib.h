#ifndef _SIM800_H_
#define _SIM800_H_

#include "main.h"
#include "Cfunction.h"
#include "time.h"

typedef enum
{
  NOT_REG       =0,
  HOME_NET      =1,
  SEARCHING     =2,
  DENNY_REG     =3,
  UNKNOW_REG    =4,
  ROAMING       =5,
  ERROR_CREG    =15
} NetworkRegType;

typedef enum 
{
  FILE_WRITE_BUSY,
  FILE_WRITE_NOT_PROMPT,
  FILE_WRITE_TIMEOUT,
  FILE_WRITE_SUCCESS
} retcode_Sim800_FileWrite;

typedef enum eRetcode
{
  SIMPLE_CMD_FAILED_SEND,
  //SIMPLE_CMD_NOT_ECHO,
  SIMPLE_CMD_BAD_ANSWER,
  //SIMPLE_CMD_ERROR,
  SIMPLE_CMD_SUCCESS
} retcode_SIM800_SimpleCmd;


typedef enum
{
  STATUS_DIR_NONE       =   0,
  STATUS_DIR_INCOMING,
  STATUS_DIR_ACTIVE_OUT,  
  STATUS_DIR_ACTIVE_IN
} eCallStatDir;

typedef enum
{
  EVENT_NONE,
  EVENT_DTMF,
  EVENT_NOCARRIER,
  EVENT_CREC0
} retcode_Event;

typedef struct
{
  uint8_t Year;
  uint8_t Month;
  uint8_t Day;
  uint8_t Hour;
  uint8_t Minute;
  uint8_t Second;
  int8_t zz;
} timestruct;

#define SIZE_DTMF_BUF   20
typedef struct 
{
 char       PhoneNum[16];
 char       AccumDTMF[SIZE_DTMF_BUF+1];
 uint8_t    headDTMF;
 uint8_t    AdminNum;
 eCallStatDir    Status;
} sActiveCall;
#define SIZE_ACTIVECALL     sizeof(sActiveCall)

void Sim800_RST();
void SIM800_Sleep(uint8_t sleep);
uint16_t Sim800_WaitLine(char *Dest,  uint16_t TimeOut);
void Sim800_WaitArray(char *Dest, uint16_t *LenStr, uint16_t TimeOut);
uint8_t waitSilent ();
retcode_SIM800_SimpleCmd SimpleCmd(const char *CmdPtr,uint16_t LenCmd);
void Sim800_Prepare();
NetworkRegType Sim800_CheckReg();
uint8_t Sim800_LocalTimeStamp();
time_t Sim800_GetTime ();
uint8_t Sim800_GetBattery ();
retcode_SIM800_SimpleCmd Sim800_GsmBusy(uint8_t busy);
uint8_t Sim800_FileExist(const char *OnlyName);
uint8_t Sim800_FileCreate(const char *PathName);
uint8_t Sim800_FileSize(const char *OnlyName, uint16_t *size);
retcode_Sim800_FileWrite Sim800_FileWrite(const char *PathName, uint8_t mode, uint16_t LenWrite, char *OutData);
uint16_t Sim800_FileREAD(const char *PathName, uint16_t Size, uint16_t Position, char *Storage);
uint8_t SIM800_EventIncoming (uint8_t *smsNum);
uint8_t SIM800_CLCC (sActiveCall *callparam, sSys_Param *WorkParam);
retcode_SIM800_SimpleCmd SIM800_ATA();
retcode_SIM800_SimpleCmd SIM800_ATH();
retcode_Event SIM800_EventsActive (sActiveCall *callparam);
uint8_t SIM800_GPRS_Connect(char *apn);
uint8_t SIM800_GPRS_Disconnect();
uint8_t SIM800_CNTP();
uint8_t SIM800_SMSsend(char *phonenum, char *body);
uint8_t SIM800_EMAILsend(sSys_Param *ParamBlock, char *Subj, char *Body, char *AttachName);
uint8_t SIM800_EMAILattachPacket(uint16_t LenPacket, char *Packet);
void SIM800_UnreadSMSRead();
uint8_t SIM800_DelReadedSms();
uint8_t SIM800_readSMSflow (char **Buff, char **PhoneNum);
void SIM800_DeleteReadedSMS();
void SIM800_PlayStart(char *amrname);
uint8_t SIM800_Call(char *phonenum);
uint8_t SIM800_CallHold();
uint8_t SIM800_CallsConf();
uint8_t Sim800_GetGSMsignal ();
uint8_t SIM800_PowerDown();
uint8_t SIM800_DisableRF();
uint8_t Sim800_FileRename(const char *OldName, const char *NewName);
uint8_t Sim800_FileDel(const char *name);

#endif
