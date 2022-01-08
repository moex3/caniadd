InstallPrefix := /usr/local/bin

PROGNAME := caniadd
VERSION := 1
CFLAGS := -Wall -std=gnu11 #-march=native #-Werror
CPPFLAGS := -DCBC=0 -DCTR=0 -DECB=1 -Isubm/tiny-AES-c/ -Isubm/md5-c/ -Isubm/MD4/ -DPROG_VERSION='"$(VERSION)"'
LDFLAGS := -lpthread -lsqlite3

SOURCES := $(wildcard src/*.c) subm/tiny-AES-c/aes.c subm/md5-c/md5.c subm/MD4/md4.c #$(TOML_SRC) $(BENCODE_SRC)
OBJS := $(SOURCES:.c=.o)

all: CFLAGS += -O3 -flto
all: CPPFLAGS += #-DNDEBUG Just to be safe
all: $(PROGNAME)

# no-pie cus it crashes on my 2nd pc for some reason
dev: CFLAGS += -Og -ggdb -fsanitize=address -fsanitize=leak -fstack-protector-all -no-pie
dev: $(PROGNAME)

t:
	echo $(SOURCES)

install: $(PROGNAME)
	install -s -- $< $(InstallPrefix)/$(PROGNAME)

uninstall:
	rm -f -- $(InstallPrefix)/$(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) -o $@ $+ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

clean:
	-rm -- $(OBJS) $(PROGNAME)

re: clean all
red: clean dev
