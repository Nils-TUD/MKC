SRC_DIR	= ../src
SRC	= $(notdir $(wildcard $(SRC_DIR)/*cc))
TARGET	= $(SRC:.cc=.nova)
VPATH	= $(SRC_DIR)

STRIP	= strip

CCFLAGS+= -m64 -g

CXXFLAGS+= \
	-m64\
	-MD \
	-Os \
	-ffunction-sections \
	-fstrict-aliasing \
	-fno-rtti \
	-fno-exceptions \
	-fcheck-new \
	-fshort-enums \
	--param max-inline-insns-single=100 \
	-fomit-frame-pointer \
	-fno-stack-protector \
	-fno-PIC \
	-minline-all-stringops \
	-ggdb \
	-nostdinc \

CXXFLAGS+= -Wall -Wextra

.PRECIOUS : %.o %.nova.debug

all : $(TARGET)

%.nova : %.nova.debug
	cp $^ $@
	$(STRIP) $@

%.nova.debug : linker.ld start.o %.o
	$(LD) -gc-sections -N -Map=$@.map -o $@ -T $^

%.o : %.S
	$(CC) $(CCFLAGS) -c -o $@ $<

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

cl clean cla cleanall :
	rm -f *.o *.d *.nova *.nova.debug *.map
