#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

export DT := $(shell date +"%Y/%m/%d")

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET		    :=	$(notdir $(CURDIR))
BUILD		    :=	build
SOURCES		    :=	soos
DATA		    :=	data
INCLUDES	    :=	inc
APP_TITLE       :=  TWPatch
APP_DESCRIPTION :=  FIRM Patcher
APP_AUTHOR      :=  Sono
APP_PRODUCT_CODE:=  CTR-P-ALSA
APP_UNIQUE_ID   :=  0x1FF0C
ICON            :=  assets/logo.png

APP_TITLE       :=  $(shell echo "$(APP_TITLE)" | cut -c1-128)
APP_DESCRIPTION :=  $(shell echo "$(APP_DESCRIPTION)" | cut -c1-256)
APP_AUTHOR      :=  $(shell echo "$(APP_AUTHOR)" | cut -c1-128)
APP_PRODUCT_CODE:=  $(shell echo $(APP_PRODUCT_CODE) | cut -c1-16)
APP_UNIQUE_ID   :=  $(shell echo $(APP_UNIQUE_ID) | cut -c1-7)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard

CFLAGS	:=	-g -Wall -Ofast -mword-relocations \
			-fomit-frame-pointer -ffast-math \
			-ffunction-sections -fdata-sections \
			$(ARCH)\
			-Wno-format -Wno-strict-aliasing \
			-Wno-discarded-qualifiers

CFLAGS	+=	$(INCLUDE) -DARM11 -D_3DS -DDATETIME=\"$(DT)\"

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections

LIBS	:= -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=  $(shell find $(SOURCES) -name '*.c' -printf "%P\n")	#$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=  $(shell find $(SOURCES) -name '*.cpp' -printf "%P\n")	#$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=  $(shell find $(SOURCES) -name '*.s' -printf "%P\n")	#$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin))) \
				$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.raw))) \
				$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.S)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I-$(CURDIR)/$(SOURCES)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

export _3DSXFLAGS += --smdh=$(APP_ICON)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@find $(SOURCES) -type d -printf "%P\0" | xargs -0 -I {} mkdir -p $(BUILD)/{}
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -rf $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(TARGET).3ds $(TARGET).cia


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
.PHONY: all
all	:	$(OUTPUT).3dsx $(OUTPUT).cia

$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).elf	:	$(OFILES)

$(TOPDIR)/assets/banner.bin: $(TOPDIR)/assets/banner.png $(TOPDIR)/assets/banner.wav
	@bannertool makebanner -i $(TOPDIR)/assets/banner.png -a $(TOPDIR)/assets/banner.wav -o $(TOPDIR)/assets/banner.bin

$(TOPDIR)/assets/image.bin: $(TOPDIR)/assets/logo.png
	@bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $(TOPDIR)/assets/logo.png -o $(TOPDIR)/assets/image.bin


stripped.elf: $(OUTPUT).elf
	@cp $(OUTPUT).elf stripped.elf
	@$(PREFIX)strip stripped.elf
	@echo "built ... $(notdir $@)"

$(OUTPUT).cia: stripped.elf $(TOPDIR)/assets/banner.bin $(TOPDIR)/assets/image.bin
	@makerom -f cia -o $(OUTPUT).cia -rsf $(TOPDIR)/assets/cia.rsf -target t -exefslogo -elf stripped.elf -icon $(TOPDIR)/assets/image.bin -banner $(TOPDIR)/assets/banner.bin -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)"
	@echo "built ... $(notdir $@)"

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.raw.o	:	%.raw
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(TOPDIR)/lzenc $< $(@).lz
	@$(TOPDIR)/lzenc $(@).lz $(@).lz
	@bin2s $(@).lz | sed 's/_o_lz\([:_]\)\?/\1/' | $(AS) -o $(@)
	@echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(<F) | tr . _)`.h
	@echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(<F) | tr . _)`.h
	@echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(<F) | tr . _)`.h

%.S.o	:	%.S
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@armips -strequ outfile $(patsubst %.S.o,%.bin,$@) -root $(CURDIR) $<
	@bin2s $(patsubst %.S.o,%.bin,$@) | $(AS) -o $(@)
	@echo "extern const u8" `(echo $(patsubst %.S,%.bin,$(<F)) | \
		sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(patsubst %.S,%.bin,$(<F)) | tr . _)`.h
	@echo "extern const u8" `(echo $(patsubst %.S,%.bin,$(<F)) | \
		sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(patsubst %.S,%.bin,$(<F)) | tr . _)`.h
	@echo "extern const u32" `(echo $(patsubst %.S,%.bin,$(<F)) | \
		sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(patsubst %.S,%.bin,$(<F)) | tr . _)`.h
	

# WARNING: This is not the right way to do this! TODO: Do it right!
#---------------------------------------------------------------------------------
%.vsh.o	:	%.vsh
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@python $(AEMSTRO)/aemstro_as.py $< ../$(notdir $<).shbin
	@bin2s ../$(notdir $<).shbin | $(PREFIX)as -o $@
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@echo "extern const u32" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(notdir $<).shbin | tr . _)`.h
	@rm ../$(notdir $<).shbin

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
