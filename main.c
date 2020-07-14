//#define F_CPU 8000000UL
//#define STM32F10X_MD
#define KYIVSTAR
#include "stm32f10x.h"
#include "stm32F10x_gpioMY.h"
#include "stm32F10x_usart1.h"
#include "stm32f10x_dma_dihalt.h"
#include "SIM800Lib.h"
#include <string.h>
#include <stdlib.h>
#include "SenseButton.h"
#include "telemetry.h"
#include "time.h"
//-----------------------------------------ф
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define LEN_YYYYMMDD       10

static const sSys_Param DefParametr={
.PonyName="Sedlo1\0",
.SensorA_without=100, .SensorB_without=100,
.Sensordelta=8,
.FreeSensorTime=10,
.RegularReportTime= 1275, // 21-15
.AdminDTMFpassw="825*191\0",
.AdminPhoneNum_1="+380997199158\0",
.AdminPhoneNum_2="+380985743342\0",
.AdminPhoneNum_3="+380507388626\0",
#ifdef VODAFONE
.APNgprs="internet\0",
#endif
#ifdef KYIVSTAR
.APNgprs="www.ab.kyivstar.net\0",
#endif
#ifdef MTS_RU
.APNgprs="internet.mts.ru\0",
#endif
#ifdef MEGAFON
.APNgprs="internet\0",
#endif
.SMTPserv="smtp.gmail.com\0",
.SMTPname="loudponi@gmail.com\0", .SMTPpassw="Ntk_x300Z\0",
.EMAILdevice="loudponi@gmail.com\0",
.EmailHost="alferov.apple@icloud.com\0", .NameHost="Boss\0"
};

static const char masterPassword[]="0711*0625*7574*1024";
//admin menu 1*
static const char cmdDTMFadminReportSMS[]="1*0";    //запросить админ СМС
static const char cmdDTMFreplaceAdmin1[]="1*11";    //добавиться первым админом
static const char cmdDTMFreplaceAdmin2[]="1*12";    //добавиться вторым админом
static const char cmdDTMFreplaceAdmin3[]="1*13";    //добавиться третим админом
static const char cmdDTMFchangePassw[]="1*65535";   //запросс ищменения пароля
static const char cmdDTMFdefaultconfig[]="1*496";   //восстановить дефолтные настройки
static const char cmdDTMFdeleteAdmin1[]="1*91";     //удалить админ номер 1
static const char cmdDTMFdeleteAdmin2[]="1*92";     //удалить админ номер 2
static const char cmdDTMFdeleteAdmin3[]="1*93";     //удалить админ номер 3

static const char cmdDTMFadjEmpty[]="2*0";      //пустой вес

static const char cmdDTMFReportEMAIL[]="3*0";       //запросить внеплановый отчет на мейл
static const char cmdDTMFhardwareSendSMS[]="3*1";   //запросить Hardstate
static const char cmdDTMFYesterdayEMAIL[]="3*2";    //запросить вчерашний отчет на мейл

static const char cmdDTMFconferenc[]="8*";
static const char cmdDTMFgprsSetting[]="9*0";
static const char cmdDTMFhostSetting[]="9*1";

static const char cmdDTMFsaveExit[]="*0*0";

static const char playWHO_ARE_YOU[]="whoiyou.amr";
static const char playHI_SHIEF[]="hishief.amr";
static const char playPSW_CHANGED[]="pswchang.amr";
static const char playREPORT[]="report.amr";
static const char playWAIT_SMS[]="waitsms.amr";
static const char playWAIT_EMAIL[]="waiteml.amr";
static const char playNOW_YOU[]="nowyou.amr";
static const char playSHIEF1[]="sheif1.amr";
static const char playSHIEF2[]="sheif2.amr";
static const char playSHIEF3[]="sheif3.amr";
static const char playDELETED[]="deleted.amr";
static const char playYOU_ALREADY[]="aleready.amr";
static const char playINPUT_PSW[]="inputpsw.amr";
static const char playTUNE_GPRS[]="tunegprs.amr";
static const char playTUNE_EMAIL[]="tunemail.amr";
static const char playDefaultConfig[]="default.amr";
static const char playatSMS[]="atsms.amr";
static const char playatEmail[]="atemail.amr";
static const char playErrorAt[]="errat.amr";
static const char playGoodbuy[]="goodbuy.amr";
static const char playGPRSfault[]="gprsflt.amr";
static const char playBatteryLow[]="lowbat.amr";
static const char playTimeNosync[]="nosyncti.amr";
static const char playCALIBROK[]="calibrok.amr";
static const char playBADEMPTY[]="badempty.amr";
static const char playSENSORBROKEN[]="broksens.amr";
static const char playConferenc[]="confern.amr";

static const char strName[]="Name: ";
static const char strPassword[]="\r\nPassword: ";
static const char strAdm1[]="\r\nAdmin1: ";
static const char strAdm2[]="\r\nAdmin2: ";
static const char strAdm3[]="\r\nAdmin3: ";
static const char Subj_Report[]="Daily report ";
static const char Body_Report_Empty[]="Horse deaktive today";
static const char Body_Report_Y_Empty[]="Horse deaktive yesterday";

static const char Body_Report_Active[]="See report about activity in attached file";

static const char APNheader[]="<apn>";
static const char EmailHostheader[]="<host>";
static const char HorseNameheader[]="<nick>";
static const char SensetivHeader[]="<sens>";
static const char ReportTimeHeader[]="<report>";
static const char FreeTimeHeader[]="<free>";
static const char BatteryCapHeader[]="\r\nBattery state = ";
static const char BatteryCapSuff[]="%\r\n";
static const char GSMsignalHeader[]="GSM signal = ";
static const char SensorsAdjA[]="Sensors adj:\r\nA = ";
static const char SensorsAdjB[]="\r\nB = ";
static const char SensorDelta[]="\r\nDelta = ";
static const char SensorFree[]="\r\nFree Time = ";
static const char SensoBroken[]="!! SENSOR BROKEN: ";
static const char SyncNTP[]="NTP time: ";
static const char SyncGSM[]="GSM time: ";

timestruct DTfromSIM;
sSys_Param SysParam;
uint8_t flagSysParamRead;
sActiveCall CallParam;
uint8_t smsNum;
char lastPhone[16];
uint16_t smaValueA, smaValueB;
uint16_t deviceFlags;

/*----------------------------------------------------------
This procedure recognize Guest or Admin called
if Guest - wait Password 20 sec, and go to admin mode
if Admin - find and decode DTMF command
return   0 - had Guest mode
         1 - had Admin mode
*/

