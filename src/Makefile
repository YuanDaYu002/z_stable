HOST=arm-hisiv300-linux
#HOST=arm-hisiv100nptl-linux
CC = $(HOST)-gcc
CPP= $(HOST)-g++
TR = $(HOST)-strip
AR = $(HOST)-ar
include ./version
CFLAGS   = -g -Wall -D_FILE_OFFSET_BITS=64 -fPIC -DP2P_LIB_VERSION=$(LIB_VERSION)
CPPFLAGS := -I../include -I./pjsip/include -I../zmdnetlib/include -I../include -I../include/jpg_include -I../include/normal_api -I../include/his_api -I../include/gui -I../include/parameter -I../include/encode -I../include/decode -I../include/filemanage -I../include/record -I../include/App -I../include/upgrade -I../include/peripheral -I../include/alarm -I../include/logmanage/ -I../include/httpd/include -I../include/net -I/include -I../include/ptz -I../zmdnetlib/tcplib/include -I../include/zmdnetlib/ -I../zmdnetlib/zmdnetlib/src -I./json -I./httpclient -I./curl/ -I../include/wifi -I../zmdnetlib/include/zmdnetlib -I../zmdnetlib/include/tcplib/ -I../ -I../include/his_api/mpp/ -I./tls -I./nopoll -I./ws -I./ -I./rtmp
LIBS	= -DPJ_AUTOCONF=1	-DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1
LIBDIR = ../lib/meshare
BIN      = zdemo
ZLIB_SO  = libmeshare.so
ZLIB_A   = libmeshare.a
CPPSRCS = $(wildcard *.cpp)
CPPSRCS += $(wildcard ws/*.cpp)
CPPOBJS := $(patsubst %.cpp, ./obj/%.o, $(CPPSRCS))
./obj/%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@ $(CPPFLAGS) $(LDFLAGS) $(LIBS) $(STATICLIBS)

all:$(CPPOBJS)
	$(AR) -r $(ZLIB_A) ./obj/*.o ./obj/ws/*.o
	#cp $(ZLIB_A) ../lib/meshare

clean:
	-rm -f $(BIN)
	-rm -f $(ZLIB_SO)
	-rm -f $(ZLIB_A)
	-rm -f $(CPPOBJS)
	-rm -f obj/ws/*.o

