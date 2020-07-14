#include "stm32F10x_gpioMY.h"
#include "SIM800Lib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32F10x_usart1.h"
#include <string.h>
#include <stdlib.h>

#define LENBUF 256
#define TIME_BREAK  100 //garantied timeout before send new AT command
#define TIME_OUT    500
#define SIZE_BLOCK  200
#define MIN_LONG_TIME_FREE_STATE    100 //0,4 sec
#define TIME_OUT_FREE_STATE        1000 //4 sec
char incoming[LENBUF];

//asks
const static char AT_IPR[]="AT+IPR=19200\r\n";
const static char ATE[]="ATE0\r\n";            //ECHO off
const static char ATV[]="ATV0\r\n";            //short answer
const static char AT_CMGF[]="AT+CMGF=1\r\n";   //TEXT MODE
const static char AT_CSCS[]="AT+CSCS=\"GSM\"\r\n";      //GSM MODE
const static char AT_CLIP[]="AT+CLIP=1\r\n";   //AOH
const static char AT_DDET[]="AT+DDET=1,200\r\n";   //DTMF decoder ON
const static char AT_CREG[]="AT+CREG?\r\n";		//type registration
const static char AT_COLP[]="AT+COLP=1\r\n";   //answer mode
const static char AT_CLTS[]="AT+CLTS=1\r\n";    //local timeStamp
const static char AT_W[]="AT&W\r\n";    //save setting
const static char AT_CCLK[]="AT+CCLK?\r\n";     //get Time
const static char AT_NETL_OFF[]="AT+CNETLIGHT=0\r\n";
const static char AT_SLEEP_MODE1[]="AT+CSCLK=1\r\n";
const static char AT_CBS[]="AT+CBC\r\n";

const static char AT_FSLS_C_USER[]="AT+FSLS=C:\\USER\\\r\n";    //Get the list of Files in C:\USER 
const static char AT_FSCREATE_PREF[]="AT+FSCREATE=C:\\USER\\";     //create File prefix
const static char SYM_RN[]="\r\n";
const static char AT_FSWRITE_PREF[]="AT+FSWRITE=C:\\USER\\";//write File prefix
const static char AT_FSWRITE_SUFF[]=",3\r\n";     //write File suffics
const static char AT_FSREAD_PREF[]="AT+FSREAD=C:\\USER\\";    //Read File prefix
const static char AT_FSRENAME_PREF[]="AT+FSRENAME=";
const static char AT_FSFLSIZE_PREF[]="AT+FSFLSIZE=C:\\USER\\";
const static char AT_FSDEL_PREF[]="AT+FSDEL=";

const static char AT_CLCC[]="AT+CLCC\r\n";      //Call State
const static char ATA[]="ATA\r\n";
const static char ATH[]="ATH\r\n";
const static char AT_SAPBR_TYPE[]="AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n";
const static char AT_SAPBR_APN[]="AT+SAPBR=3,1,\"APN\",\"";
const static char AT_SAPBR_SUFF[]="\"\r\n";
const static char AT_SAPBR_ON[]="AT+SAPBR=1,1\r\n";
const static char AT_SAPBR_IP[]="AT+SAPBR=2,1\r\n";
const static char AT_SAPBR_OFF[]="AT+SAPBR=0,1\r\n";
const static char AT_CNTP_SERV[]="AT+CNTP=\"pool.ntp.org\",8\r\n";
const static char AT_CNTPCID[]="AT+CNTPCID=1\r\n";
const static char AT_CNTP[]="AT+CNTP\r\n";
const static char AT_CMGS[]="AT+CMGS=\"";    //SMS send
const static char AT_EMAIL_CID[]="AT+EMAILCID=1\r\n";
const static char AT_EMAIL_TO[]="AT+EMAILTO=60\r\n";
const static char AT_EMAIL_SSL[]="AT+EMAILSSL=2\r\n";
const static char AT_EMAIL_SMTPSRV[]="AT+SMTPSRV=\"";
const static char AT_EMAIL_AUTH[]="AT+SMTPAUTH=1,\"";
const static char AT_EMAIL_RECIP[]="AT+SMTPRCPT=0,0,\"";
const static char AT_EMAIL_FROM[]="AT+SMTPFROM=\"";
const static char AT_EMAIL_SUB[]="AT+SMTPSUB=\"";
const static char AT_EMAIL_BODY[]="AT+SMTPBODY=";
const static char AT_EMAIL_SEND[]="AT+SMTPSEND\r\n";
const static char SUFF_LNK[]="\",\"";
const static char AT_SMTP_FILE[]="AT+SMTPFILE=1,\"";
const static char AT_SMTP_FILE_SUFF[]="\",0\r\n";
const static char AT_SMTP_FT[]="AT+SMTPFT=";

const static char AT_CPMS[]="AT+CPMS=\"SM\"\r\n";
const static char AT_CMGL_UNR[]="AT+CMGL=\"REC UNREAD\"\r\n";
const static char AT_CMGDR[]="AT+CMGDA=\"DEL READ\"\r\n";
static const char AT_CREC4[]="AT+CREC=4,\"C:\\USER\\";
static const char AT_CREC4_suff[]="\",1,100\r\n";
static const char ATD[]="ATD";
static const char ATD_SUFF[]=";\r\n";
static const char AT_CHLD2[]="AT+CHLD=2\r\n";
static const char AT_CHLD3[]="AT+CHLD=3\r\n";
static const char AT_CSQ[]="AT+CSQ\r\n";
static const char AT_POWERDOWN[]="AT+CPOWD=1\r\n";
static const char AT_CFUN0[]="AT+CFUN=0\r\n";

