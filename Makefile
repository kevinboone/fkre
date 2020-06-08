NAME    := fkre
VERSION := 0.1a
LIBS    := ${EXTRA_LIBS} 
KLIB    := klib
KLIB_INC := $(KLIB)/include
KLIB_LIB := $(KLIB)
TARGET	:= $(NAME)
SOURCES := $(shell find src/ -type f -name *.c)
OBJECTS := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	:= $(OBJECTS:.o=.deps)
DESTDIR := /
PREFIX  := /usr
MANDIR  := $(DESTDIR)/$(PREFIX)/share/man
BINDIR  := $(DESTDIR)/$(PREFIX)/bin
SHARE   := $(DESTDIR)/$(PREFIX)/share/$(TARGET)
CFLAGS  := -fpie -fpic -Wall -Werror -DNAME=\"$(NAME)\" -DVERSION=\"$(VERSION)\" -DSHARE=\"$(SHARE)\" -DPREFIX=\"$(PREFIX)\" -I $(KLIB_INC) ${EXTRA_CFLAGS}
LDFLAGS := -pie ${EXTRA_LDFLAGS}

$(TARGET): $(OBJECTS) 
	make -C klib
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS) $(KLIB)/klib.a

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

clean:
	$(RM) -r build/ $(TARGET) 
	make -C klib clean

install: $(TARGET)
	mkdir -p $(DESTDIR)/$(PREFIX) $(DESTDIR)/$(BINDIR) $(DESTDIR)/$(MANDIR)
	strip $(TARGET)
	install -m 755 $(TARGET) $(DESTDIR)/${BINDIR}
	mkdir -p $(DESTDIR)/$(MANDIR)/man1
	cp -p man1/* $(DESTDIR)/${MANDIR}/man1/

-include $(DEPS)

.PHONY: clean

