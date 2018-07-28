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
