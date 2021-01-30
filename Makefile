<<<<<<< HEAD
### 
## libmotion_driver
###

TARGET_LIB = libmotion_driver.a
OBJS   = motion_driver.o motion.o

INCDIR = 
CFLAGS = -G0 -O2 -Wall

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

USE_USER_LIBS = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

install:
	@cp -v libmotion_driver.a `psp-config --psp-prefix`/lib
	@cp -v motion.h `psp-config --psp-prefix`/include
	@echo "Done."
=======
TARGET = motion_driver
OBJS = main.o debug.o sio.o hook.o exports.o sceCodec_driver.o utils.o config.o

BUILD_PLUGIN = 1

NOLIBM = 1

# Use only kernel libraries
USE_KERNEL_LIBS = 1

INCDIR = 
CFLAGS = -Os -G0 -Wall -nostdlib -fno-builtin-printf -fno-builtin-log2 #-DRELEASE #-DDEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -lpspumd_driver -lpsphprm_driver -lpsppower_driver

ifeq ($(RELEASE),1)
CFLAGS += -DRELEASE
else
CFLAGS += -g
endif

ifeq ($(NOLIBM),1)
# Use the kernel's small inbuilt libc
USE_KERNEL_LIBC = 1
#USE_PSPSDK_LIBC = 1
CFLAGS += -DNOLIBM
OBJS += minimath.o
else
USE_PSPSDK_LIBC = 1
LIBS += -lm -lpspkernel
endif


ifdef BUILD_PLUGIN
CFLAGS += -DBUILD_PLUGIN
endif


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

all:
	cp motion_driver.prx SEPLUGINS/motion_driver.prx
	cp motion_driver.prx SDK/MOTIONSAMPLE/motion_driver.prx
>>>>>>> psp-motion-driver/master