//answers
const static char ANSW_OK[]="OK\r\n";
const static char ANSW_0[]="0\r\n";
const static char ANSW_NO_CARRIER[]="3\r\n";
const static char ANSW_ERROR[]="4\r\n";
const static char ANSW_SMS_ERROR[]="> 4\r\n";
const static char ANSW_NODIAL[]="6\r\n";
const static char ANSW_BUSY[]="7\r\n";

const static char ANSW_CREG[]="+CREG: 0,1\r\n";
const static char ANSW_ERR[]="+CME ERROR:";
const static char _DTMF[]="+DTMF:";
const static char _CREC0[]="+CREC: 0\r\n";
const static char _CNTP1[]="+CNTP: 1\r\n";
const static char _CMTI[]="+CMTI:";
const static char _DNLD[]="DOWNLOAD\r\n";
const static char _SMTPSEND[]="+SMTPSEND: 1\r\n";
const static char _CMGL[]="+CMGL:";
const static char _NORM_PD[]="NORMAL POWER DOWN\r\n";

//-------------------------------------------------------
void Sim800_RST()
{
GPIO_RESET(SIM800_PORT,1<<DTR_SIM800);  //DTR=0 (wake up SIM800);
GPIO_RESET(SIM800_PORT,1<<RST_SIM800);  //RST input=0;
vTaskDelay(100);     //400 ms
GPIO_SET(SIM800_PORT,1<<RST_SIM800);    //RST input=1
vTaskDelay(2000);   //8 sec
}

void SIM800_Sleep(uint8_t sleep)
{
  if (sleep == 1)
   GPIO_SET(SIM800_DTR_PORT,1<<DTR_SIM800);
  else
  {
   GPIO_RESET(SIM800_DTR_PORT,1<<DTR_SIM800); 
   vTaskDelay(50);     //50*4=200ms
  }
}
//------------------------------------------------------
/* String Reading with Timeout. 
for input:  *Dest - pointer free place for String
            Timeout - timeout in SysTickIf 
return:     Lenth of read line */
uint16_t Sim800_WaitLine(char *Dest,  uint16_t TimeOut)
{
  uint16_t quant;
  do
  {
  quant=USART_GetLine(Dest);
  if (quant>0)  break;
  vTaskDelay(1);
  } while (TimeOut-->0);
  return (quant);
}

/*------------------------------------------------------
String Reading with Timeout. 
for input:  *Dest - pointer free place for String
            Timeout - timeout in SysTickIf 
            *LenStr - ptr. Variable contain Lenth of Array
for output: *LenStr contain enth of Array OR 0 if timeout
*/
void Sim800_WaitArray(char *Dest, uint16_t *LenStr, uint16_t TimeOut)
{
  uint16_t quant;
  do
  {
  quant=USART_GetArray(Dest,*LenStr);
  if (quant>0)  break;
  vTaskDelay(1);
  } while (TimeOut-->0);
  *LenStr=quant;
  return;
}

/*------------------------------------------------------
return 1 if USART receive buffer is Empty MIN_LONG_TIME_FREE_STATE
or       0 if TIME_OUT_FREE_STATE buffer not Empty */
uint8_t waitSilent ()
{
  uint16_t EmptyStateTime=0;
  uint16_t trycnt=TIME_OUT_FREE_STATE;
  uint16_t OldFullness;
  do
   {
     uint16_t fulness=OringBuf_GetFullness();
    if (fulness != OldFullness)
    {
      OldFullness=fulness;
      EmptyStateTime=0;
    }
    else 
       vTaskDelay(1);
    if (++EmptyStateTime>MIN_LONG_TIME_FREE_STATE) return (1);
   } while (--trycnt>0);
  return(0);  
}

/*------------------------------------------------------
Send comand as pointer *CmdPtr
and wait ANSW_OK or ANSW_0
return:  SIMPLE_CMD_FAILED_SEND - if send not done
or       SIMPLE_CMD_BAD_ANSWER - if answer ANSW_OK or ANSW_0 haven't waited
or       SIMPLE_CMD_SUCCESS - if OK */
retcode_SIM800_SimpleCmd SimpleCmd(const char *CmdPtr,uint16_t LenCmd)
{
  uint16_t Lenth;
  if (waitSilent()==0)   return(SIMPLE_CMD_FAILED_SEND);
  OringBuf_Clear();
  SendStringUsart1WithDMA(CmdPtr,LenCmd);   
  do    //wait ANSWER
  {
    Lenth=Sim800_WaitLine(incoming, TIME_OUT);
    if (Lenth == 0)  return(SIMPLE_CMD_BAD_ANSWER);
  } while ((strcmp(incoming,ANSW_OK)!=0) && (strcmp(incoming,ANSW_0)!=0));   //bad answer
 return (SIMPLE_CMD_SUCCESS);
}

