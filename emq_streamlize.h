#ifndef _EMQ_STREAMLIZE_INCLUDE
#define _EMQ_STREAMLIZE_INCLUDE

extern int short2buf(short value, char *buff);
extern short buff2short(char *buff);
extern int int2buf(int value, char *buff);
extern int buff2int(char *buff);
extern int long2buf(long value, char *buff);
extern long buff2long(char *buff);
extern int string2buff(char *string, char *buff);
extern int buff2string(char *buff, char *string);

#endif