uint8_t Discuss(uint16_t *DiscussFlag)
{
uint32_t CalltimeExpiration;
uint8_t  fWaitNewPassword = 0;
uint8_t fWaitConferenc = 0;
sSys_Param tempSysParam;
retcode_Event AnswDTMF;
  *DiscussFlag &= ~FPENDING_SAVE_CONFIG;
  memcpy (&tempSysParam, &SysParam, sizeof(sSys_Param));    //make copy of SysParam
  CallParam.headDTMF=0;   //clear DTMF buffer
  CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
  if (CallParam.AdminNum>0)       //admin Called ?
  {
   lAdminMode:
   fWaitNewPassword=0;
   CalltimeExpiration = xTaskGetTickCount()+30000;  //2 min max duration this call
   SIM800_PlayStart((char *)playHI_SHIEF); //say promt "Привет шеф"
   while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   // голосовое информирование о неполадках
   if ((*DiscussFlag & (FPENDING_SMS_ADMIN | FPENDING_SMS_HARDSTATE | FPENDING_SMS_GPRS_SETTING | FPENDING_SMS_HOST_SETTING)) >0)
   {
    SIM800_PlayStart((char *)playErrorAt);   //Ошибки при отправке..."  
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
    SIM800_PlayStart((char *)playatSMS);   //...SMS"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   
   if ((*DiscussFlag & (FPENDING_EMAIL_REPORT | REGULAR_EMAIL_REP_REQEST)) >0)
   {
    SIM800_PlayStart((char *)playErrorAt);   //Ошибки при отправке..."  
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
    SIM800_PlayStart((char *)playatEmail);   //...EMAIL"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   
   if ((*DiscussFlag & GPRS_CONNECT_ERROR) >0)
   {
    SIM800_PlayStart((char *)playGPRSfault);   //GPRS не работает"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   
   if ((*DiscussFlag & LOW_BATTERY_ENAERGY) >0)
   {
    SIM800_PlayStart((char *)playBatteryLow);   //батарея разряжена"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   
   if ((*DiscussFlag & BROKEN_SENSOR) >0)
   {
    SIM800_PlayStart((char *)playSENSORBROKEN);   //сенсор веса поврежден"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   
   if ((*DiscussFlag & TIME_SYNC_ERROR) >0)
   {
    SIM800_PlayStart((char *)playTimeNosync);   //время не синхронизировано"
    while (xTaskGetTickCount()<CalltimeExpiration)
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF == EVENT_CREC0)  break;
      else if (AnswDTMF == EVENT_NOCARRIER) goto lExitNoSave;
    }
   }
   // цикл опроса DTMF
   do
    {
     AnswDTMF=SIM800_EventsActive (&CallParam);
     if (AnswDTMF==EVENT_DTMF)  //DTMF code incoming ?
     {
       if (fWaitNewPassword==1)     //wait input new DTMF password ?
       {
         if (CallParam.headDTMF==7)     //DTMF password must be long 7 characters ?
         {
          memcpy(tempSysParam.AdminDTMFpassw,CallParam.AccumDTMF,8);
          CallParam.headDTMF=0;   //clear DTMF buffer
          CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
          *DiscussFlag |= FPENDING_SAVE_CONFIG;
          *DiscussFlag |= FPENDING_SMS_ADMIN;
          fWaitNewPassword=0;
          SIM800_PlayStart((char *)playPSW_CHANGED);   //say "пароль изменен"
         }
         continue;
       }
       
       if (fWaitConferenc == 1)     //wait input phone num to Conference ?
       {
         if ((CallParam.headDTMF >0) && (CallParam.AccumDTMF[CallParam.headDTMF-1] == '*'))    //ввод номера закончен и введена звездочка
         {
          CallParam.AccumDTMF[CallParam.headDTMF-1] = 0x00;
          if (SIM800_Call(CallParam.AccumDTMF) == 0)
            if (SIM800_CallHold() == SIMPLE_CMD_SUCCESS)
              if (SIM800_CallsConf() == SIMPLE_CMD_SUCCESS)
              {
                CalltimeExpiration = xTaskGetTickCount() + 100000; //400sec for Call
                while (xTaskGetTickCount() < CalltimeExpiration) 
                  {
                  AnswDTMF=SIM800_EventsActive (&CallParam);
                  if (AnswDTMF == EVENT_NOCARRIER) break;
                  }
              }
           goto lExitNoSave;
         }
        continue;
       }
       
   // admin Report by SMS    
       if (strcmp(CallParam.AccumDTMF,cmdDTMFadminReportSMS)==0) 
       {
         *DiscussFlag |= FPENDING_SMS_ADMIN;
lReportSms:
         SIM800_PlayStart((char *)playREPORT);   //say "Отчет ..."
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
          SIM800_PlayStart((char *)playWAIT_SMS); //say "...ждите в SMS"                        
  lclearDTMFbuff:
         CallParam.headDTMF=0;   //clear DTMF buffer
         CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
         continue;
       }
     // Attached admin #1  
       if (strcmp(CallParam.AccumDTMF,cmdDTMFreplaceAdmin1)==0)
       {
         if (CallParam.AdminNum>0)  goto  lAlereadyExist;
        SIM800_PlayStart((char *)playNOW_YOU);   //say "теперь Вы.."
        while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF1); //say "...первый шеф"                          
        strcpy(tempSysParam.AdminPhoneNum_1,CallParam.PhoneNum);
        CallParam.AdminNum=1;
  lPhoneNumsRewrite:
        *DiscussFlag |= FPENDING_SAVE_CONFIG;
        goto lclearDTMFbuff;
       }
    // Attached admin #2    
       if (strcmp(CallParam.AccumDTMF,cmdDTMFreplaceAdmin2)==0)
       {
         if (CallParam.AdminNum>0)  goto  lAlereadyExist;
        SIM800_PlayStart((char *)playNOW_YOU);   //say "теперь Вы.."
        while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF2); //say "...Второй шеф"
        CallParam.AdminNum=2;
        strcpy(tempSysParam.AdminPhoneNum_2,CallParam.PhoneNum);
        goto lPhoneNumsRewrite;
       }
    // Attached admin #3   
       if (strcmp(CallParam.AccumDTMF,cmdDTMFreplaceAdmin3)==0)
       {
          if (CallParam.AdminNum>0)  goto  lAlereadyExist;
          SIM800_PlayStart((char *)playNOW_YOU);   //say "теперь Вы.."
          while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF3); //say "...третий шеф"
        CallParam.AdminNum=3;
        strcpy(tempSysParam.AdminPhoneNum_3,CallParam.PhoneNum);
        goto lPhoneNumsRewrite;   
  // amin Phone Number aleready is in AdminList 
  lAlereadyExist:
          SIM800_PlayStart((char *)playYOU_ALREADY);   //say "Вы и так в списке"
          goto  lclearDTMFbuff;
       }
    // prompt to Enter New password   
        if (strcmp(CallParam.AccumDTMF,cmdDTMFchangePassw)==0)
       {
         CallParam.headDTMF=0;   //clear DTMF buffer
         CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
         SIM800_PlayStart((char *)playINPUT_PSW);    //say "Вводите новый пароль"
         fWaitNewPassword=1;
         continue;
       }
   // prompt to conference   
        if (strcmp(CallParam.AccumDTMF,cmdDTMFconferenc)==0)
       {
         CallParam.headDTMF=0;   //clear DTMF buffer
         CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
         SIM800_PlayStart((char *)playConferenc);    //say "введите номер для конференции и *"
         fWaitConferenc=1;
         continue;
       }    
       
        // default config load   
       if (strcmp(CallParam.AccumDTMF,cmdDTMFdefaultconfig) == 0)
       {
        SIM800_PlayStart((char *)playDefaultConfig); //say "Настройки по умолчанию восстановлены"
        memcpy(&tempSysParam,&DefParametr,sizeof(sSys_Param));
        goto  lPhoneNumsRewrite;
       }
       
     // deleted admin phone Number #1  
       if (strcmp(CallParam.AccumDTMF,cmdDTMFdeleteAdmin1)==0)
       {
          SIM800_PlayStart((char *)playDELETED);   //say "Deleted .."
          while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF1); //say "...first Shief"
         *tempSysParam.AdminPhoneNum_1=0x00;
          goto lPhoneNumsRewrite;
       }
     // deleted admin phone Number #2  
       if (strcmp(CallParam.AccumDTMF,cmdDTMFdeleteAdmin2)==0)
       {
         SIM800_PlayStart((char *)playDELETED);   //say "Deleted .."
          while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF2); //say "...second Shief"
         *tempSysParam.AdminPhoneNum_2=0x00;
         goto lPhoneNumsRewrite;
       }
       
    // deleted admin phone Number #3   
       if (strcmp(CallParam.AccumDTMF,cmdDTMFdeleteAdmin3)==0)
       {
         SIM800_PlayStart((char *)playDELETED);   //say "Deleted .."
          while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
        SIM800_PlayStart((char *)playSHIEF3); //say "...third Shief"
         *tempSysParam.AdminPhoneNum_3 = 0x00;
         goto lPhoneNumsRewrite;
       }
       
       //сохранение значения пустовго веса
       if (strcmp(CallParam.AccumDTMF,cmdDTMFadjEmpty)==0) 
       {
         if ((smaValueA > SENSOR_A_MIN_VALUE + 2*SysParam.Sensordelta) && (smaValueA < SENSOR_A_MAX_VALUE) &&
             (smaValueB > SENSOR_B_MIN_VALUE + 2*SysParam.Sensordelta) && (smaValueB < SENSOR_B_MAX_VALUE))
         {
           tempSysParam.SensorA_without = smaValueA;
           tempSysParam.SensorB_without = smaValueB;
           SIM800_PlayStart((char *)playCALIBROK); //say "калибровка веса выполнена"
           *DiscussFlag |= PENDING_SENSOR_INFO;
           goto lPhoneNumsRewrite;
         }
         else
           SIM800_PlayStart((char *)playBADEMPTY); //say "пустой вес не принят"
         *DiscussFlag |= PENDING_SENSOR_INFO;
         goto lclearDTMFbuff;
       }
       
     // request to send SMS with Battery state  
        if (strcmp(CallParam.AccumDTMF,cmdDTMFhardwareSendSMS)==0) 
       {  
         *DiscussFlag |= FPENDING_SMS_HARDSTATE;
         goto lReportSms;
       }

      // request to send SMS with setting GPRS
       if (strcmp (CallParam.AccumDTMF, cmdDTMFgprsSetting)==0) 
       {
         SIM800_PlayStart((char *)playTUNE_GPRS);   //say "Setting GPRS..." 
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
          SIM800_PlayStart((char *)playWAIT_SMS); //say "...wait in SMS" 
         *DiscussFlag |= FPENDING_SMS_GPRS_SETTING;
         goto lclearDTMFbuff;
       }
      // request to send SMS with HOST setting 
       if (strcmp (CallParam.AccumDTMF, cmdDTMFhostSetting)==0)
       {
         SIM800_PlayStart((char *)playTUNE_EMAIL);   //say "Setting EMAIL..." 
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
          SIM800_PlayStart((char *)playWAIT_SMS); //say "...wait in SMS" 
         *DiscussFlag |= FPENDING_SMS_HOST_SETTING;
         goto lclearDTMFbuff;
       }
       
      // request to send EMAIL with report Horse
       if (strcmp(CallParam.AccumDTMF,cmdDTMFReportEMAIL)==0) 
       {
         SIM800_PlayStart((char *)playREPORT);   //say "Report..."
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
          SIM800_PlayStart((char *)playWAIT_EMAIL); //say "...wait in EMAIL"     
         *DiscussFlag |= FPENDING_EMAIL_REPORT;
         goto lclearDTMFbuff;
       }
       
       // request to send EMAIL with Yesterday report Horse
       if (strcmp(CallParam.AccumDTMF,cmdDTMFYesterdayEMAIL)==0) 
       {
         SIM800_PlayStart((char *)playREPORT);   //say "Report..."
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if (AnswDTMF==EVENT_CREC0)  break;
            else if (AnswDTMF==EVENT_NOCARRIER) goto lExitNoSave;
          }
          SIM800_PlayStart((char *)playWAIT_EMAIL); //say "...wait in EMAIL"     
         *DiscussFlag |= FPENDING_EMAIL_YESTERDAY_REPORT;
         goto lclearDTMFbuff;
       }
       
      // Exit with save changes
       if (strcmp(CallParam.AccumDTMF,cmdDTMFsaveExit)==0)
       {
         SIM800_PlayStart((char *)playGoodbuy);   //say "Изменения сохранены, до свидания !"
         memcpy (&SysParam, &tempSysParam, sizeof(sSys_Param));    //save change from temp structure to main structure
         while (xTaskGetTickCount()<CalltimeExpiration)
          {
            AnswDTMF=SIM800_EventsActive (&CallParam);
            if ((AnswDTMF==EVENT_CREC0) || (AnswDTMF==EVENT_NOCARRIER)) 
              break;
          }
         return (1);
       }
     }
     else if (AnswDTMF == EVENT_NOCARRIER) break;
    } while (xTaskGetTickCount() < CalltimeExpiration);
  lExitNoSave:
    *DiscussFlag &= ~FPENDING_SAVE_CONFIG;
    return (1);
  }
  
  else //Guest mode
  {
    SIM800_PlayStart((char *)playWHO_ARE_YOU); 
    CalltimeExpiration = xTaskGetTickCount()+7500;  //30 sec
    do
    {
      AnswDTMF=SIM800_EventsActive (&CallParam);
      if (AnswDTMF==EVENT_DTMF)  //DTMF code incoming
      {
        if ((strcmp(CallParam.AccumDTMF,tempSysParam.AdminDTMFpassw)==0) | (strcmp(CallParam.AccumDTMF,masterPassword)==0))
        {  
          CallParam.headDTMF=0;   //clear DTMF buffer
          CallParam.AccumDTMF[CallParam.headDTMF]=0x00;
          goto lAdminMode ;
        }
      }
      else if (AnswDTMF==EVENT_NOCARRIER) break;
    } while (xTaskGetTickCount()<CalltimeExpiration);
  }
  return (0);
}