/*------------------------------------------------------
Boot initial SIM800 module. Exexuted while init not completed
*/
void Sim800_Prepare()
{
uint8_t trycnt;
do
  {
    Sim800_RST();
    trycnt=30;
    while (--trycnt>0)
     {
       if (SimpleCmd(AT_IPR,sizeof(AT_IPR)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(ATE,sizeof(ATE)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(ATV,sizeof(ATV)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (Sim800_GsmBusy(1) != SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_CMGF,sizeof(AT_CMGF)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_CSCS,sizeof(AT_CSCS)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_CLIP,sizeof(AT_CLIP)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_DDET,sizeof(AT_DDET)-1)!=SIMPLE_CMD_SUCCESS) continue; 
       if (SimpleCmd(AT_COLP,sizeof(AT_COLP)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_CPMS,sizeof(AT_CPMS)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_CLTS,sizeof(AT_CLTS)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_W,sizeof(AT_W)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_NETL_OFF,sizeof(AT_NETL_OFF)-1)!=SIMPLE_CMD_SUCCESS) continue;
       if (SimpleCmd(AT_SLEEP_MODE1,sizeof(AT_SLEEP_MODE1)-1)!=SIMPLE_CMD_SUCCESS) continue;
       break;
     }
  } while (trycnt==0);
}

/*------------------------------------------------------
Check registration in GSM */
NetworkRegType Sim800_CheckReg()
{
  uint16_t Lenth;
  NetworkRegType answ;
  if (waitSilent()==0)  return (ERROR_CREG);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_CREG,sizeof(AT_CREG)-1); 

 do //this cycle take all answer and compare first 6 symbol "+CREG:"
 {
   Lenth=Sim800_WaitLine(incoming, TIME_OUT);
   if (Lenth==0) return (ERROR_CREG);   //answer timeout
 } while (strncmp(incoming,ANSW_CREG,6)!=0);
 answ= (*(incoming+9)-0x30);
 Lenth=Sim800_WaitLine(incoming, TIME_OUT);
 return (answ);
}

/*------------------------------------------------------
Command AT_CLTS/ return >0 if Success, or 0 if Fault */
uint8_t Sim800_LocalTimeStamp()
{
  uint8_t trycnt=5;
  while ((SimpleCmd(AT_CLTS,sizeof(AT_CLTS)-1)!=SIMPLE_CMD_SUCCESS) & (--trycnt>0))
    vTaskDelay(125);     //125*4=0.5sec
  return (trycnt);
}

/*------------------------------------------------------
return UNIX time (seconds before 01.01.1970)
or       0 if Error
*/
time_t Sim800_GetTime ()
{
struct tm DT; 
uint16_t Lenth;
  if (waitSilent()==0)  return (0);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_CCLK,sizeof(AT_CCLK)-1); 
  
 //this cycle take all answer and compare first 5 symbol "+CCLK"
 do 
 {
   Lenth=Sim800_WaitLine(incoming, TIME_OUT);
   if (Lenth==0)    return (0);   //answer timeout
 } while (strncmp(incoming,AT_CCLK+2,5)!=0);
 char *ptr=strchr(incoming,0x22); // search "
 if (ptr==NULL)    return(0);
 DT.tm_year=100+(*(ptr+1)-0x30)*10+(*(ptr+2))-0x30;
 DT.tm_mon=(*(ptr+4)-0x30)*10+(*(ptr+5))-0x30-1;
 DT.tm_mday=(*(ptr+7)-0x30)*10+(*(ptr+8))-0x30;
 DT.tm_hour=(*(ptr+10)-0x30)*10+(*(ptr+11))-0x30;
 DT.tm_min=(*(ptr+13)-0x30)*10+(*(ptr+14))-0x30;
 DT.tm_sec=(*(ptr+16)-0x30)*10+(*(ptr+17))-0x30;
 DT.tm_isdst=0;
 //DT->zz=(*(ptr+19)-0x30)*10+(*(ptr+20))-0x30;
 //if (*(ptr+18)==0x2D) DT->zz = -DT->zz;     //if "-"
 Lenth=Sim800_WaitLine(incoming, TIME_OUT);
 return (mktime(&DT));
}

//return battery capasity in percent or 255 if fault
uint8_t Sim800_GetBattery ()
{
uint16_t Lenth;
uint8_t retval;
  if (waitSilent()==0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_CBS,sizeof(AT_CBS)-1); 
  
 //this cycle take all answer and compare first 5 symbol "+CBS"
 do 
 {
   Lenth=Sim800_WaitLine(incoming, TIME_OUT);
   if (Lenth==0)    return (255);   //answer timeout
 } while (strncmp(incoming,AT_CBS+2,4)!=0);
 char *ptr=strchr(incoming,','); // search ,
 if (ptr == NULL)    return(255);
 retval=(uint8_t)atoi(++ptr);
 Lenth=Sim800_WaitLine(incoming, TIME_OUT);
 return (retval);
}

/*------------------------------------------------------
Allow or Deny incoming Call (AT_BUSY1 or AT_BUSY0)*/
retcode_SIM800_SimpleCmd Sim800_GsmBusy(uint8_t busy)
{
  const static char AT_GSM_BUSY1[]="AT+GSMBUSY=1\r\n"; //Deny Calls
  const static char AT_GSM_BUSY0[]="AT+GSMBUSY=0\r\n"; //Deny Calls
  const static char AT_GSM_BUSYQ[]="AT+GSMBUSY?\r\n";
  const char *ptr;
  uint8_t trycnt=10;
  uint16_t Lenth;
  if (busy == 1) ptr=AT_GSM_BUSY1;
   else ptr=AT_GSM_BUSY0;

  do
  {
    SimpleCmd(ptr,sizeof(AT_GSM_BUSY1)-1);
    if (waitSilent() == 0)  return (SIMPLE_CMD_FAILED_SEND);   //permanent full Rx buffer
    OringBuf_Clear();
    SendStringUsart1WithDMA(AT_GSM_BUSYQ,sizeof(AT_GSM_BUSYQ)-1);
    do
    {
      Lenth=Sim800_WaitLine(incoming, TIME_OUT);
      if (strncmp(incoming,AT_GSM_BUSYQ+2,8) == 0)
        if (*(incoming + 10) - '0' == busy) return (SIMPLE_CMD_SUCCESS); //success
    } while (Lenth > 0);
    vTaskDelay(250);
  } while (--trycnt >0);
  return (SIMPLE_CMD_BAD_ANSWER); 
}

/*------------------------------------------------------
Chech exist file in foldef C:\USER\*OnlyName
return 1 if File Exist 
      or 0 - if not Exist 
      or 255 if Command failed send
*/
uint8_t Sim800_FileExist(const char *OnlyName)
{
  uint16_t Lenth;
  uint8_t trycnt=4;        //try if No Answer
Re_try:
  if (waitSilent() == 0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_FSLS_C_USER,sizeof(AT_FSLS_C_USER)-1); 
  do
  {
    Lenth=Sim800_WaitLine(incoming, TIME_OUT);
    if (Lenth == 0)
      if(--trycnt>0) goto Re_try;
       else break;
    if ((strcmp(incoming,ANSW_OK)==0) | (strcmp(incoming,ANSW_OK)==0)) return (0); //File not exist
    strrepl(incoming, 0x0D, 0);
    strrepl(incoming, 0x0A, 0);
    if (strcmp(incoming,OnlyName)==0)  return (1); //File is exist
    
  } while (1);
  return (255);
}

/*------------------------------------------------------                          
Create File in filesystem SIM800.
input:  *OnlyName - pointer of name without "
output: 1 - SUCCESS comand send to SIM800
        0 - Error create
        255 - command failed send.
*/
uint8_t Sim800_FileCreate(const char *OnlyName)
{
  uint16_t Lenth;
  uint8_t trycnt=4;        //try if No Answer
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSCREATE_PREF);
  ptr= stradd(ptr,OnlyName);
  ptr= stradd(ptr,SYM_RN);
Re_try:
  if (waitSilent()==0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(tempBuf,strlen (tempBuf)); 
  do
  {
    Lenth=Sim800_WaitLine(incoming, TIME_OUT);
    if ((strcmp(incoming,ANSW_OK)==0) | (strcmp(incoming,ANSW_0)==0)) return (1); //File created
    if (strncmp(incoming,ANSW_ERR,sizeof(ANSW_ERR)-1)==0) return(0);
    if (Lenth==0)
      if(--trycnt>0) goto Re_try;
  } while (trycnt>0);
  return (255);
}

/*------------------------------------------------------                          
File  Size in filesystem SIM800.
input:  *OnlyName - pointer of name without "
output: 1 - SUCCESS comand send to SIM800
        0 - Error
        255 - command failed send.
return *size - lenth of File
*/
uint8_t Sim800_FileSize(const char *OnlyName, uint16_t *size)
{
  uint16_t Lenth;
  uint8_t trycnt=4;        //try if No Answer
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSFLSIZE_PREF);
  ptr= stradd(ptr,OnlyName);
  ptr= stradd(ptr,SYM_RN);
Re_try:
  if (waitSilent()==0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(tempBuf,strlen (tempBuf)); 
  do
  {
    Lenth=Sim800_WaitLine(incoming, TIME_OUT);
    if (strncmp(incoming,AT_FSFLSIZE_PREF+2,9) == 0)
    {
      ptr = strchr(incoming,':');
      *size = (uint16_t)atoi(++ptr); //return Size
      return (1);
    }
    if (strncmp(incoming,ANSW_ERR,sizeof(ANSW_ERR)-1)==0) return(0);
    if (Lenth==0)
      if(--trycnt>0) goto Re_try;
  } while (trycnt>0);
  return (255);
}


/*------------------------------------------------------                          
Rename File in filesystem SIM800.
input:  *OldName - pointer of Old name without "
        *NewName - pointer of New name without "
output: 1 - SUCCESS comand send to SIM800
        0 - Error rename
        255 - command failed send. */
uint8_t Sim800_FileRename(const char *OldName, const char *NewName)
{
  const static char Path[]="C:\\USER\\";
  uint8_t trycnt=4;        //try if No Answer
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSRENAME_PREF);
  ptr= stradd(ptr,Path);
  ptr= stradd(ptr,OldName);
  *(ptr++)=',';
  ptr= stradd(ptr,Path);
  ptr= stradd(ptr,NewName);
  ptr= stradd(ptr,SYM_RN);
  if (waitSilent()==0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  do
  {
    if (SimpleCmd(tempBuf,strlen (tempBuf)) == SIMPLE_CMD_SUCCESS)
      return (1);
     vTaskDelay(125); 
  } while (--trycnt > 0);
  return (0);
}

uint8_t Sim800_FileDel(const char *name)
{
  const static char Path[]="C:\\USER\\";
  uint8_t trycnt=4;        //try if No Answer
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSDEL_PREF);
  ptr= stradd(ptr,Path);
  ptr= stradd(ptr,name);
  ptr= stradd(ptr,SYM_RN);
  if (waitSilent()==0)  return (255);   //permanent full Rx buffer
  OringBuf_Clear();
  do
  {
    if (SimpleCmd(tempBuf,strlen (tempBuf)) == SIMPLE_CMD_SUCCESS)
      return (1);
     vTaskDelay(125); 
  } while (--trycnt > 0);
  return (0);
}


