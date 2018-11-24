#############################################################################
# SRC_FILES - a list of all source files
# Output name under one of the following:
#     EXE_NAME (executable), 
#     LIB_NAME (dynamic library) or 
#     SLIB_NAME (static library) or
# BIN_DIR - Bin directory (output dir)
# INC_DIRS - a list of additional include directories
# LIB_DIRS - a list of additional library directories
# USED_LIBS - a list of libraries to link with (Example: SimpleAmqpClient dropbox usb-1.0 udev blkid)
# DEFINES - [Optional] additional preprocessor defines
# CFLAGS - [Optional] additional flags for the compiler
# LDFLAGS - [Optional] additional flags for the linker
# TARGET_SYS_ROOT - [Optional] The path to the root of the target
#############################################################################
# TODO:
# - Add support for MIPS
#############################################################################

# Cross compilation (Arm, x86, x64)
#PLATFORM = 
# Release or Debug (with gdb support)
CFG = Release
include Common/CommonDefs.mak
BIN_DIR = .
INC_DIRS = Include #/usr/local/include/opencv
SRC_FILES = Source/*.cpp
EXE_NAME = vcode
USED_LIBS = pthread boost_system opencv_core opencv_imgproc opencv_features2d opencv_highgui opencv_videoio
LDFLAGS = -pthread -L /usr/local/lib # Required for multiple installations of OpenCV

# define a function to figure .o file for each source file (placed under intermediate directory)
SRC_TO_OBJ = $(addprefix ./$(INT_DIR)/,$(addsuffix .o,$(notdir $(basename $1))))

# create a list of all object files
OBJ_FILES = $(call SRC_TO_OBJ,$(SRC_FILES_LIST))

# define a function to translate any source file to its dependency file (note that the way we create
# dep files, as a side affect of compilation, always puts the files in the INT_DIR with suffix .d)
SRC_TO_DEP = $(addprefix ./$(INT_DIR)/,$(addsuffix .d,$(notdir $(basename $1))))

# create a list of all dependency files
DEP_FILES = $(call SRC_TO_DEP,$(SRC_FILES_LIST))

# older version of gcc doesn't support the '=' symbol in include dirs, so we replace it ourselves with sysroot
INC_DIRS_FROM_SYSROOT = $(patsubst =/%,$(TARGET_SYS_ROOT)/%,$(INC_DIRS))

# append the -I switch to each include directory
INC_DIRS_OPTION = $(foreach dir,$(INC_DIRS_FROM_SYSROOT),-I$(dir))

# append the -L switch to each library directory
LIB_DIRS_OPTION = $(foreach dir,$(LIB_DIRS),-L$(dir)) -L$(OUT_DIR)

# append the -l switch to each library used
USED_LIBS_OPTION = $(foreach lib,$(USED_LIBS),-l$(lib))

# append the -D switch to each define
DEFINES_OPTION = $(foreach def,$(DEFINES),-D$(def))

# tell compiler to use the target system root
ifdef TARGET_SYS_ROOT
	CFLAGS += --sysroot=$(TARGET_SYS_ROOT)
	LDFLAGS += --sysroot=$(TARGET_SYS_ROOT)
endif

# set Debug / Release flags
ifeq "$(CFG)" "Debug"
	CFLAGS += -O0 -g -std=c++11 -static -Wall
endif
ifeq "$(CFG)" "Release"
	CFLAGS += -O3 -DNDEBUG -std=c++11 -static -Wall
endif

CFLAGS += $(INC_DIRS_OPTION) $(DEFINES_OPTION)
LDFLAGS += $(LIB_DIRS_OPTION) $(USED_LIBS_OPTION)

# some lib / exe specifics
ifneq "$(LIB_NAME)" ""
	OUTPUT_NAME = lib$(LIB_NAME).so
	CFLAGS += -fPIC -fvisibility=hidden
	ifneq ("$(OSTYPE)","Darwin")
		LDFLAGS += -Wl,--no-undefined
		OUTPUT_NAME = lib$(LIB_NAME).so
		OUTPUT_COMMAND = $(CXX) -o $(OUTPUT_FILE) $(OBJ_FILES) $(LDFLAGS) -shared
	else
		LDFLAGS += -undefined error
		OUTPUT_NAME = lib$(LIB_NAME).dylib
		OUTPUT_COMMAND = $(CXX) -o $(OUTPUT_FILE) $(OBJ_FILES) $(LDFLAGS) -dynamiclib -headerpad_max_install_names -install_name $(OUTPUT_NAME)
	endif
endif
ifneq "$(EXE_NAME)" ""
	OUTPUT_NAME = $(EXE_NAME)
	OUTPUT_COMMAND = $(CXX) -o $(OUTPUT_FILE) $(OBJ_FILES) $(LDFLAGS)
endif
ifneq "$(SLIB_NAME)" ""
	CFLAGS += -fPIC
	OUTPUT_NAME = lib$(SLIB_NAME).a

	ifneq ("$(OSTYPE)","Darwin")
		OUTPUT_COMMAND = $(AR) -cq $(OUTPUT_FILE) $(OBJ_FILES)
	else
		OUTPUT_COMMAND = libtool -static -o $(OUTPUT_FILE) $(OBJ_FILES)
	endif
endif

# create a target for the object file (the CXX command creates both an .o file
# and a .d file)
define CREATE_SRC_TARGETS
ifneq ("$(OSTYPE)","Darwin")
$(call SRC_TO_OBJ,$1) : $1 | $(INT_DIR)
	$(CXX) -MD -MP -MT "$(call SRC_TO_DEP,$1) $$@" -c $(CFLAGS) -o $$@ $$<
else
$(call SRC_TO_OBJ,$1) : $1 | $(INT_DIR)
	$(CXX) -c $(CFLAGS) -o $$@ $$<
endif
endef

# Remove output directory when exists
define CLEAN_ALL
ifneq ("$(OUT_DIR)",".")
clean-extra:
	rm -rf $(OUT_DIR) $(INT_DIR)
	ls -p |grep -v / |grep -v -i makefile |xargs rm -rf
else
clean-extra:
	rm -rf $(INT_DIR)
	ls -p |grep -v / |grep -v -i makefile |xargs rm -rf
endif
endef

#############################################################################
# Targets
#############################################################################
.PHONY: all clean-objs clean-defs clean-$(OUTPUT_FILE) clean-all clean-extra

# define the target 'all' (it is first, and so, default)
all: $(OUTPUT_FILE)

# create targets for each source file
$(foreach src,$(SRC_FILES_LIST),$(eval $(call CREATE_SRC_TARGETS,$(src))))

# include all dependency files (we don't need them the first time, so we can use -include)
-include $(DEP_FILES)

# Intermediate directory
$(INT_DIR):
	mkdir -p $(INT_DIR)

# Output directory
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Final output file
$(OUTPUT_FILE): $(OBJ_FILES) $(SRC_FILES_LIST) | $(OUT_DIR)
	$(OUTPUT_COMMAND)

clean-$(OUTPUT_FILE):
	rm -rf $(OUTPUT_FILE)

clean-objs:
	rm -rf $(OBJ_FILES)
	
clean-defs:
	rm -rf $(DEP_FILES)

clean: clean-objs clean-defs clean-$(OUTPUT_FILE)

$(eval $(call CLEAN_ALL))

clean-all: clean clean-extra

