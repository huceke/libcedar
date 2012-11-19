include Makefile.include

PREFIX ?= /usr

INCLUDES+=-Isrc/

SRC  = 	src/CedarBitstreamBufferManager.cpp \
				src/CedarFrameBufferManager.cpp \
				src/CedarDecoder.cpp \
				src/CedarVEHwControll.cpp \

OBJ	=$(filter %.o,$(SRC:.cpp=.o))

LIBS += -L./ -lpthread

TARGET = libcedar.so

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

all: $(TARGET)

$(TARGET): libs 

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)

libs: $(OBJ)
	$(CC) -rdynamic -shared -ldl -L./ -Wl,-soname,libcedar.so -Wl,-Bsymbolic \
		-Wl,--export-dynamic -o libcedar.so src/*.o  -fvisibility=hidden \
		-Wl,-version-script=src/Version_script-dec 

install:
	install -d -m 755 $(PREFIX)/include/libcedar
	install -d -m 755 $(PREFIX)/lib
	install -m 666 src/CedarDecoder.h $(PREFIX)/include/libcedar/
	install -m 755 libcedar.so $(PREFIX)/lib/