/*------------------------------------------------------
Write File in filesystem SIM800. Only send command. User should wait > symbol and send data for write
input:  *OnlyName   - pointer of path and name without "
        mode        - 0 write at begin,  = 1 append
        LenWrite    - Lenth Data for writing
        *OutData    - pointer of DataStorage for sending
output: enum retcode_Sim800_FileWrite
*/
retcode_Sim800_FileWrite Sim800_FileWrite(const char *OnlyName, uint8_t mode, uint16_t LenWrite, char *OutData)
{
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSWRITE_PREF);
  ptr= stradd(ptr,OnlyName);
  *(ptr++)=',';
  *(ptr++)= mode+'0';
  *(ptr++)=',';
  ptr = itoa(LenWrite,ptr);
  ptr= stradd(ptr,AT_FSWRITE_SUFF);
  if (waitSilent()==0)  return (FILE_WRITE_BUSY);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(tempBuf,strlen (tempBuf));
  uint16_t WaitPrompt = TIME_OUT;
  do
  {
    vTaskDelay(1);
    if ((USART_GetArray(incoming, 1)==1) & (*incoming=='>')) 
      break;
    if (--WaitPrompt==0)  
      return (FILE_WRITE_NOT_PROMPT); //didn't wait '>'
  } while(1);
  if (SimpleCmd(OutData,LenWrite)!=SIMPLE_CMD_SUCCESS)   return (FILE_WRITE_TIMEOUT);    
  return (FILE_WRITE_SUCCESS);
}

