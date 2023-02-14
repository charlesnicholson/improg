SRCS := main.c

BUILD_DIR := build
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
OS := $(shell uname)
COMPILER_VERSION := $(shell $(CXX) --version)

CFLAGS = --std=c99
CXXFLAGS = --std=c++20

CPPFLAGS += -MMD -MP -Os -g

CPPFLAGS += -Wall -Werror -Wextra

ifneq '' '$(findstring clang,$(COMPILER_VERSION))'
CPPFLAGS += -Weverything \
			-Wno-poison-system-directories \
			-Wno-format-pedantic \
			-Wno-c++98-compat-bind-to-temporary-copy
CFLAGS += -Wno-declaration-after-statement
else
CPPFLAGS += -Wconversion
endif

CPPFLAGS += -Wno-c++98-compat \
			-Wno-padded \
			-Wno-format-nonliteral \
			-Wno-covered-switch-default

$(BUILD_DIR)/demo: $(OBJS) $(BUILD_DIR)/improg.c.o Makefile
	$(CXX) $(LDFLAGS) $(LDFLAGS_SAN) $(OBJS) $(BUILD_DIR)/improg.c.o -o $@

$(BUILD_DIR)/%.c.o: %.c Makefile
	mkdir -p $(dir $@) && $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cc.o: %.cc Makefile
	mkdir -p $(dir $@) && $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

.DEFAULT_GOAL := $(BUILD_DIR)/demo

-include $(DEPS)

