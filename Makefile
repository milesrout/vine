ifeq ($(BUILD),release)
	CFLAGS += -O3 -s -DNDEBUG
else
	CFLAGS += -O0 -g
endif

TARGET    := vine

SRCS      := $(shell find src -name *.c)
OBJS      := $(SRCS:%=build/%.o)
DEPS      := $(OBJS:%.o=%.d)

INCS      := $(addprefix -I,$(shell find ./include -type d))

#CFLAGS    += -Qunused-arguments -Wno-unknown-warning-option
CFLAGS    += -D_POSIX_C_SOURCE $(INCS) -MMD -MP -pedantic -pedantic-errors
CFLAGS    += -std=c89 -ftrapv -fstack-protector -Werror -Wfatal-errors
CFLAGS    += -fmax-errors=1 -Wall -Wextra -Wdouble-promotion -Wformat=2
CFLAGS    += -Wformat-signedness -Wvla -Wformat-truncation=2 -Wformat-overflow=2
CFLAGS    += -Wnull-dereference -Winit-self -Wuninitialized
CFLAGS    += -Wimplicit-fallthrough=4 -Wstack-protector -Wmissing-include-dirs
CFLAGS    += -Wshift-overflow=2 -Wswitch-default -Wswitch-enum
CFLAGS    += -Wunused-parameter -Wunused-const-variable=2 -Wstrict-overflow=5
CFLAGS    += -Wstringop-overflow=4 -Wstringop-truncation -Walloc-zero -Walloca
CFLAGS    += -Warray-bounds -Wattribute-alias=2 -Wlogical-op
CFLAGS    += -Wduplicated-branches -Wduplicated-cond -Wtrampolines -Wfloat-equal
CFLAGS    += -Wshadow -Wunsafe-loop-optimizations -Wbad-function-cast
CFLAGS    += -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion
CFLAGS    += -Wsign-conversion -Wpacked -Wdangling-else -Wparentheses
CFLAGS    += -Wdate-time -Wjump-misses-init -Waggregate-return
CFLAGS    += -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
CFLAGS    += -Wmissing-declarations -Wnormalized=nfkc -Wredundant-decls
CFLAGS    += -Wnested-externs
LDLIBS    += -lm

build/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

build/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	@$(RM) *.d

tags:
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