/*------------------------------------------------------
Read File in filesystem SIM800. Only send command.
input:  *OnlyName - pointer of path and name without "
         Size    - Lenth Data for reading
         Position - position for begin read
        *Storage - place for Reading bytes
output: how many bytes receive
*/
uint16_t Sim800_FileREAD(const char *OnlyName, uint16_t Size, uint16_t Position, char *Storage)
{
  char tempBuf[64];
  char *ptr = tempBuf;
  ptr= stradd(ptr,AT_FSREAD_PREF);
  ptr= stradd(ptr,OnlyName);
  *(ptr++)=',';
  *(ptr++)='1';
  *(ptr++)=',';
  ptr = itoa(Size,ptr);
  *(ptr++)=',';
  ptr = itoa(Position,ptr);
  ptr= stradd(ptr,SYM_RN);
  
  if (waitSilent()==0)  return (0);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(tempBuf,strlen(tempBuf));    //send start position for Reading
  
  //wait string 0D0A before DataArray
  uint16_t ReceiveLen=Sim800_WaitLine(incoming, TIME_OUT);
  if (ReceiveLen == 0)
    return (0); 
  //wait containing of File
  char *pStorage=Storage;
  uint16_t TotalLen=0;
  while (TotalLen<Size)
  {
    ReceiveLen=Size-TotalLen;
    if (ReceiveLen>SIZE_BLOCK) ReceiveLen=SIZE_BLOCK;
    Sim800_WaitArray(pStorage,&ReceiveLen,TIME_OUT);
    pStorage +=ReceiveLen;
    TotalLen +=ReceiveLen;
    if (ReceiveLen==0)  return (0);
  }
  return (TotalLen);
}


/*------------------------------------------------------
return 1  if have Incoming Call
return 2  if have Incoming SMS
*/
uint8_t SIM800_EventIncoming (uint8_t *smsNum)
{
  uint16_t quant = USART_GetLine(incoming);
  if (quant>0)
  {
    if (strncmp(incoming,AT_CLIP+2,5)==0)   //RING detection
      return (1);
    if (strncmp(incoming,_CMTI,sizeof(_CMTI)-1)==0)   //SMS detection
    {
      char *ptr=strchr(incoming,','); // search ','
      if (ptr == NULL)       return (0);
      *smsNum = atoi(++ptr);
      if ((*smsNum) > 0)
        return (2);
    }
  }
  return (0); 
}


/*------------------------------------------------------
Refreched status of Calling
return            0 - if   silent
                  1 - if   have Call
                  255 - failed send command
fillet structure callparam.PhoneNum, callparam.AdminNum, callparam.Status
*/
uint8_t SIM800_CLCC (sActiveCall *callparam, sSys_Param *WorkParam)
{
  char *ptr, *ptr2;
  uint16_t LenLine;
  if (waitSilent()==0)  return (255);   //fault send command
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_CLCC,sizeof(AT_CLCC)-1); 
  while (LenLine=Sim800_WaitLine(incoming, TIME_OUT)>0)
  {
    if ((strcmp(incoming,ANSW_OK)==0) | (strcmp(incoming,ANSW_0)==0))   break;
    if (strncmp(incoming,AT_CLCC+2,5)!=0)   continue;
    //have +CLCC
    ptr=strchr(incoming,'"'); // search 1st "
    if (ptr==NULL)         return (255);
    ptr2=strchr(++ptr,'"'); // search 2nd "
    if (ptr2==NULL)        return (255);
    *ptr2=0x00;   //end string terminator
    strcpy (callparam->PhoneNum, ptr);
    if (strcmp(ptr,WorkParam->AdminPhoneNum_1)==0)
      callparam->AdminNum=1;
    else if (strcmp(ptr,WorkParam->AdminPhoneNum_2)==0)
          callparam->AdminNum=2;
         else if (strcmp(ptr,WorkParam->AdminPhoneNum_3)==0)
              callparam->AdminNum=3;
              else         callparam->AdminNum=0;
    ptr=strchr(incoming,',');       // search 1st ,
    if (ptr++==NULL)         return (255);
    ptr2=strchr(ptr,','); // search 2nd "
    if (ptr2++==NULL)        return (255);
    if ((*ptr=='1') & (*ptr2=='4')) callparam->Status=STATUS_DIR_INCOMING;
      else if ((*ptr=='1') & (*ptr2=='0')) callparam->Status=STATUS_DIR_ACTIVE_IN;
            else if ((*ptr=='0') & (*ptr2=='0')) callparam->Status=STATUS_DIR_ACTIVE_OUT;          
                else  callparam->Status=STATUS_DIR_NONE;
    return(1);    //success decode +CLCC: answer
  }
  if (LenLine==0)   return (255);
  callparam->Status=STATUS_DIR_NONE;
  return (0);
}

 /*------------------------------------------------------
*/
retcode_SIM800_SimpleCmd SIM800_ATA()
  {
    return (SimpleCmd(ATA,sizeof(ATA)-1));
  }

/*------------------------------------------------------
*/
retcode_SIM800_SimpleCmd SIM800_ATH()
  {
    return (SimpleCmd(ATH,sizeof(ATH)-1));
  }

