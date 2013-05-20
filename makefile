CC= cc -g

BINNAME1=server
BINNAME2=client
BINNAME3=client1
BINNAME4=emqmsgdef
MSGOBJ=message_example1.o
MSGOBJECT=message_example1.o 
OBJECT=emq_message.o emq_typeblock.o emq_msgheader.o emq_streamlize.o emq_socket.o emq_nio.o \
       emq_global.o emq_log.o emq_queue.o emq_reqproc.o emq_register.o emq_synclink.o \
       emq_thread.o hello.o
         
FLAGS=-g -lpthread

all:$(BINNAME1) $(BINNAME2) $(BINNAME3) $(BINNAME4) clean

$(BINNAME1):${OBJECT} ${MSGOBJECT} emq_main.o
	$(CC) $(FLAGS) -o $@ $?

$(BINNAME2):${OBJECT} ${MSGOBJECT} emq_client.o
	$(CC) $(FLAGS) -o $@ $?

$(BINNAME3):${OBJECT} ${MSGOBJECT} emq_client1.o
	$(CC) $(FLAGS) -o $@ $?

$(BINNAME4):emq_msgdef.o
	$(CC) $(FLAGS) -o $@ $?

clean:
	rm -rf *.o

.SUFFIXES: .c .o

.c.o:
	$(CC) -c $<
