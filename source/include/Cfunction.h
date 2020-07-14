#ifndef _C_FUNCTION_
#define _C_FUNCTION_

void removeSpaces(char *str1);
char *itoa(int16_t n, char *s);
uint32_t CRCcalc( void *pStorage, uint16_t Size);
void debugprint (const char *toSWO);
char *stradd (char * destptr, const char * srcptr );
void strrepl(char *src, char fnd, char rep);

#endif //_C_FUNCTION_