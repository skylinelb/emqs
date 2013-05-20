#include <string.h>
#include <stdio.h>

int short2buff(short value, char *buff)
{
    char tmp[2];

    tmp[0] = value & 0x0FF;
    tmp[1] = (value >> 8)  & 0x0FF;
    memcpy(buff, tmp, 2);

    return 2;
}

short buff2short(char *buff)
{
 
    return ((buff[1] & 0xFF) << 8) | (buff[0] & 0xFF);
}

int int2buff(int value, char *buff)
{

    buff[0] = value & 0x0FF;
    buff[1] = (value >> 8)  & 0x0FF;
    buff[2] = (value >> 16) & 0x0FF;
    buff[3] = (value >> 24) & 0x0FF;

    return 4;
}

int buff2int(char *buff)
{
    int  value = 0;
 
    value = buff[3];
    value = (value << 8) | (buff[2] & 0xFF);
    value = (value << 8) | (buff[1] & 0xFF);
    value = (value << 8) | (buff[0] & 0xFF);

    return value;
}

int long2buff(long value, char *buff)
{
    buff[0] = value & 0x0FF;
    buff[1] = (value >> 8)  & 0x0FF;
    buff[2] = (value >> 16) & 0x0FF;
    buff[3] = (value >> 24) & 0x0FF;

    return 4;
}

long buff2long(char *buff)
{
    long value = 0;

    value = buff[3];
    value = (value << 8) | (buff[2] & 0xFF);
    value = (value << 8) | (buff[1] & 0xFF);
    value = (value << 8) | (buff[0] & 0xFF);

    return value;
}

int string2buff(char *string, char *buff)
{
    int size;
    size = int2buff(strlen(string), buff); 
    memcpy(buff+size,string,strlen(string));
    return size + strlen(string);
}

int buff2string(char *buff, char *string)
{
    int len;
    len = buff2int(buff);
    memcpy(string, buff+4,len);
    string[len]=0;

    return len+4;
}
