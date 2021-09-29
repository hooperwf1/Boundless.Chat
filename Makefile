IDIR = include
SDIR = src
ODIR = obj
CC=gcc
CFLAGS=-I$(IDIR) -lpthread -Wall -Werror -Wextra -g -Wno-format-truncation -lssl -lcrypto -lmenu -lncursesw

_OBJS=main.o logging.o linkedlist.o hstring.o communication.o array.o tui.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c $(IDIR)/%.h
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	rm -f obj/*.o client
