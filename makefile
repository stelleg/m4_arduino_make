# Simple makefile for developing for grandcentral metro m4
# You'll need versions of the following tools
cxx = arm-none-eabi-g++ 
cc = arm-none-eabi-gcc 
objcopy = arm-none-eabi-objcopy 
bossac = bossac 

# Once you have them, double tap reset to make it ready to accept a new
# firmware, then run make (<your_program>.upload), e.g. make
# examples/blink.upload

# For libraries, download them into libs/libraries and list them here
extras := Adafruit_NeoPixel
libdir := $(patsubst %/makefile, %/libs, $(abspath $(lastword $(MAKEFILE_LIST))))
extras_dirs := $(patsubst %, ${libdir}/libraries/%/, ${extras})
extras_inc := $(patsubst %, -I%, ${extras_dirs})
extras_libs := $(shell find ${extras_dirs} -path */examples -prune -o -name "*.cpp" -print) 
libs := $(shell find libs/cores libs/variants -type f \( -name '*.c' -o -name '*.cpp' \)) ${extras_libs}
libos := $(patsubst %, %.o, $(libs))

all: examples/blink.bin

# Object files used by 
ldflags=\
  -Os \
  -Wl,--gc-sections \
  -save-temps \
  -T${libdir}/variants/grand_central_m4/linker_scripts/gcc/flash_with_bootloader.ld \
  --specs=nano.specs \
  --specs=nosys.specs \
  -mcpu=cortex-m4 \
  -mthumb \
  -Wl,--cref \
  -Wl,--check-sections \
  -Wl,--gc-sections \
  -Wl,--unresolved-symbols=report-all \
  -Wl,--warn-common \
  -Wl,--warn-section-align \
  -Wl,--start-group \
  -L${libdir}/CMSIS/4.5.0/CMSIS/Lib/GCC/ \
  -larm_cortexM4lf_math \
  -lm \
  -Wl,--end-group \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \

ccflags=\
	-mcpu=cortex-m4 \
	-mthumb \
	-Os \
	-w \
	-ffunction-sections \
	-fdata-sections \
	-fno-threadsafe-statics \
	-nostdlib \
	--param max-inline-insns-single=500 \
	-fno-rtti \
	-fno-exceptions \
	-DF_CPU=120000000L \
	-DARDUINO=10812 \
	-DARDUINO_GRAND_CENTRAL_M4 \
	-DARDUINO_ARCH_SAMD \
	-D__SAMD51P20A__ \
	-DADAFRUIT_GRAND_CENTRAL_M4 \
	-D__SAMD51__ \
	-DUSB_VID=0x239A \
	-DUSB_PID=0x8031 \
	-DUSBCON \
	-DUSB_CONFIG_POWER=100 \
	-DUSB_MANUFACTURER='"Adafruit LLC"' \
	-DUSB_PRODUCT='"Adafruit Grand Central M4"' \
	-D__FPU_PRESENT \
	-DARM_MATH_CM4 \
	-DENABLE_CACHE \
	-DVARIANT_QSPI_BAUD_DEFAULT=50000000 \
	-I${libdir}/cores/arduino \
	-I${libdir}/cores/arduino/TinyUSB \
	-I${libdir}/cores/arduino/TinyUSB/Adafruit_TinyUSB_ArduinoCore \
	-I${libdir}/cores/arduino/TinyUSB/Adafruit_TinyUSB_ArduinoCore/tinyusb/src \
	-I${libdir}/variants/grand_central_m4 \
	-I${libdir}/libraries/ \
	-I${libdir}/CMSIS/4.5.0/CMSIS/Include/ \
  -I${libdir}/CMSIS-Atmel/1.2.0/CMSIS/Device/ATMEL/  \
	${extras_inc} \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 

.SECONDARY:
%.cpp.o: %.cpp
	${cxx} -c ${ccflags} -std=gnu++11 $< -o $@

%.c.o: %.c
	${cc} -c ${ccflags} -std=gnu11 -fpermissive $< -o $@
	
%.elf: ${libos}	%.cpp.o
	${cxx} ${ldflags} $^ -o $@

%.bin: %.elf
	${objcopy} -O binary $< $@

%.upload: %.bin
	bossac --port=ttyACM0 -U -i --offset=0x4000 -w -v $< -R

.PHONY:
clean: 
	find . \( -name *.elf -o -name *.o -o -name *.bin \) -exec rm {} \;
