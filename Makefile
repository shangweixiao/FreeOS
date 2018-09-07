ENTRYPOINT = 0x0
ASM=as
CC=gcc
LD=ld

DIRNAME =
ASMFLAGS= -I include
CFLAGS = -I include -Wall -c -fno-pie
LDFLAGS = -Ttext $(ENTRYPOINT) -s --oformat binary

BOSBOOT = $(BOSBOOTSECTOR) $(BOSLOADER)

BOSBOOTSECTOR = boot/boot.bin
BOSBSOBJ = boot/boot.o

BOSLOADER = boot/boot32.bin
BOSLDOBJS = boot/boot32.o

BOSKERNEL = kernel.bin
HEADER = kernel/code32.o
ASMLIB = lib/klibs.o 
OBJS = kernel/main.o lib/klibc.o kernel/8259A.o kernel/init.o \
       kernel/keyboard.o kernel/floppy.o kernel/process.o kernel/clock.o
DEPS = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all clean erverything image buildimg

all: erverything
clean: 
	rm -f BOS.img $(DEPS) $(HEADER) $(ASMLIB) $(OBJS) $(BOSBOOT) $(BOSKERNEL) $(BOSBSOBJ) $(BOSLDOBJS)
erverything: $(BOSBOOT) $(BOSKERNEL)
image: all buildimg

buildimg:
	@umount /dev/sdb; \
	/sbin/mkfs.vfat -F 32 -I /dev/sdb; \
	./wt boot/boot.bin /dev/sdb 0; \
	./wt boot/boot.bin /dev/sdb 0; \
	sync; \
	sync; \
	mount -t vfat /dev/sdb /mnt/remote; \
	cp -f boot/boot32.bin /mnt/remote/loader.bin; \
	cp -f kernel.bin /mnt/remote/kernel.bin; \
	umount /mnt/remote

$(BOSBOOTSECTOR) : $(BOSBSOBJ)
	$(LD) $(LDFLAGS) -o $@ $<
$(BOSBSOBJ) : boot/boot.s
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BOSLOADER) : $(BOSLDOBJS)
	$(LD) $(LDFLAGS) -o $@ $<
$(BOSLDOBJS) : boot/boot32.s
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BOSKERNEL) : $(HEADER) $(ASMLIB) $(OBJS)
	$(LD) $(LDFLAGS) -o $(BOSKERNEL) $(HEADER) $(ASMLIB) $(OBJS)
$(HEADER) : kernel/code32.s
	$(ASM) $(ASMFLAGS) -o $@ $<
$(ASMLIB) : lib/klibs.s
	$(ASM) $(ASMFLAGS) -o $@ $<

%.d : %.c
	@$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[: ]*\(.*\)\.c,\1.o $@:\ \1.c\ ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

-include $(DEPS)

