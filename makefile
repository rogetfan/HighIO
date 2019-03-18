# flags
CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -lws2_32
LFLAGS = -lws2_32
# args
RELEASE = 0
BITS = 64
CPU = default # ryzen1,ryzen2,default
# files
TARGET = highio
OBJS = asynet.o client.o main.o 
# in posix system , it is rm -rf
RM = del /f

# [Binary Type] = debug:0,realse:1. 
ifeq ($(RELEASE),0)
    # debug
    CFLAGS += -g -Wno-unused-but-set-variable
	LFLAGS += -g -Wno-unused-but-set-variable
else
    # release
    CFLAGS += -static -O3 -DNDEBUG
    LFLAGS += -static
endif

# [System Type] = 32: 32 bit system, 64:64 bit system.
ifeq ($(BITS),32)
    CFLAGS += -m32
    LFLAGS += -m32
else
    ifeq ($(BITS),64)
        CFLAGS += -m64
        LFLAGS += -m64
    else
    endif
endif

#[CPU Type]
ifeq ($(CPU),ryzen1)
	CFLAGS += -march=znver1
	LFLAGS += -march=znver1
else
	ifeq ($(CPU),ryzen2)
		CFLAGS += -march=znver2
		LFLAGS += -march=znver2
	else
		CFLAGS += -march=native
		LFLAGS += -march=native
	endif
endif

.PHONY : all clean

all : $(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LFLAGS)


$(OBJS):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	$(RM) $(OBJS) $(addsuffix .exe,$(TARGET))