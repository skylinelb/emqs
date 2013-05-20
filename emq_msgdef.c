#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

void toupstr(const char *sstr, char *dstr);

int main(int argc, char **argv)
{
    int i;
    int fd_h,fd_c;
    char msg_name_h[128];
    char msg_name_c[128];
    char buff[1024];
    char up_msg_name[128];
    int  cin;
    int  option_num; 
    int  func_no; 
    int  msg_pri;
    char req_struct_name[64];
    char rep_struct_name[64];

    if(argc < 2) {
        printf("usage:%s <msg_name>\n",argv[0]);
        exit(0);
    }

    /* check *.h *.h file */
    sprintf(msg_name_h, "%s.h",argv[1]);
    sprintf(msg_name_c, "%s.c",argv[1]);
    fd_h = open(msg_name_h, O_RDWR | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd_h < 0) {
        printf("message [%s] may be defined before,please take a other name\n",argv[1]);
        exit(0);
    }

    fd_c = open(msg_name_c, O_RDWR | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd_c < 0) {
        close(fd_h);
        printf("message [%s] may be defined already,please take a other one\n", argv[1]);
        exit(0);
    }

    /* .c file header */
  
    sprintf(buff, "#include <stdio.h>\n#include \"%s\"\n\n", msg_name_h);
    write(fd_c, buff, strlen(buff));

    /* .h file header */ 
    toupstr(argv[1], up_msg_name);
    sprintf(buff,"#ifndef _EMQ_MSG_%s_INCLUDE\n", up_msg_name);
    write(fd_h, buff, strlen(buff));
    sprintf(buff,"#define _EMQ_MSG_%s_INCLUDE\n", up_msg_name);
    write(fd_h, buff, strlen(buff));
    write(fd_h, "\n#include \"emq_message.h\"\n",26);

    /* define option */
    while(1) {
        printf("input operation num:");
        scanf("%d", &cin);
        if(cin < 0) { 
            printf("the %d operation is wrong, >= 1\n", cin);
            continue;
        }
        else break;
    }
    option_num = cin;

    write(fd_h,"\nenum{\n", 7);
    for(i = 0; i < option_num - 1; i++) {
        sprintf(buff, "    %s_OPERATION%d = %d,\n",up_msg_name,i,i);
        write(fd_h, buff, strlen(buff));
    }
    sprintf(buff, "    %s_OPERATION%d = %d\n",up_msg_name,i,i);
    write(fd_h, buff, strlen(buff));
    write(fd_h, "};\n\n", 3);

    /* define func no */
    while(1) {
        printf("input func no:");
        scanf("%d", &cin);
        if(cin % 2 != 0) {
            printf("func no %d is wrong, func_no%2!=0\n", cin);
        }else break;
    }
    func_no = cin; 
    sprintf(buff, "enum {\n    %s_FUNCTION_REQ = %d,\n    %s_FUNCTION_REP = %d\n};\n\n", \
            up_msg_name, func_no, up_msg_name,func_no+1);
    write(fd_h, buff, strlen(buff));


    /* struct of request */
    sprintf(req_struct_name, "EmqMsg_%s_Req", up_msg_name);
    sprintf(buff, "typedef struct _emq_msg_%s_req{\n    \/* add member of this struct *\/"
                   "\n\n}%s;\n\n", argv[1], req_struct_name);
    write(fd_h, buff, strlen(buff));

    /* struct of response */
    sprintf(rep_struct_name, "EmqMsg_%s_Rep", up_msg_name);
    sprintf(buff, "typedef struct _emq_msg_%s_rep{\n    \/* add member of this struct *\/"
                  "\n\n}%s;\n\n", argv[1], rep_struct_name);
    write(fd_h, buff, strlen(buff));

    /* create msg fun */
    printf("input msg priority:");
    scanf("%d", &msg_pri);
    sprintf(buff, "#define EMQ_MSG_%s_PRIORITY %d\n", up_msg_name, msg_pri);
    write(fd_h,buff,strlen(buff));
  
    sprintf(buff, "EmqMessage *emqMsgCreate_%s(int fun, int oper_no, int flag)", up_msg_name);
    write(fd_h,buff,strlen(buff));
    write(fd_h,";\n\n", 3);
    write(fd_c, buff, strlen(buff));
    sprintf(buff, "\n{\n    if(fun == %s_FUNCTION_REQ)\n        return emqMessageNew(1,fun,"
            "oper_no,flag,%d, sizeof(%s));\n",up_msg_name, msg_pri,req_struct_name);
    write(fd_c, buff, strlen(buff));
    sprintf(buff, "    else if(fun == %s_FUNCTION_REP)\n        return emqMessageNew(1,fun,"
            "oper_no,flag,%d, sizeof(%s));\n",up_msg_name, msg_pri,rep_struct_name);
    write(fd_c, buff, strlen(buff));
    sprintf(buff, "%s", "    else \n        return NULL;\n}\n\n");
    write(fd_c, buff, strlen(buff));

    /* define stream lize func */
    sprintf(buff, "char *%sReqStreamlize(%s *data, int *len)", up_msg_name, req_struct_name);
    write(fd_h, buff, strlen(buff));
    write(fd_h, ";\n", 2);
    write(fd_c, buff, strlen(buff));
    write(fd_c, "\n{\n\n}\n\n", 7);

    sprintf(buff, "%s *%sReqRestreamlize(char *buff, int len)", req_struct_name, up_msg_name);
    write(fd_h, buff, strlen(buff));
    write(fd_h, ";\n", 2);
    write(fd_c, buff, strlen(buff));
    write(fd_c, "\n{\n\n}\n\n", 7);

    sprintf(buff, "char *%sRepStreamlize(%s *data, int *len)", up_msg_name, rep_struct_name);
    write(fd_h, buff, strlen(buff));
    write(fd_h, ";\n", 2);
    write(fd_c, buff, strlen(buff));
    write(fd_c, "\n{\n\n}\n\n", 7);

    sprintf(buff, "%s *%sRepRestreamlize(char *buff, int len)", rep_struct_name, up_msg_name);
    write(fd_h, buff, strlen(buff));
    write(fd_h, ";\n", 2);
    write(fd_c, buff, strlen(buff));
    write(fd_c, "\n{\n\n}\n\n", 7);

   
    write(fd_h, "\n\n", 2);
    for(i = 0; i < option_num; i++)  {
        sprintf(buff, "int %sReqCallbackFunc%d(EmqMessage *reqMsg, EmqMessage *repMsg)", \
                up_msg_name , i+1);
        write(fd_h, buff, strlen(buff));
        write(fd_h, ";\n", 2);
        write(fd_c, buff, strlen(buff));
        write(fd_c, "\n{\n\n}\n\n", 7);

        sprintf(buff, "int %sRepCallbackFunc%d(EmqMessage *reqMsg, EmqMessage *repMsg)", \
                up_msg_name, i+1);
        write(fd_h, buff, strlen(buff));
        write(fd_h, ";\n", 2);
        write(fd_c, buff, strlen(buff));
        write(fd_c, "\n{\n\n}\n\n", 7);
    }

    /* define process flag */
    sprintf(buff, "\n#define %s_OPERATION_FLAG_DEFINE  ", up_msg_name); 
    write(fd_h, buff, strlen(buff));
    for(i = 0; i < option_num; i++) {
        write(fd_h, "\\\n", 2);
        sprintf(buff, "    EMQ_GETFLAG_DEFINE(1,%s_FUNCTION_REQ," \
                "%s_OPERATION%d,0) ", up_msg_name, up_msg_name, i);
        write(fd_h, buff, strlen(buff));
    }
  
    /* define streamlize func */
    sprintf(buff, "\n\n#define %s_STREAMLIZE_REQ_DEFINE \\\n" \
            "    EMQ_MESSAGE_STREAMLIZE_REQ_DEFINE(1,%s_FUNCTION_REQ, \\\n" \
            "    %sReqStreamlize,%s)\n\n", up_msg_name, up_msg_name,up_msg_name,req_struct_name);
    write(fd_h, buff, strlen(buff));

    sprintf(buff, "#define %s_RESTREAMLIZE_REQ_DEFINE \\\n" \
            "    EMQ_MESSAGE_RESTREAMLIZE_REQ_DEFINE(1,%s_FUNCTION_REQ, \\\n" \
            "    %sReqRestreamlize,%s)\n\n", up_msg_name, up_msg_name,up_msg_name,req_struct_name);
    write(fd_h, buff, strlen(buff));

    sprintf(buff, "#define %s_STREAMLIZE_REP_DEFINE \\\n" \
            "    EMQ_MESSAGE_STREAMLIZE_REP_DEFINE(1,%s_FUNCTION_REP, \\\n" \
            "    %sRepStreamlize,%s)\n\n", up_msg_name, up_msg_name,up_msg_name,rep_struct_name);
    write(fd_h, buff, strlen(buff));

    sprintf(buff, "#define %s_RESTREAMLIZE_REP_DEFINE \\\n" \
            "    EMQ_MESSAGE_RESTREAMLIZE_REP_DEFINE(1,%s_FUNCTION_REP, \\\n" \
            "    %sRepRestreamlize,%s)\n\n", up_msg_name, up_msg_name,up_msg_name,rep_struct_name);
    write(fd_h, buff, strlen(buff));

    /* define callback func */
    for(i = 0; i < option_num; i++) {
        sprintf(buff, "#define %s_REQ_CALLBACKFUNC%d_RDEFINE \\\n" \
                "    EMQ_MESSAGE_REQ_CALLBACKFUNC_DEFINE(1,%s_FUNCTION_REQ, \\\n" \
                "    %s_OPERATION%d,%sReqCallbackFunc%d)\n\n",up_msg_name, i+1, up_msg_name, \
                up_msg_name, i, up_msg_name, i+1);
        write(fd_h, buff, strlen(buff));

        sprintf(buff, "#define %s_REP_CALLBACKFUNC%d_RDEFINE \\\n" \
                "    EMQ_MESSAGE_REP_CALLBACKFUNC_DEFINE(1,%s_FUNCTION_REP, \\\n" \
                "    %s_OPERATION%d,%sRepCallbackFunc%d)\n\n",up_msg_name, i+1, up_msg_name, \
                up_msg_name, i, up_msg_name, i+1);
        write(fd_h, buff, strlen(buff));
    }
     
    write(fd_h,"\n#endif", 7);
error:
    close(fd_h);
    close(fd_c);
}

void toupstr(const char *sstr, char *dstr)
{
   int i; 

   for(i = 0; i < strlen(sstr); i++) {
       dstr[i] = toupper(sstr[i]);
   }
   dstr[i] = 0;

   return;
}
