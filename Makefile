ifeq ($(BUILD),release)
	CFLAGS += -O3 -s -DNDEBUG
else
	CFLAGS += -O0 -g -Werror 
endif

TARGET    := vine

SRCS      := $(shell find src -name *.c -or -name *.S)
OBJS      := $(SRCS:%=build/%.o)
DEPS      := $(OBJS:%.o=%.d)

INCS      := $(addprefix -I,$(shell find ./include -type d))
INCS      := -I./include

WARNINGS  += -pedantic -pedantic-errors -Wno-overlength-strings
WARNINGS  += -fmax-errors=1 -Wall -Wextra -Wdouble-promotion -Wformat=2
WARNINGS  += -Wformat-signedness -Wvla -Wformat-truncation=2 -Wformat-overflow=2
WARNINGS  += -Wnull-dereference -Winit-self -Wuninitialized
WARNINGS  += -Wimplicit-fallthrough=4 -Wstack-protector -Wmissing-include-dirs
WARNINGS  += -Wshift-overflow=2 -Wswitch-default -Wswitch-enum
WARNINGS  += -Wunused-parameter -Wunused-const-variable=2 -Wstrict-overflow=5
WARNINGS  += -Wstringop-overflow=4 -Wstringop-truncation -Walloc-zero -Walloca
WARNINGS  += -Warray-bounds -Wattribute-alias=2 -Wlogical-op
WARNINGS  += -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wfloat-equal
WARNINGS  += -Wshadow -Wunsafe-loop-optimizations -Wbad-function-cast
WARNINGS  += -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
WARNINGS  += -Wsign-conversion -Wpacked -Wdangling-else -Wparentheses
WARNINGS  += -Wdate-time -Wjump-misses-init -Waggregate-return
WARNINGS  += -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
WARNINGS  += -Wmissing-declarations -Wnormalized=nfkc -Wredundant-decls
WARNINGS  += -Wnested-externs 

CFLAGS    += -D_GNU_SOURCE $(INCS) -MMD -MP -std=c89 $(WARNINGS) -fPIE
CFLAGS    += -ftrapv -fstack-protector

LDFLAGS   += -pie -fPIE
LDLIBS    += -lm

build/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

build/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	@$(RM) *.d

build/%.S.o: %.S
	mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

tags: $(SRCS)
	gcc -M $(INCS) $(SRCS) | sed -e 's/[\ ]/\n/g' | \
		sed -e '/^$$/d' -e '/\.o:[ \t]*$$/d' | \
		ctags -L - $(CTAGS_FLAGS)

.PHONY: clean syntastic
clean:
	rm -f build/$(TARGET) $(OBJS) $(DEPS)

syntastic:
	echo $(CFLAGS) | tr ' ' '\n' > .syntastic_c_config

release:
	-$(MAKE) "BUILD=release"

-include $(DEPS)
