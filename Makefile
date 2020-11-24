TARGETS=sender receiver
CFLAGS = -O0 -g -Wall -Wvla -Werror -Wno-error=unused-variable

all: $(TARGETS)

sender: sender.c SelectiveRepeat.c
	gcc $(CFLAGS) -o sender sender.c SelectiveRepeat.c -lpthread

receiver: receiver.c SelectiveRepeat.c
	gcc $(CFLAGS) -o receiver receiver.c SelectiveRepeat.c -lpthread

clean:
	rm -f $(TARGETS)