/*------------------------------------------------------
return 1 if have +DTMF. and save DTMF code to struct
return 0 if not incoming events
return 255 if NO_CARRIER or ANSW_NODIAL or BUSY or ERROR incoming

*/
retcode_Event SIM800_EventsActive (sActiveCall *callparam)
{
  vTaskDelay(1);
  uint16_t quant = USART_GetLine(incoming);
  if (quant==0) return (EVENT_NONE);
  if (strncmp(incoming,_DTMF,sizeof(_DTMF)-1)==0)   //+DTMF detection
  {
    char DTMFchar=*(incoming+7);
    if (DTMFchar == '#')
        callparam->headDTMF=0;
    else
      if (callparam->headDTMF < SIZE_DTMF_BUF)
      {
        callparam->AccumDTMF[(callparam->headDTMF)++] = DTMFchar;
        callparam->AccumDTMF[(callparam->headDTMF)] = 0x00;
        return (EVENT_DTMF);
      }
  }
  else if (strncmp(incoming,_CREC0,sizeof(_CREC0)-1)==0)   //+CREC: 0 detection
    return (EVENT_CREC0);
  
  else if ((strcmp(incoming,ANSW_BUSY)==0) | (strcmp(incoming,ANSW_NO_CARRIER)==0) | (strcmp(incoming,ANSW_NODIAL)==0))
      return (EVENT_NOCARRIER);
  return (EVENT_NONE); 
}

uint8_t SIM800_IP()
{
  uint8_t retcode;
  uint16_t Lenth;
  waitSilent();
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_SAPBR_IP,sizeof(AT_SAPBR_IP)-1);
  do    //wait IP
  {
    Lenth=Sim800_WaitLine(incoming, 1000);
    if (Lenth==0)
    {
      retcode=255; //timeout
      break;
    }
    if (strncmp(incoming,AT_SAPBR_IP+2,6) == 0)
    {
      char *ptr = strchr(incoming,'\"');
      if (ptr++ != NULL)
      {
        char *ptr2 = strchr(ptr,'\"');
        if (ptr2 != NULL)
          if ((ptr2-ptr >7) && (ptr2-ptr < 16)) //long IP 7-15 characters
            retcode=0;       //have IP
          else retcode=1;    // IP=0.0.0.0
      }
    }
  } while ((strcmp(incoming,ANSW_OK)!=0) & (strcmp(incoming,ANSW_0)!=0));
    return(retcode);  
}

// return 0 if SUCCESS of 1 if FAULT
uint8_t SIM800_GPRS_Connect(char *apn)
{
  if (SIM800_IP() == 0) return (0);     //GPRS aleready connected
    uint8_t trycnt=4;
    char ATstring[64];
    uint16_t Lenth;
lrepGPRS:
    SimpleCmd(AT_SAPBR_TYPE,sizeof(AT_SAPBR_TYPE)-1);
    char *ptr = stradd(ATstring,AT_SAPBR_APN);
    ptr= stradd(ptr,apn);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    if (SimpleCmd(ATstring,ptr-ATstring)!=SIMPLE_CMD_SUCCESS) return (1);
    waitSilent();
    OringBuf_Clear();
    SendStringUsart1WithDMA(AT_SAPBR_ON,sizeof(AT_SAPBR_ON)-1); 
    do    //wait ANSWER
    {
      Lenth=Sim800_WaitLine(incoming, 5000);        //20 sec timeout for GPRS connect
      if (Lenth == 0)
        if (--trycnt > 0) 
          goto lrepGPRS;
         else return(1);
    } while ((strcmp(incoming,ANSW_OK)!=0) && (strcmp(incoming,ANSW_0)!=0));   //bad answer
    if (SIM800_IP() == 0)
    {
      SIM800_Sleep(1);
      vTaskDelay(10);
      SIM800_Sleep(0);
      return (0);
    }
    return (1);
}

uint8_t SIM800_GPRS_Disconnect()
{
  if (SimpleCmd(AT_SAPBR_OFF,sizeof(AT_SAPBR_OFF)-1)==SIMPLE_CMD_SUCCESS)
      return (0);
  return (1);
}
/*----------------------------------------------------------
return 0 if Successfull syncronize Time with NTP server
return 1 if not Success
*/
uint8_t SIM800_CNTP()
{
  uint16_t Lenth;
  SimpleCmd(AT_CNTP_SERV,sizeof(AT_CNTP_SERV)-1);
  SimpleCmd(AT_CNTPCID,sizeof(AT_CNTPCID)-1);
  SimpleCmd(AT_CNTP,sizeof(AT_CNTP)-1);
  do    //wait ANSWER
  {
    Lenth=Sim800_WaitLine(incoming, 5000);  //20 sec timeout
    if (Lenth == 0)  return(1);
    if (strncmp(incoming,_CNTP1,6) == 0) break;
  } while (1);
  if (strcmp(incoming,_CNTP1) == 0)
    return (0); //success
  return (1);
}

/*------------------------------------------------------
*phonenum - pointer of phone number like NULL terminated string
*body - pointerr of NULL terminatedd string
Return 0 if send Successful
return 1 if fail send
*/
uint8_t SIM800_SMSsend(char *phonenum, char *body)
{
  char ATstring[32];
  uint8_t bodyLen=strlen(body);
  uint16_t Lenth;
  *(body + bodyLen) = 26;    //terminator 0 replacing on SUB
  char *ptr = stradd(ATstring,AT_CMGS);
  ptr= stradd(ptr,phonenum);
  ptr= stradd(ptr,AT_SAPBR_SUFF);
  waitSilent();
  OringBuf_Clear();
  SendStringUsart1WithDMA(ATstring,ptr-ATstring);

  uint16_t WaitPrompt=TIME_OUT;
  do
  {
    vTaskDelay(1);
    if ((USART_GetArray(incoming, 1)==1) & (*incoming=='>')) 
      break;
    if (--WaitPrompt == 0)  
      goto  lPrompttimeout;  //didn't wait '>'
  } while(1);
  if (waitSilent() == 0)   return(1);
  OringBuf_Clear();
  SendStringUsart1WithDMA(body,++bodyLen);

  do    //wait ANSWER
  {
    Lenth=Sim800_WaitLine(incoming, 2500);  //10 sec timeout
    if ((Lenth==0) || (strcmp(incoming,ANSW_ERROR) == 0) || (strcmp(incoming,ANSW_SMS_ERROR) == 0 )) 
      return(1); //bad answer
  } while ((strcmp(incoming,ANSW_OK)!=0) & (strcmp(incoming,ANSW_0)!=0));   
  return (0);
  lPrompttimeout:
  *ptr=27;        //escape
  SendStringUsart1WithDMA(ptr,1);
  while (GET_USART1_TC == 0)
     vTaskDelay(1);
  return (1);
}

