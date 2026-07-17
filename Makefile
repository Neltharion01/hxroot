SOURCES    := $(wildcard src/*.c)
CXXSOURCES := $(wildcard src/*.cpp)
OBJS       := $(SOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)
DEPS       := $(OBJS:.o=.d)
LINTS      := -Wall -Wextra -Wno-unused-parameter -Wno-missing-attributes -Wno-sign-compare
CFLAGS     += $(INCLUDES) -MMD -MP -pipe -fPIC -D_FORTIFY_SOURCE=2 $(LINTS)

# When building a package, set GLIBC_PREFIX instead of PREFIX (already set on termux-glibc)
GLIBC_PREFIX ?= /usr

DEBUG := 1
ifeq ($(DEBUG),1)
    CFLAGS += -g -Og
else
    CFLAGS += -Os
endif

all: libhxroot.so hxroot

-include $(DEPS)

libhxroot.so: $(OBJS)
	$(CC) -shared -o libhxroot.so $(OBJS)

hxroot: hxroot.in
	LIBHXROOT="$(CURDIR)/libhxroot.so" sh hxroot.in >hxroot
	chmod +x hxroot

install: libhxroot.so
	mkdir -p "$(DESTDIR)/$(GLIBC_PREFIX)/bin"
	LIBHXROOT="$(GLIBC_PREFIX)/lib/libhxroot.so" sh hxroot.in >"$(DESTDIR)/$(GLIBC_PREFIX)/bin/hxroot"
	chmod 755 "$(DESTDIR)/$(GLIBC_PREFIX)/bin/hxroot"
	mkdir -p "$(DESTDIR)/$(GLIBC_PREFIX)/lib"
	install -m755 libhxroot.so "$(DESTDIR)/$(GLIBC_PREFIX)/lib/libhxroot.so"
	install -m755 hxconvert "$(DESTDIR)/$(GLIBC_PREFIX)/bin/hxconvert"
	install -m755 hxunconvert "$(DESTDIR)/$(GLIBC_PREFIX)/bin/hxunconvert"

uninstall:
	rm -f "$(GLIBC_PREFIX)/bin/hxroot"
	rm -f "$(GLIBC_PREFIX)/bin/hxconvert"
	rm -f "$(GLIBC_PREFIX)/bin/hxunconvert"
	rm -f "$(GLIBC_PREFIX)/lib/libhxroot.so"

clean:
	rm -f libhxroot.so hxroot src/*.o src/*.d

.PHONY: all clean install
