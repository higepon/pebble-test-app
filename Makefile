TARGET  = ./build/pebble-test-app.pbw
SOURCES = ./src/pebble-test-app.c

OBJECTS = ./build/chalk/pebble-app.elf

all : $(TARGET)

$(TARGET): ./src/pebble-test-app.c
	pebble build

install : $(TARGET)
	pebble install --emulator chalk

install-cloud : $(TARGET)
	pebble install --cloudpebble

clean:
	pebble clean