/*------------------------------------------------------
*AttachName - pointer of text FileName? or NULL if without Attachment
if use Attach - need call SIM800_EMAILattachPacket to added content in Attach
Return 0 if send Successful send EMAIL
return 1 if fail send EMAIL
*/
uint8_t SIM800_EMAILsend(sSys_Param *ParamBlock, char *Subj, char *Body, char *AttachName)
{
  if (SIM800_GPRS_Connect(ParamBlock->APNgprs)==0)
  {
    char tempBuf[64];
    char *ptr = tempBuf;
    uint16_t Lenth;
    SimpleCmd(AT_EMAIL_CID,sizeof(AT_EMAIL_CID)-1);
    SimpleCmd(AT_EMAIL_TO,sizeof(AT_EMAIL_TO)-1);
    SimpleCmd(AT_EMAIL_SSL,sizeof(AT_EMAIL_SSL)-1);
    ptr= stradd(ptr,AT_EMAIL_SMTPSRV);
    ptr= stradd(ptr,ParamBlock->SMTPserv);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    SimpleCmd(tempBuf,ptr-tempBuf);
    ptr=tempBuf;
    ptr= stradd(ptr,AT_EMAIL_AUTH);
    ptr= stradd(ptr,ParamBlock->SMTPname);
    ptr= stradd(ptr,SUFF_LNK);
    ptr= stradd(ptr,ParamBlock->SMTPpassw);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    SimpleCmd(tempBuf,ptr-tempBuf);
    ptr=tempBuf;
    ptr= stradd(ptr,AT_EMAIL_RECIP);
    ptr= stradd(ptr,ParamBlock->EmailHost);
    ptr= stradd(ptr,SUFF_LNK);
    ptr= stradd(ptr,ParamBlock->NameHost);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    SimpleCmd(tempBuf,ptr-tempBuf);
    ptr=tempBuf;
    ptr= stradd(ptr,AT_EMAIL_FROM);
    ptr= stradd(ptr,ParamBlock->EMAILdevice);
    ptr= stradd(ptr,SUFF_LNK);
    ptr= stradd(ptr,ParamBlock->PonyName);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    SimpleCmd(tempBuf,ptr-tempBuf);
    ptr=tempBuf;
    ptr= stradd(ptr,AT_EMAIL_SUB);
    ptr= stradd(ptr,Subj);
    ptr= stradd(ptr,AT_SAPBR_SUFF);
    SimpleCmd(tempBuf,ptr-tempBuf);
    ptr=tempBuf;
    ptr= stradd(ptr,AT_EMAIL_BODY);
    ptr= itoa(strlen(Body),ptr);
    ptr= stradd(ptr,SYM_RN);
    SendStringUsart1WithDMA(tempBuf,ptr-tempBuf); 
    do    //wait 'DOWNLOAD'
    {
      Lenth=Sim800_WaitLine(incoming, 1000);//4 sec timeout
      if (Lenth == 0)  return(1);
    } while (strcmp(incoming,_DNLD) != 0);
    SimpleCmd(Body,strlen(Body));
    if (AttachName != NULL)
    {
      ptr=tempBuf;
      ptr= stradd(ptr,AT_SMTP_FILE);
      ptr= stradd(ptr,AttachName);
      ptr= stradd(ptr,AT_SMTP_FILE_SUFF);
      SimpleCmd(tempBuf,ptr-tempBuf);
      vTaskDelay(100);
      if (SimpleCmd(AT_EMAIL_SEND,sizeof(AT_EMAIL_SEND)-1) == SIMPLE_CMD_SUCCESS)
       do    //wait '+SMTPFT'
        {
          Lenth=Sim800_WaitLine(incoming, 16000);
          if (Lenth == 0) return (1);
          if (strncmp(incoming,AT_SMTP_FT+2,5) == 0) break;
        } while (1);
      if (strncmp(incoming,AT_SMTP_FT+2,7) == 0)
        return(0);
    }
    else
      {//without Attachment
      vTaskDelay(100);
      SendStringUsart1WithDMA(AT_EMAIL_SEND,sizeof(AT_EMAIL_SEND)-1);
      do    //wait 'SMTPSEND 1'
        {
          Lenth=Sim800_WaitLine(incoming, 16000);
          if (Lenth == 0) return (1);
          if (strncmp(incoming,_SMTPSEND,10) == 0) break;
        } while (1);
      if (strcmp(incoming,_SMTPSEND) == 0)
        return (0); //success
    }
  }
  return (1);//non GPRS connect
}

/*------------------------------------------------------
Return 0 if packet send Successful
return 1 if fail send packet
*/
uint8_t SIM800_EMAILattachPacket(uint16_t LenPacket, char *Packet)
{
  char tempBuf[64];
  char *ptr = tempBuf;
  uint16_t Lenth;
  ptr = tempBuf;
  ptr = stradd(ptr,AT_SMTP_FT);
  ptr = itoa(LenPacket,ptr);
  ptr = stradd(ptr,SYM_RN);
  if (LenPacket !=0)
  {
    SendStringUsart1WithDMA(tempBuf,ptr-tempBuf);
    do    //wait '+SMTPFT'
    {
      Lenth=Sim800_WaitLine(incoming, 250);
      if (Lenth==0)  return(1);
    } while (strncmp(incoming,AT_SMTP_FT+2,7) != 0);
    if (SimpleCmd(Packet,LenPacket) == SIMPLE_CMD_SUCCESS)
      return (0);
  }
  else
   if (SimpleCmd(tempBuf,ptr-tempBuf) == SIMPLE_CMD_SUCCESS)
   {
     do    //wait 'SMTPSEND 1'
      {
        Lenth=Sim800_WaitLine(incoming, 10000);
        if (Lenth == 0) return (1);
        if (strncmp(incoming,_SMTPSEND,10) == 0) break;
      } while (1);
    if (strcmp(incoming,_SMTPSEND) == 0)
      return (0); //success
   }
  return (1);//fail send
}


