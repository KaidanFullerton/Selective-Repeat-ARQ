TARGETS=sender receiver
CFLAGS = -O0 -g -Wall -Wvla -Werror -Wno-error=unused-variable

all: $(TARGETS)

sender: sender.c GoBackN.c
	gcc $(CFLAGS) -o sender sender.c GoBackN.c -lpthread

receiver: receiver.c GoBackN.c
	gcc $(CFLAGS) -o receiver receiver.c GoBackN.c -lpthread

clean:
	rm -f $(TARGETS)
