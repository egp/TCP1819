# Makefile v1

CXX      ?= clang++
AR       ?= ar
RM       ?= rm -rf
MKDIR_P  ?= mkdir -p

TARGET   := TCP1819
BUILD    := build

SRC_DIRS := src linux
INCLUDES := -Ilinux -Isrc

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -pedantic -g -O0 $(INCLUDES)
ARFLAGS  ?= rcs

SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

LIB  := $(BUILD)/lib$(TARGET)_host.a

.PHONY: all lib clean print

all: lib

lib: $(LIB)

$(LIB): $(OBJS)
	@$(MKDIR_P) $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

print:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"
	@echo "LIB=$(LIB)"

-include $(DEPS)

# Makefile v1