void SIM800_UnreadSMSRead()
  {
    SendStringUsart1WithDMA(AT_CMGL_UNR,sizeof(AT_CMGL_UNR)-1);
  }

void SIM800_DeleteReadedSMS()
{  
  SimpleCmd(AT_CMGDR,sizeof(AT_CMGDR)-1);
}

/*----------------------------------------------------
return 0 - if reading +CMGL and recognize Phone Number
return 1 - if reading corrupt +CMGL string
return 3 - if timeout
return 2 - if reading another string

*/
uint8_t SIM800_readSMSflow (char **Buff, char **PhoneNum)
{
  char *ptr, *ptr2;
  uint16_t Lenth;
  *Buff=incoming;
  Lenth=Sim800_WaitLine(incoming, 250);
  if (Lenth==0)  return(3);//timeout
  if (strncmp(incoming,_CMGL,6) == 0)
    {
    ptr=strchr(incoming,'"'); // search 1st "
    if (ptr == NULL) return (1);
    ptr=strchr(++ptr,'"'); // search 2st "
    if (ptr == NULL) return (1);
    ptr=strchr(++ptr,'"'); // search 3st "
    if (ptr == NULL) return (1);
    ptr2=strchr(++ptr,'"'); // search 4st "
    if (ptr2 == NULL) return (1);
    *ptr2=0x00;
    *PhoneNum=ptr;
    return (0);//success
    }
  return (2);
}

void SIM800_PlayStart(char *amrname)
{
  char tempBuf[48];
  char *ptr = tempBuf;
  ptr = stradd(ptr,AT_CREC4);
  ptr = stradd(ptr,amrname);
  ptr = stradd(ptr,AT_CREC4_suff);
  SendStringUsart1WithDMA(tempBuf,strlen(tempBuf));
  while (GET_USART1_TC == 0)
     vTaskDelay(1);
  return;
}

uint8_t SIM800_Call(char *phonenum)
{
  uint16_t Lenth;
  char CmdBuf[32];
  char *ptr = CmdBuf;
  ptr = stradd(ptr,ATD);
  ptr = stradd(ptr,phonenum);
  ptr = stradd(ptr,ATD_SUFF);
  SendStringUsart1WithDMA(CmdBuf,strlen(CmdBuf));
  do    //wait answer
    {
      Lenth=Sim800_WaitLine(incoming, 2500);  //10sec
      if (Lenth == 0) return (255);
      if ((strcmp(incoming,ANSW_OK) == 0) || (strcmp(incoming,ANSW_0) == 0)) return (0);
      if (strcmp(incoming,ANSW_NO_CARRIER) == 0) return (3);
      if (strcmp(incoming,ANSW_ERROR) == 0) return (4);
      if (strcmp(incoming,ANSW_NODIAL) == 0) return (6);
      if (strcmp(incoming,ANSW_BUSY) == 0) return (7);
    } while (1);
}

uint8_t SIM800_CallHold()
{
 return (SimpleCmd(AT_CHLD2,sizeof(AT_CHLD2)-1));
}

uint8_t SIM800_CallsConf()
{
 return (SimpleCmd(AT_CHLD3,sizeof(AT_CHLD3)-1));
}         

//return GSM signal or 0 if fault
uint8_t Sim800_GetGSMsignal()
{
uint16_t Lenth;
uint8_t retval;
  if (waitSilent()==0)  return (0);   //permanent full Rx buffer
  OringBuf_Clear();
  SendStringUsart1WithDMA(AT_CSQ,sizeof(AT_CSQ)-1); 
  
 //this cycle take all answer and compare first 5 symbol "+CSQ"
 do 
 {
   Lenth=Sim800_WaitLine(incoming, TIME_OUT);
   if (Lenth == 0)    return (255);   //answer timeout
 } while (strncmp(incoming,AT_CSQ+2,4)!=0);
 char *ptr=strchr(incoming,':'); // search ,
 if (ptr == NULL)    return(255); //non correct answer
 retval=(uint8_t)atoi(++ptr);
 Lenth=Sim800_WaitLine(incoming, TIME_OUT);
 return (retval);
}

 uint8_t SIM800_PowerDown()
 {
   uint16_t Lenth;
   if (waitSilent()==0)  return (0);   //permanent full Rx buffer
   OringBuf_Clear();
   SendStringUsart1WithDMA(AT_POWERDOWN,sizeof(AT_POWERDOWN)-1); 
   do 
   {
     Lenth=Sim800_WaitLine(incoming, TIME_OUT);
     if (Lenth == 0)    return (1);   //answer timeout
   } while (strcmp(incoming,_NORM_PD) != 0);
 return (0);
 }

uint8_t SIM800_DisableRF()
{
 uint16_t Lenth;
 SendStringUsart1WithDMA(AT_CFUN0,sizeof(AT_CFUN0)-1);
 do 
   {
     Lenth=Sim800_WaitLine(incoming, 2000);
     if (Lenth == 0)    return (1);   //answer timeout
   } while ((strcmp(incoming,ANSW_OK)!=0) && (strcmp(incoming,ANSW_0)!=0));
 return (0);
} 