/*--------------------------------------------------------------------------------------------------*/
void vSIM800 (void *pvParameters)
{
uint8_t trycnt;
uint8_t bat;
NetworkRegType HowReg;
time_t    NIXtime;
uint8_t FileXistRes;
uint8_t ArrayLogTail = 0;   //хвост в массиве лога нагрузки
uint8_t tryTimeReq = 0;
const static char CONFIG_INI[]="CONF.INI";    //config FileName
const static char LOG_NAME_TD[]="TODAY.REP";
const static char LOG_NAME_YTD[]="YESRTDAY.REP";
const static  char csvExtended[]=".csv";
deviceFlags = 0;
Full_Reset_SIMmodule:
  Sim800_Prepare();
  if (Sim800_GetBattery () < MIN_BATTERY)   ParkDevice();
  
  //----    10 try for registration in GSM
  for (trycnt=10; trycnt>0; trycnt--)
  {
    HowReg=Sim800_CheckReg();
    if ((HowReg==HOME_NET) | (HowReg==ROAMING)) break;
    vTaskDelay(1200);   //pause 5 sec
  }
  if (trycnt==0)  goto Full_Reset_SIMmodule; 
  SIM800_DeleteReadedSMS();
  
 //Getting or Default configuration 
lCheckConfigFile:
  FileXistRes = Sim800_FileExist (CONFIG_INI);
  switch (FileXistRes)
  {
    case 0:
      if (Sim800_FileCreate(CONFIG_INI)!=1)
        goto Full_Reset_SIMmodule;
      goto lCheckConfigFile;
      
    case 1:
      for (trycnt=3;trycnt>0;trycnt--)
        if (Sim800_FileREAD(CONFIG_INI, sizeof(SysParam), 0,((char *)&SysParam)) == sizeof(SysParam)) 
          if (CRCcalc( &SysParam.PonyName, sizeof(SysParam)-sizeof(SysParam.strCRC)) == SysParam.strCRC) 
            goto lConfigFileReadOK;   //CRC is OK
        // Parametr file lost or corrupted
        memcpy (&SysParam, &DefParametr, sizeof(SysParam));    //Load default setting
        SysParam.strCRC = CRCcalc (&SysParam.PonyName, sizeof(SysParam)-sizeof(SysParam.strCRC));
        //save in FileSystem SIM800
        trycnt=3;
        while ((Sim800_FileWrite(CONFIG_INI,0,sizeof(SysParam),((char *)&SysParam)) != FILE_WRITE_SUCCESS) & (--trycnt>0))
          vTaskDelay(1000);   //pause 4sec
         if (trycnt == 0) goto Full_Reset_SIMmodule;
        goto lCheckConfigFile;
      
    default: //retcode Sim800_FileExist = 255
      vTaskDelay(1000);   //pause 4sec
      if (Sim800_FileExist (CONFIG_INI) == 255) goto Full_Reset_SIMmodule;
      goto lCheckConfigFile;
  }      //vSIM800
lConfigFileReadOK:   
   flagSysParamRead = 1;
   
  //-------  getting Date and Time
lSyncTime:
  NIXtime=Sim800_GetTime ();  //Read DateTime from GSM
  if (NIXtime < UNIX_DATA_2020)
  {
    if (++tryTimeReq < TRY_SYN_GSM_TIME )    goto Full_Reset_SIMmodule;
    
    if (SIM800_GPRS_Connect (SysParam.APNgprs) == 0) 
    {
      deviceFlags &= ~GPRS_CONNECT_ERROR;
      SIM800_CNTP();
      NIXtime=Sim800_GetTime ();  //Read DateTime
      if (NIXtime < UNIX_DATA_2020)
        deviceFlags |= TIME_SYNC_ERROR;
      else  deviceFlags &= ~TIME_SYNC_ERROR;
    }
    else
    {
      deviceFlags |= GPRS_CONNECT_ERROR;
      deviceFlags |= TIME_SYNC_ERROR;
    }
    SIM800_GPRS_Disconnect();
  }
  RTC_SetCounter (NIXtime);
  
  //allowed Calls
  if (Sim800_GsmBusy(0) !=  SIMPLE_CMD_SUCCESS) goto Full_Reset_SIMmodule;
 
  //------------------------------------------------------ main cycle wait
  uint16_t servicePeriod;
  while (1)
  {
    SIM800_Sleep(1); //sleep
    for(servicePeriod=10000; servicePeriod>0; servicePeriod-- )   //200 sec
    {
      vTaskDelay(5); //20msec
      uint8_t whatEvent = SIM800_EventIncoming(&smsNum);
      if (whatEvent == 1)    //have RING
      {
        lcmdATA:
        SIM800_Sleep(0);  //wake up
        vTaskDelay(500);
        SIM800_ATA();
        vTaskDelay(250);
        trycnt=10;
        while ((SIM800_CLCC (&CallParam, &SysParam) == 255) && (--trycnt>0))
          vTaskDelay(125);
        if (trycnt==0)         goto Full_Reset_SIMmodule;
        if (CallParam.Status == STATUS_DIR_ACTIVE_IN)
        {
          if (Discuss(&deviceFlags) == 1)
            strcpy(lastPhone,(const char *)&CallParam.PhoneNum) ;
          else *lastPhone = 0x00;  
        }  
        else if (CallParam.Status == STATUS_DIR_INCOMING)
                goto lcmdATA;
        SIM800_ATH();
        vTaskDelay(250);
        SIM800_ATH();
        break;  //to service area
      }//if (whatEvent == 1)
      else if (whatEvent == 2) break; //have SMS
      else if (((deviceFlags & BROKEN_SENSOR) >0) && ((deviceFlags & BROKEN_SENSOR_ALARMED) == 0)) break;    //broken Sensor
    }// for(
    
    //-----------  service Area (read and send SMS EMAIl etc)---------------
    SIM800_Sleep(0); //wake up
    if (Sim800_GsmBusy(1) !=  SIMPLE_CMD_SUCCESS) goto Full_Reset_SIMmodule;
    
    //СМС оповещение о сломанном датчике веса
    if (((deviceFlags & BROKEN_SENSOR) >0) && ((deviceFlags & BROKEN_SENSOR_ALARMED) ==0))
     {
      char smsBody[64];
      char *ptr = smsBody;
      ptr = stradd(ptr,SensoBroken); 
      ptr = itoa(smaValueA, ptr);
      *(ptr++)='&';
      ptr = itoa(smaValueB, ptr);
      deviceFlags |= BROKEN_SENSOR_ALARMED;
      if (SIM800_SMSsend(SysParam.AdminPhoneNum_1, smsBody) == 0)
        deviceFlags |= BROKEN_SENSOR_ALARMED;
     }
    //наблюдение за зарядом батареи
    uint8_t Oldbat = bat;
    bat = Sim800_GetBattery ();
    if (bat == 255) bat = Oldbat;
    if (bat < MIN_BATTERY)
      ParkDevice();
        
    if (bat < CRITICAL_BATTERY)
     deviceFlags |= LOW_BATTERY_ENAERGY;
    else
     deviceFlags &= ~LOW_BATTERY_ENAERGY;
     
     if ((deviceFlags & FPENDING_SMS_ADMIN) >0)
     {
      char smsBody[160];
      char *ptr = smsBody;
      ptr = stradd(ptr,strName);
      ptr = stradd(ptr,(const char *)&SysParam.PonyName);
      ptr = stradd(ptr,strPassword);
      ptr = stradd(ptr,(const char *)&SysParam.AdminDTMFpassw);
      ptr = stradd(ptr,strAdm1);
      ptr = stradd(ptr,(const char *)&SysParam.AdminPhoneNum_1);
      ptr = stradd(ptr,strAdm2);
      ptr = stradd(ptr,(const char *)&SysParam.AdminPhoneNum_2);
      ptr = stradd(ptr,strAdm3);
      ptr = stradd(ptr,(const char *)&SysParam.AdminPhoneNum_3);
      if (SIM800_SMSsend(lastPhone, smsBody) == 0)
        deviceFlags &= ~FPENDING_SMS_ADMIN;
     }
     
     if ((deviceFlags & FPENDING_SMS_GPRS_SETTING) >0)
     {
      char smsBody[160];
      char *ptr = smsBody;
      ptr = stradd(ptr,APNheader);
      *(ptr++)='\r';    *(ptr++)='\n';
      ptr = stradd(ptr,(const char *)&SysParam.APNgprs);
      if (SIM800_SMSsend(lastPhone, smsBody) == 0)
        deviceFlags &= ~FPENDING_SMS_GPRS_SETTING;
     }
     
      if ((deviceFlags & FPENDING_SMS_HOST_SETTING) >0)
     {
      char smsBody[160];
      char *ptr = smsBody;
      ptr = stradd(ptr,EmailHostheader);
      *(ptr++)='\r';    *(ptr++)='\n';
      ptr = stradd(ptr,(const char *)&SysParam.EmailHost);
      if (SIM800_SMSsend(lastPhone, smsBody) == 0)
        deviceFlags &= ~FPENDING_SMS_HOST_SETTING;
     }
     
     if ((deviceFlags & FPENDING_SMS_HARDSTATE) >0)
     {
      char smsBody[64];
      char *ptr = smsBody;
      struct tm * pStrTime;
      time_t nowRTC = RTC_GetCounter();
      const static char formatDDMMYYHHMM[]="%d/%m/%Y %H:%M";
      
      pStrTime = gmtime(&nowRTC);         //get time now
      if (tryTimeReq >= TRY_SYN_GSM_TIME)
        ptr = stradd(ptr,SyncNTP);
      else
        ptr = stradd(ptr,SyncGSM);
      ptr += strftime (ptr,60,formatDDMMYYHHMM,pStrTime);    
      ptr = stradd(ptr,BatteryCapHeader);
      ptr = itoa((uint16_t)bat, ptr);
      ptr = stradd(ptr,BatteryCapSuff);
      uint8_t signal = Sim800_GetGSMsignal();
      if ((signal > 0) && (signal != 255))
      {
       ptr = stradd(ptr,GSMsignalHeader );
       ptr = itoa((uint16_t)signal, ptr); 
      }
      if (SIM800_SMSsend(lastPhone, smsBody) == 0)
        deviceFlags &= ~FPENDING_SMS_HARDSTATE;
     }
     
      if ((deviceFlags & PENDING_SENSOR_INFO) >0)
     {
      char smsBody[64];
      char *ptr = smsBody;
      ptr = stradd(ptr,SensorsAdjA);
      ptr = itoa(SysParam.SensorA_without, ptr);
      ptr = stradd(ptr,SensorsAdjB);
      ptr = itoa(SysParam.SensorB_without, ptr);
      ptr = stradd(ptr,SensorDelta);
      ptr = itoa(SysParam.Sensordelta, ptr);
      ptr = stradd(ptr,SensorFree);
      ptr = itoa(SysParam.FreeSensorTime, ptr);
      if (SIM800_SMSsend(lastPhone, smsBody) == 0)
        deviceFlags &= ~PENDING_SENSOR_INFO;
     }
     
  // обработка входящих SMS
     SIM800_UnreadSMSRead();    //run ALL sms read in flow mode
     uint8_t smsFromAdminFlag=0;
     uint8_t smsHeaderFlag = NONE_HEADER_FLAG;
     uint8_t haveUnreadSmsFlag=0;
     while (1)
     {
      char *ptrIncoming = NULL;
      char *pSmsPhoneNum;
      uint8_t rescode = SIM800_readSMSflow (&ptrIncoming, &pSmsPhoneNum);
      //removeSpaces(ptrIncoming);
      char *ptr0D0A = strchr(ptrIncoming,0x0D);
      if (ptr0D0A !=NULL) *ptr0D0A= 0x00;
      ptr0D0A = strchr(ptrIncoming,0x0A);
      if (ptr0D0A !=NULL) *ptr0D0A = 0x00;
      if (rescode == 0)
      {
        smsFromAdminFlag=0;
        haveUnreadSmsFlag=1;
        smsHeaderFlag = NONE_HEADER_FLAG;
        if ((strcmp(pSmsPhoneNum,SysParam.AdminPhoneNum_1) == 0) || (strcmp(pSmsPhoneNum,SysParam.AdminPhoneNum_2) == 0) || (strcmp(pSmsPhoneNum,SysParam.AdminPhoneNum_3) == 0))
          smsFromAdminFlag=1;
        continue;
      }
      else if (rescode == 3) 
      {
        if (haveUnreadSmsFlag == 1) SIM800_DeleteReadedSMS(); //delete readed sms 
        break;
      }
      else if ((rescode == 1) || (smsFromAdminFlag != 1))   continue;
      else if (strcmp(ptrIncoming,APNheader) == 0)          smsHeaderFlag = APN_HEADER_FLAG;
      else if (strcmp(ptrIncoming,EmailHostheader) == 0)    smsHeaderFlag = EMAILHOST_HEADER_FLAG;
      else if (strcmp(ptrIncoming,HorseNameheader) == 0)    smsHeaderFlag = HORSENAME_HEADER_FLAG;
      else if (strcmp(ptrIncoming,SensetivHeader) == 0)     smsHeaderFlag = SENSETIV_HEADER_FLAG;
      else if (strcmp(ptrIncoming,ReportTimeHeader) == 0)   smsHeaderFlag = REPORT_TIME_HEADER;
      else if (strcmp(ptrIncoming,FreeTimeHeader) == 0)     smsHeaderFlag = FREE_TIME_HEADER;
      
      else if ((smsHeaderFlag == APN_HEADER_FLAG) && (strlen(ptrIncoming) < sizeof (SysParam.APNgprs)))
      {
        strcpy(SysParam.APNgprs,ptrIncoming);
        deviceFlags |= FPENDING_SAVE_CONFIG;
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
      else if ((smsHeaderFlag == EMAILHOST_HEADER_FLAG) && (strlen(ptrIncoming) < sizeof (SysParam.EmailHost)))
      {
        strcpy(SysParam.EmailHost,ptrIncoming);
        deviceFlags |= FPENDING_SAVE_CONFIG;
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
      else if ((smsHeaderFlag == HORSENAME_HEADER_FLAG) && (strlen(ptrIncoming) < sizeof (SysParam.PonyName)))
      {
        strcpy(SysParam.PonyName,ptrIncoming);
        deviceFlags |= FPENDING_SAVE_CONFIG;
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
      else if (smsHeaderFlag == SENSETIV_HEADER_FLAG)
      {
        uint16_t delta = atoi(ptrIncoming);
        if ((delta > 2) && (delta < 30))
        {
          SysParam.Sensordelta = (uint8_t)delta;
          deviceFlags |= FPENDING_SAVE_CONFIG;
        }
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
      
      else if (smsHeaderFlag == REPORT_TIME_HEADER)
      {
        uint16_t hour = atoi(ptrIncoming);
        if (hour < 24)
        {
          char *ptrsep = strchr(ptrIncoming,'-');
          if (ptrsep != NULL)
          {
            uint16_t minute = atoi(++ptrsep);
            if (minute < 60)
            {
              SysParam.RegularReportTime = 60*hour + minute;
              deviceFlags |= FPENDING_SAVE_CONFIG;
            }
          }
        }
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
      
      else if (smsHeaderFlag == FREE_TIME_HEADER)
      {
        uint16_t pause = atoi(ptrIncoming);
        if ((pause > 3) && (pause < 50))
        {
          SysParam.FreeSensorTime = (uint8_t)pause;
          deviceFlags |= FPENDING_SAVE_CONFIG;
        }
        smsHeaderFlag = NONE_HEADER_FLAG;
      }
     } //while (1)
    
    //сохранения лога нагрузки на седло в файл SIM800 
     uint8_t dupArrayLogHead = ArrayLog_GetHead();
     time_t nowRTC = RTC_GetCounter();
     if (ArrayLogTail != dupArrayLogHead)
     {
       char tempBuf[512];
       char *pt = tempBuf;
       struct tm * pStrTime;
       const static char row1report[]="%d/%m/%Y,%H:%M,%A,- Create\r\n,";
       const static char row2report[]="\r\nData,Time,Duration (min), sec";
       const static char cols12report[]="\r\n%d/%m/%Y,%H:%M:%S,";
       uint8_t dupArrayLogTail = ArrayLogTail;
       uint16_t sizeTodayFile=0;
       int8_t fullness = dupArrayLogHead - ArrayLogTail;
       if (fullness < 0) fullness += LOG_SIZE;
       if (Sim800_FileExist(LOG_NAME_TD) != 1)
         if (Sim800_FileCreate(LOG_NAME_TD) !=1)
           goto lLogFail;
       if (Sim800_FileSize(LOG_NAME_TD, &sizeTodayFile) != 1)
          goto lLogFail;
       if (sizeTodayFile == 0)
        {
         pStrTime = gmtime(&nowRTC);         //make Title
         pt += strftime (pt,60,row1report,pStrTime);    
         pt = stradd(pt,row2report);         //make head row
        }
        else if (sizeTodayFile > 10000) //file almost Full
        {
          deviceFlags |= REGULAR_EMAIL_REP_REQEST; //emergency send almost Full report File
          goto lLogFail;
        }
       while (ArrayLogTail != dupArrayLogHead)
       {
        sLogElem * ptCurrLogNum = ArrayLog_GetRecord(ArrayLogTail);
        pStrTime = gmtime((time_t *)&(ptCurrLogNum->unixTime));
        pt += strftime (pt,60,cols12report,pStrTime);
        pt = itoa(ptCurrLogNum->duration / 60, pt);
        *pt++ = ',';
        pt = itoa(ptCurrLogNum->duration % 60, pt);
        if (++ArrayLogTail >= LOG_SIZE)
          ArrayLogTail -= LOG_SIZE;
       }
       *pt = 0x00;
       if (Sim800_FileWrite(LOG_NAME_TD, 1, strlen(tempBuf), tempBuf) != FILE_WRITE_SUCCESS)
         ArrayLogTail = dupArrayLogTail;    //если неудачная запись в файл то хвост не перемещаем
     }
  lLogFail:
//----------------------------------------------------------
  
   //создание задания для отсылки EMAIL отчета по расписанию
   if ((nowRTC % 86400)/60 > SysParam.RegularReportTime)
   {
    if ((deviceFlags & REGULAR_EMAIL_REP_DONE) == 0)
      deviceFlags |= REGULAR_EMAIL_REP_REQEST;  
   }
   else deviceFlags &= ~REGULAR_EMAIL_REP_DONE;
//----------------------------------------------------------- 
     
   //Отсылка EMAIL отчета по расписанию
   if (((deviceFlags & REGULAR_EMAIL_REP_REQEST) >0) || ((deviceFlags & FPENDING_EMAIL_REPORT) >0))
   {
    static const char regularAttName_PREF[]="Regular Report ";
    //static const char solicitedAttName_PREF[]="Solicited Report ";
    char tempBuf[128];
    char *pt = tempBuf;
    uint16_t sizeTodayFile=0;
    if (Sim800_FileSize(LOG_NAME_TD, &sizeTodayFile) == 1)
    {
     if (sizeTodayFile > LEN_YYYYMMDD)
     {
       pt = stradd(pt,regularAttName_PREF);
       if (Sim800_FileREAD(LOG_NAME_TD, LEN_YYYYMMDD, 0, pt) == LEN_YYYYMMDD)  //LOG_NAME_TD contain datetime
       {
        pt += LEN_YYYYMMDD;
        pt = stradd(pt,csvExtended);
        strrepl(tempBuf, '/', '_');
        strrepl(tempBuf, ';', '_');
        strrepl(tempBuf, ':', '_');
        if (SIM800_EMAILsend(&SysParam, (char *)Subj_Report,  (char *)Body_Report_Active, tempBuf) == 0)
         {
           uint16_t sendedSize = 0;
           uint16_t packetSize;
           while (sendedSize < sizeTodayFile)
            { 
              packetSize = sizeTodayFile - sendedSize;
              if (packetSize > 128) packetSize = 128;
              if  (Sim800_FileREAD (LOG_NAME_TD, packetSize, sendedSize, tempBuf) != packetSize)
                break; 
              if (SIM800_EMAILattachPacket(packetSize, tempBuf) == 1)
                break;
              sendedSize +=  packetSize;
            } //  while
           if ((SIM800_EMAILattachPacket(0, tempBuf) == 0) && (sendedSize == sizeTodayFile))
             {
              if ((deviceFlags & REGULAR_EMAIL_REP_REQEST) >0)
              {
                Sim800_FileDel(LOG_NAME_YTD);
                Sim800_FileRename(LOG_NAME_TD, LOG_NAME_YTD);
                Sim800_FileCreate(LOG_NAME_TD);
              }
              deviceFlags &= ~REGULAR_EMAIL_REP_REQEST; //successfull attached and send csv with Email body
              deviceFlags |= REGULAR_EMAIL_REP_DONE;
              deviceFlags &= ~FPENDING_EMAIL_REPORT;
              deviceFlags &= ~GPRS_CONNECT_ERROR;
             }
         }//if (SIM800_EMAILsend 
       } //if (Sim800_FileREAD(LOG_NAME_TD
     } // if (sizeTodayFile > LEN_YYYYMMDD)
    
    else //email without attachement
    {
      if (SIM800_EMAILsend(&SysParam, (char *)Subj_Report,  (char *)Body_Report_Empty, NULL) == 0) 
      {
        deviceFlags &= ~REGULAR_EMAIL_REP_REQEST;
        deviceFlags |= REGULAR_EMAIL_REP_DONE;
        deviceFlags &= ~FPENDING_EMAIL_REPORT;
        deviceFlags &= ~GPRS_CONNECT_ERROR;
      }
    }
    SIM800_GPRS_Disconnect();
   }
   else Sim800_FileCreate(LOG_NAME_TD);
   }
//---------------------------------------------------
   
   
   //Отсылка вчерашнего EMAIL отчета по разапросу
   if ((deviceFlags & FPENDING_EMAIL_YESTERDAY_REPORT) >0)
   {
    static const char regularAttName_PREF[]="Yesterday Report ";
    char tempBuf[128];
    char *pt = tempBuf;
    uint16_t sizeFile=0;
    if (Sim800_FileSize(LOG_NAME_YTD, &sizeFile) == 1)
    {
     if (sizeFile > LEN_YYYYMMDD)
     {
       pt = stradd(pt,regularAttName_PREF);
       if (Sim800_FileREAD(LOG_NAME_YTD, LEN_YYYYMMDD, 0, pt) == LEN_YYYYMMDD)  //LOG_NAME_YTD contain datetime
       {
        pt += LEN_YYYYMMDD;
        pt = stradd(pt,csvExtended);
        strrepl(tempBuf, '/', '_');
        strrepl(tempBuf, ';', '_');
        strrepl(tempBuf, ':', '_');
        if (SIM800_EMAILsend(&SysParam, (char *)Subj_Report,  (char *)Body_Report_Active, tempBuf) == 0)
         {
           uint16_t sendedSize = 0;
           uint16_t packetSize;
           while (sendedSize < sizeFile)
            { 
              packetSize = sizeFile - sendedSize;
              if (packetSize > 128) packetSize = 128;
              if  (Sim800_FileREAD (LOG_NAME_YTD, packetSize, sendedSize, tempBuf) != packetSize)
                break; 
              if (SIM800_EMAILattachPacket(packetSize, tempBuf) == 1)
                break;
              sendedSize +=  packetSize;
            } //  while
           if ((SIM800_EMAILattachPacket(0, tempBuf) == 0) && (sendedSize == sizeFile))
             {
              deviceFlags &= ~FPENDING_EMAIL_YESTERDAY_REPORT;
              deviceFlags &= ~GPRS_CONNECT_ERROR;
             }
         }//if (SIM800_EMAILsend 
       } //if (Sim800_FileREAD(LOG_NAME_YTD
     } // if (sizeTodayFile > LEN_YYYYMMDD)
    
    else //вчерашняя копия отчета куда-то делась. Просто игнорируем это и отменяем задание
      if (SIM800_EMAILsend(&SysParam, (char *)Subj_Report,  (char *)Body_Report_Y_Empty, NULL) == 0) 
        deviceFlags &= ~FPENDING_EMAIL_YESTERDAY_REPORT;
     
    SIM800_GPRS_Disconnect();
   }
  }
//---------------------------------------------------
   
     
    // check if need to change configuration structure - save it in CONFIG file   
    if ((deviceFlags & FPENDING_SAVE_CONFIG) >0)
      {
        SysParam.strCRC = CRCcalc( &SysParam.PonyName, sizeof(SysParam)-sizeof(SysParam.strCRC));
        trycnt=3; //save main structure to FileSystem SIM800
        while ((Sim800_FileWrite(CONFIG_INI,0,sizeof(SysParam),((char *)&SysParam)) != FILE_WRITE_SUCCESS) & (--trycnt>0))
          vTaskDelay(1000);
       if (trycnt == 0) goto Full_Reset_SIMmodule;
       deviceFlags &= ~FPENDING_SAVE_CONFIG;
      }
   //check to time syncronize state
    if ((deviceFlags & TIME_SYNC_ERROR) > 0)
      goto lSyncTime;
     if (Sim800_GsmBusy(0) !=  SIMPLE_CMD_SUCCESS) goto Full_Reset_SIMmodule;
     vTaskDelay(250);
  }//while (1)
} // vSIM800   


void vApplicationIdleHook()
{
    SCB->SCR = 0x0;  //SCB->SCR = SCB_SCR_SLEEPDEEP;
    __DSB(); //Added by me
    __WFI();
    __ISB(); //Added by me
  //(GPIO_READ_OUTPUT(GPIOC,1<<LED_13_PIN)==0) ?  GPIO_SET(GPIOC,1<<LED_13_PIN):GPIO_RESET(GPIOC,1<<LED_13_PIN);
}

/*-----------------------------------------------------------------------------------------------------------------*/
void main()
{ 
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN ;      //GPIO A & C & AFIO & USART1
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;// PWR & Backup Enable

  PinParametr GPIO_descript[3];
  GPIO_descript[0].PinPos=RST_SIM800;
  GPIO_descript[0].PinMode=GPIO_MODE_OUTPUT2_OPEN_DRAIN;
  
  GPIO_descript[1].PinPos=TX_FOR_SIM800;
  GPIO_descript[1].PinMode=GPIO_MODE_OUTPUT10_ALT_PUSH_PULL;
  
  GPIO_descript[2].PinPos=RX_FOR_SIM800;
  GPIO_descript[2].PinMode=GPIO_MODE_INPUT_PULL_UP_DOWN;
  
  GPIO_SET(SIM800_PORT,1<<RX_FOR_SIM800);             //PullUp
  CLL_GPIO_SetPinMode(SIM800_PORT,GPIO_descript,3);   //SIM800_PORT init
  
  GPIO_descript[0].PinPos=DTR_SIM800;
  GPIO_descript[0].PinMode=GPIO_MODE_OUTPUT2_OPEN_DRAIN;
  CLL_GPIO_SetPinMode(SIM800_DTR_PORT,GPIO_descript,1);   //DTR output config
  
  GPIO_descript[0].PinPos=LED_13_PIN;
  GPIO_descript[0].PinMode=GPIO_MODE_OUTPUT50_PUSH_PULL;  
  CLL_GPIO_SetPinMode(GPIOC,GPIO_descript,1);   //GPIOC init
  GPIO_SET(GPIOC,1<<LED_13_PIN);
  
  
  //RCC->CR |= RCC_CR_CSSON; //detect Fault HSE (NMI handler)
   
   //SWO debug ON
  DBGMCU->CR &= ~(DBGMCU_CR_TRACE_MODE_0 | DBGMCU_CR_TRACE_MODE_0);
  DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
  
  // JTAG-DP Disabled and SW-DP Enabled
  AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;
  
  //RTC init
  if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
  {
    PWR->CR   |= PWR_CR_DBP;// Backup Access Enable
    RCC->BDCR |=  RCC_BDCR_BDRST;//Backup Reset
    RCC->BDCR &= ~RCC_BDCR_BDRST;
    RCC->BDCR |= RCC_BDCR_LSEON;//start LSE
    while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY)    //wait ready LSE 
      asm("NOP");
    RCC->BDCR |= RCC_BDCR_RTCSEL_LSE;//LSE select
    RCC->BDCR |= RCC_BDCR_RTCEN;
    RTC->CRL &= (uint16_t)~RTC_CRL_RSF;
    while ((RTC->CRL & RTC_CRL_RSF) == 0)
      asm("NOP");
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0)
      asm("NOP");
    RTC->CRL  |= RTC_CRL_CNF;//Enter configuration mode
    RTC->PRLL =  0x7FFF;// 1 sec
    RTC->PRLH=  0x00;
    RTC->CRL  &= ~RTC_CRL_CNF;//exit configuration mode 
  }
  
  USART1->CR1 |= USART_CR1_UE ;                 //USART1 enable
  USART1->CR1 &= ~(uint16_t)(USART_CR1_M);      //8 data bits
  USART1->CR2 &= ~(uint16_t)(USART_CR2_STOP);   //1 Stop bit
  USART1->BRR = (uint16_t)117<<4 | (uint16_t)3;  //19200 baud    (117*16+3)*19200=36MHz
  USART1->CR1 |= USART_CR1_TE;      //Tx enable
  USART1->CR1 |= USART_CR1_RXNEIE;  //Interrupt RXNE  enable
  USART1->CR1 |= USART_CR1_RE;  // Rx enable
  OringBuf_Init();
  NVIC_EnableIRQ(USART1_IRQn);
  
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;  //DMA1 clocking enable
  DMA_Disable(DMA1_Channel4);	    //DMA1_Channel4 is use for USART1 Tx
  DMA_DeInit(DMA1_Channel4);	    //Reset to Default State
  DMA_Init( DMA1_Channel4,	
              (uint32_t)&(USART1->DR),		//adres Periferial registr
              (uint32_t)0,	                //address in memory.
              0,			//transfer Data Size 
              TRANSFER_COMPL_INT_DISABLE        |
              HALF_COMPL_INT_DISABLE            |
              TRANSFER_ERROR_INT_DISABLE        |
              READ_FROM_MEMORY                  |
              CIRCULAR_MODE_DISABLE             |
              PERIPHERAL_INC_MODE_DISABLE       |
              MEMORY_INC_MODE_ENABLE            |
              PERIPHERAL_SIZE_8                 |
              MEMORY_SIZE_8                     |
              CHANNEL_PRIOTITY_LOW              |
              MEMORY_2_MEMORY_MODE_DISABLE);
 
  USART1->CR3 |=USART_CR3_DMAT;		//Connect USART1 to DMA4
  
  flagSysParamRead = 0;
  if (xTaskCreate(vTelemetry,"Telemetry", 	configMINIMAL_STACK_SIZE*4, NULL, tskIDLE_PRIORITY + 1, NULL)!=pdTRUE) ERROR_ACTION(TASK_NOT_CREATE,0);
  if (xTaskCreate(vSIM800,"SIM800", 	configMINIMAL_STACK_SIZE*11, NULL, tskIDLE_PRIORITY + 1, NULL)!=pdTRUE) ERROR_ACTION(TASK_NOT_CREATE,0);
  
   vTaskStartScheduler();
  for( ;; )
  {
  ITM_SendChar((uint32_t)0x80);
  }
}

void NMI_Handler()
{
  if (RCC->CIR & RCC_CIR_CSSF) 
  {
    RCC->CIR|=RCC_CIR_CSSC;
    GPIO_RESET(GPIOC,1<<LED_13_PIN);
  }
}

/*void Standby()
{
    SCB->SCR = SCB_SCR_SLEEPDEEP;
    PWR->CR |= PWR_CR_PDDS;
    PWR->CSR &= PWR_CSR_WUF;
    __DSB(); //Added by me
    __WFI();
    __ISB(); //Added by me
}*/

void ParkDevice()
{
  SIM800_DisableRF();
  vTaskDelay(250);
  SIM800_Sleep(1); //sleep
  RCC->CR |=RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0)
    asm("nop");     //wait ready HSI
  RCC->CFGR = 0;    //switch to HSI
  while ((RCC->CFGR & RCC_CFGR_SWS) != 0)
    asm("nop");     //wait switch to HSI
  RCC->CR |= (uint32_t)0x00000001; //disable HSE
  RCC->CFGR |= RCC_CFGR_HPRE; // AHB/512
  while (1)
    vTaskDelay(25);
 }



