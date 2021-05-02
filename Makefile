ifeq ($(BUILD),release)
	CFLAGS += -O3 -s -DNDEBUG -fno-delete-null-pointer-checks
	LDFLAGS += -flto
else ifeq ($(BUILD),valgrind)
	CFLAGS += -Og -g -Werror -DUSE_VALGRIND
else ifeq ($(BUILD),sanitise)
	CFLAGS += -Og -g -Werror -fsanitize=address -fsanitize=undefined
	LDFLAGS += -lasan -lubsan
else
	BUILD = debug
	CFLAGS += -Og -g -Werror
endif

TARGET    := vine

#PC_DEPS   :=
#CFLAGS    += $(shell pkg-config --cflags $(PC_DEPS))
#LDLIBS    += $(shell pkg-config --static --libs $(PC_DEPS))

SRCS      := $(shell find src -name *.c -or -name *.S)
OBJS      := $(SRCS:%=build/$(BUILD)/%.o)
DEPS      := $(OBJS:%.o=%.d)
VHDRS     := $(shell find include -name *.v.h)
HDRS      := $(VHDRS:%.v.h=build/$(BUILD)/%.h)

INCS      := -I./build/$(BUILD)/include

WARNINGS  += -pedantic -pedantic-errors -Wno-overlength-strings
WARNINGS  += -fmax-errors=5 -Wall -Wextra -Wdouble-promotion -Wformat=2
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
WARNINGS  += -Wdate-time -Wjump-misses-init
WARNINGS  += -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
WARNINGS  += -Wmissing-declarations -Wnormalized=nfkc -Wredundant-decls
WARNINGS  += -Wnested-externs -fanalyzer

CFLAGS    += -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 $(INCS) -std=c89
CFLAGS    += -fPIE -ftrapv -fstack-protector $(WARNINGS)

LDFLAGS   += -pie -fPIE
LDLIBS    += -lm

VALGRIND_FLAGS += -s --show-leak-kinds=all --leak-check=full

.PHONY: $(TARGET)
$(TARGET): build/$(BUILD)/$(TARGET)
	@echo '  SYMLINK ' $(TARGET) "->" build/$(BUILD)/$(TARGET)
	@ln -sf build/$(BUILD)/$(TARGET) $(TARGET)

build/$(BUILD)/$(TARGET): $(OBJS)
	@echo '  LD      ' $@
	@$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

build/$(BUILD)/%.c.d: %.c
	@mkdir -p $(dir $@)
	@# This isn't typically very interesting
	@#echo '  CC [D]  ' $<.d
	@$(CC) -c $(CFLAGS) $< -MM -MG -MF - | tee $@.tmp | \
		sed -E -e 's#build/$(BUILD)/include/##g' | \
		sed -E -e 's#\b(\w|\.)*\.h\b#build/$(BUILD)/include/\0#g' | \
		sed -E -e 's#\b((\w|\.)*)\.o\b#build/$(BUILD)/src/\1.c.o#' \
		> $@

build/$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo '  CC      ' $<.o
	@$(CC) -c $(CFLAGS) $< -o $@

build/$(BUILD)/%.S.o: %.S
	@mkdir -p $(dir $@)
	@echo '  AS      ' $<.o
	@$(AS) $(ASFLAGS) $< -o $@

build/$(BUILD)/%.h: %.v.h
	@mkdir -p $(dir $@)
	@echo '  AWK     ' ${<:%.v.h=%.h}
	@awk -S -f vineinclude.awk $< > $@

tags: $(SRCS)
	gcc -M $(INCS) $(SRCS) | sed -e 's/[\ ]/\n/g' | \
		sed -e '/^$$/d' -e '/\.o:[ \t]*$$/d' | \
		ctags -L - $(CTAGS_FLAGS)


.PHONY: clean cleanall syntastic debug release valgrind sanitise
clean:
	$(RM) build/$(TARGET) $(OBJS) $(DEPS) $(HDRS)

cleanall: clean
	$(RM) -r build/*/*

syntastic:
	echo $(CFLAGS) | tr ' ' '\n' | grep -v 'max-errors' > .syntastic_c_config

release:
	-$(MAKE) "BUILD=release"

valgrind:
	-$(MAKE) "BUILD=valgrind"
	valgrind $(VALGRIND_FLAGS) ./build/valgrind/$(TARGET)

sanitise:
	-$(MAKE) "BUILD=sanitise"
	./build/sanitise/$(TARGET)

debug:
	-$(MAKE)
	./build/debug/$(TARGET)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
-include $(DEPS)
endif
endif
