SRCS := main.c improg.c

BUILD_DIR := build
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
OS := $(shell uname)
COMPILER_VERSION := $(shell $(CXX) --version)

CFLAGS = --std=gnu11
CXXFLAGS = --std=c++20

CPPFLAGS += -MMD -MP -Os -flto -g

CPPFLAGS += -Wall -Werror -Wextra -Wimplicit-fallthrough

ifneq '' '$(findstring clang,$(COMPILER_VERSION))'
CPPFLAGS += -Weverything \
			-Wno-poison-system-directories \
			-Wno-format-pedantic
CXXFLAGS += -Wno-c++98-compat-bind-to-temporary-copy
CFLAGS += -Wno-declaration-after-statement
else
CPPFLAGS += -Wconversion
endif

CXXFLAGS = -Wno-c++98-compat
CPPFLAGS += -Wno-padded \
			-Wno-format-nonliteral \
			-Wno-covered-switch-default
LDFLAGS = -flto

$(VERBOSE).SILENT:

$(BUILD_DIR)/demo: $(OBJS) Makefile
	$(CXX) $(LDFLAGS) $(LDFLAGS_SAN) $(OBJS) -o $@ && strip $@

$(BUILD_DIR)/%.c.o: %.c Makefile
	mkdir -p $(dir $@) && $(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/%.cc.o: %.cc Makefile
	mkdir -p $(dir $@) && $(CXX) $(CPPFLAGS) $(CXXFLAGS) -Iinclude -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

.DEFAULT_GOAL := $(BUILD_DIR)/demo

-include $(DEPS)

