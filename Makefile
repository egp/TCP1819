# Makefile v4

CXX      ?= clang++
AR       ?= ar
RM       := rm -rf
MKDIR_P  ?= mkdir -p

TARGET   := TCP1819
BUILD    := build

SRC_DIRS := src linux
INCLUDES := -Ilinux -Isrc
HOST_TEST_INCLUDES := $(INCLUDES) -Iextras/host_test

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -pedantic -g -O0
ARFLAGS  ?= rcs

LIB_SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
LIB_OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(LIB_SRCS))
LIB_DEPS := $(LIB_OBJS:.o=.d)

HOST_TEST_SUPPORT_DIRS := extras/host_test
HOST_TEST_DIRS := tests/host
HOST_TEST_SUPPORT_SRCS := $(foreach d,$(HOST_TEST_SUPPORT_DIRS),$(wildcard $(d)/*.cpp))
HOST_TEST_SRCS := $(foreach d,$(HOST_TEST_DIRS),$(wildcard $(d)/*.cpp))
HOST_TEST_OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(HOST_TEST_SUPPORT_SRCS) $(HOST_TEST_SRCS))
HOST_TEST_DEPS := $(HOST_TEST_OBJS:.o=.d)
HOST_TEST_BIN := $(BUILD)/tests/host/test_WB_tcp1819_scripted_bus

LIB := $(BUILD)/lib$(TARGET)_host.a

.PHONY: all lib test clean print

all: lib

lib: $(LIB)

test: $(HOST_TEST_BIN)
	$(HOST_TEST_BIN)

$(LIB): $(LIB_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(HOST_TEST_BIN): $(HOST_TEST_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(HOST_TEST_INCLUDES) $^ -o $@

$(BUILD)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(HOST_TEST_INCLUDES) -MMD -MP -c $< -o $@

clean:
	$(RM) $(BUILD)

print:
	@echo "LIB_SRCS=$(LIB_SRCS)"
	@echo "LIB_OBJS=$(LIB_OBJS)"
	@echo "LIB=$(LIB)"
	@echo "HOST_TEST_SUPPORT_SRCS=$(HOST_TEST_SUPPORT_SRCS)"
	@echo "HOST_TEST_SRCS=$(HOST_TEST_SRCS)"
	@echo "HOST_TEST_BIN=$(HOST_TEST_BIN)"

-include $(LIB_DEPS)
-include $(HOST_TEST_DEPS)

# Makefile v4