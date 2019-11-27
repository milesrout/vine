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

CFLAGS    += $(INCS) -MMD -MP -pedantic -pedantic-errors -std=c89
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
