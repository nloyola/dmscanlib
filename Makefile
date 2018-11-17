#***************************************************************************
#
#
#***************************************************************************

.EXPORT_ALL_VARIABLES:

PROJECT := dmscanlib

DEBUG := on

OSTYPE := $(shell uname -s | tr [:upper:] [:lower:])
MTYPE := $(shell uname -m)
LANG := en_US                # for gcc error messages
BUILD_DIR := obj
JAVA_HOME := /usr/lib/jvm/java-8-oracle

SRCS := \
	src/DmScanLib.cpp \
	src/jni/DmScanLibJni.cpp \
	src/decoder/DecodeOptions.cpp \
	src/decoder/Decoder.cpp \
	src/decoder/DmtxDecodeHelper.cpp \
	src/decoder/WellRectangle.cpp \
	src/decoder/WellDecoder.cpp \
	src/decoder/ThreadMgr.cpp \
	src/imgscanner/ImgScanner.cpp \
	src/imgscanner/ImgScannerSane.cpp \
	src/imgscanner/SaneOption.cpp \
	src/imgscanner/SaneUtil.cpp \
	src/utils/DmTimeLinux.cpp \
	src/Image.cpp

# Uncomment DMTX_SRCS to compile with the version of libdmtx in third_party/lbdmtx
#DMTX_SRCS := third_party/libdmtx/dmtx.c

TEST_SRCS := \
	src/test/TestWellRectangle.cpp \
	src/test/ImageInfo.cpp \
	src/test/Tests.cpp \
	src/test/TestDmScanLib.cpp \
	src/test/TestDmScanLibLinux.cpp \
	src/test/TestImgScanner.cpp \
	src/test/TestImgScannerLinux.cpp \
	src/test/TestCommon.cpp

ifdef DMTX_SRCS
	SRCS += $(DMTX_SRCS)
endif

ifeq ($(MAKECMDGOALS),test)
	SRCS += $(TEST_SRCS)
endif

FILES = $(notdir $(SRCS))

PATHS = $(sort $(dir $(SRCS) ) )
OBJS := $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(FILES))))

INCLUDE_PATH := $(foreach inc,$(PATHS),$(inc)) $(JAVA_HOME)/include $(JAVA_HOME)/include/linux

LIBS := -lglog -lOpenThreads -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lsane

ifndef DMTX_SRCS
	LIBS := -ldmtx $(LIBS)
endif

TEST_LIBS := -lgtest -lconfig++ -lpthread
LIB_PATH :=

CC := g++
CXX := $(CC)
CFLAGS := -fmessage-length=0 -fPIC
SED := /bin/sed

ifneq ($(MAKECMDGOALS),test)
	CFLAGS += -O3
endif

ifeq ($(OSTYPE),mingw32)
	HOST := windows
endif

ifeq ($(HOST),windows)
	LIBS +=
	LIB_PATH :=
endif

ifeq ($(OSTYPE),linux)
	LIBS +=
	SRC +=
	CFLAGS +=

	ifeq ($(MTYPE),x86_64)
		LIB_PATH +=
	else
		LIB_PATH +=
		LIBS +=
	endif
endif

VPATH := $(CURDIR) $(INCLUDE_PATH)

CFLAGS += -c $(foreach inc,$(INCLUDE_PATH),-I$(inc))
CXXFLAGS := -pedantic -Wall -Wno-long-long -Wno-variadic-macros -Wno-deprecated \
	$(CFLAGS) -std=c++11 -fpermissive -std=gnu++0x
LDFLAGS += $(foreach path,$(LIB_PATH),-L$(path))
CFLAGS += -Wno-write-strings

ifeq ($(HOST),windows)
	CFLAGS += -D'srand48(n)=srand ((n))' -D'drand48()=((double)rand()/RAND_MAX)'
endif

ifdef DEBUG
	CFLAGS += -DDEBUG -g
	CXXFLAGS += -DDEBUG -g
#	CXXFLAGS += -D_GLIBCXX_DEBUG -DDEBUG -g
else
	CFLAGS += -O2
	CXXFLAGS += -O2
endif

ifndef VERBOSE
  SILENT := @
endif

.PHONY: all everything clean doc check-syntax

all: $(PROJECT)

$(PROJECT) : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -shared

test : $(OBJS)
	@echo "linking $@"
	$(SILENT) $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(TEST_LIBS)

clean:
	rm -rf  $(BUILD_DIR)/*.[odP] $(PROJECT) *.pnm

doc: doxygen.cfg
	doxygen $<

help:
	@echo 'Available targets:'
	@echo '  all:           Builds the library'
	@echo '  test:          Builds the test executable'
	@echo '  clean:         Clean compiled files'
	@echo '  doc:           Generates documentation from comments'
	@echo ''

#
# This rule also creates the dependency files
#
$(BUILD_DIR)/%.o : %.cpp
	@echo "compiling $<..."
	$(SILENT)$(CXX) $(CXXFLAGS) -MD -o $@ $<
	$(SILENT)cp $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.P; \
	$(SED) -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(BUILD_DIR)/$*.d >> $(BUILD_DIR)/$*.P; \
	rm -f $(BUILD_DIR)/$*.d
#
$(BUILD_DIR)/%.o : %.c
	@echo "compiling $<..."
	$(SILENT) gcc $(CFLAGS) -MD -o $@ $<
	$(SILENT)cp $(BUILD_DIR)/$*.d $(BUILD_DIR)/$*.P; \
	$(SED) -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(BUILD_DIR)/$*.d >> $(BUILD_DIR)/$*.P; \
	rm -f $(BUILD_DIR)/$*.d

-include $(OBJS:.o=.P)
