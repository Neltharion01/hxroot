SOURCES    := $(wildcard src/*.c)
CXXSOURCES := $(wildcard src/*.cpp)
OBJS       := $(SOURCES:.c=.o) $(CXXSOURCES:.cpp=.o)
DEPS       := $(OBJS:.o=.d)
CFLAGS     := $(INCLUDES) -MMD -MP -pipe -fPIC -Wall -Wextra -Wno-unused-parameter -Wno-missing-attributes

RESULT     := libhxroot.so

DEBUG := 1
ifeq ($(DEBUG),1)
    CFLAGS += -g -Og
else
    CFLAGS += -Os
endif

all: $(RESULT)

-include $(DEPS)

$(RESULT): $(OBJS)
	$(CC) -shared -o $(RESULT) $(OBJS)

clean:
	rm -f $(RESULT) $(OBJS) $(DEPS)

.PHONY: all clean
