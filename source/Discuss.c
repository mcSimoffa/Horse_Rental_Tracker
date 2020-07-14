uint8_t Discuss(uint8_t *PendingFlags, *callparam)
{
	uint8_t	State;
	uint32_t timeLimit;
	switch (State)
	{
	uint16_t quant=USART_GetLine(incoming)	
	case STATE_WAIT:
	if (quant>0)
		if (strncmp(incoming,AT_CLIP+2,5)==0)   //+CLIP detection
		{
			SendStringUsart1WithDMA(ATA,sizeof(ATA)-1);
			timeLimit +=  xTaskGetTickCount()+250;  //1 sec or wait 'OK'
			State=STATE_WAIT_ATA_RESPONSE;
		}
	break;
	//-----------------------------------------------------
	
	case STATE_WAIT_ATA_RESPONSE:
	if ((strcmp(incoming,ANSW_OK)==0) | (strcmp(incoming,ANSW_0)==0))
		State=STATE_WHO_ACTIVE;
	if (xTaskGetTickCount() > timeLimit)
		State=STATE_WAIT;
	break;
	//--------------------------------------------------------
	
	case STATE_WHO_ACTIVE:
	SIM800_EventDTMF (callparam);
	SendStringUsart1WithDMA(AT_CLCC,sizeof(AT_CLCC)-1);
	timeLimit +=  xTaskGetTickCount()+250;  //1 sec or wait CLCC response
	State = State=STATE_WAIT_CLCC_RESPONSE;
	break;
	//--------------------------------------------------------
	
	case STATE_WAIT_CLCC_RESPONSE:
	SIM800_EventDTMF (callparam);
	if ((strcmp(incoming,ANSW_OK)==0) | (strcmp(incoming,ANSW_0)==0))
	{
		memset (callparam,0x00,SIZE_ACTIVECALL);
		State=STATE_WAIT;
		break;
	}
    if (strncmp(incoming,AT_CLCC+2,5)==0);
	{
		ptr=strchr(incoming,'"'); // search 1st "
		if (ptr!=NULL)
		{
			ptr2=strchr(++ptr,'"'); // search 2nd "
			if (ptr2!=NULL)
			{
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
				if (ptr++!=NULL)
				{
					ptr2=strchr(ptr,','); // search 2nd "
					if (ptr2++!=NULL)
					{
						if ((*ptr=='1') & (*ptr2=='4')) callparam->Status=STATUS_DIR_INCOMING;
							else if ((*ptr=='1') & (*ptr2=='0')) callparam->Status=STATUS_DIR_ACTIVE_IN;
								else if ((*ptr=='0') & (*ptr2=='0')) callparam->Status=STATUS_DIR_ACTIVE_OUT;          
								else  callparam->Status=STATUS_DIR_NONE;
					}
					
				}
			}
		}
	}
	if (callparam->AdminNum>0)
		State=STATE_ACTIVE_ADMIN;
		else
			State=STATE_ACTIVE_GUEST;
	break;
	



case STATE_USERMODE:	
		
		
	}
}

void SIM800_eventDTMF(sActiveCall *callparam)
{
 if (strncmp(incoming,_DTMF,sizeof(_DTMF)-1)==0)   //+DTMF detection
	if (callparam->headDTMF < SIZE_DTMF_BUF)
	{
	  char DTMFchar=*(incoming+7);
	  if (DTMFchar=='#')
		  callparam->headDTMF=0;
	   else
		   callparam->AccumDTMF[(callparam->headDTMF)++] = DTMFchar;
	  callparam->AccumDTMF[(callparam->headDTMF)] = 0x00;
	}
}
	