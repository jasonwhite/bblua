.PHONY: all clean test

SOURCES=$(wildcard src/*.cc)
OBJECTS=$(addsuffix .o, $(SOURCES))

LUA_SCRIPTS=$(shell find scripts -type f -name '*.lua')
LUA_SCRIPTS_C=$(patsubst scripts/%.lua, src/embedded/%.c, $(LUA_SCRIPTS))

TARGET=button-lua

# Path to Lua's installation directory. This is mostly used for the Travis CI
# build. If this path doesn't exist and if Lua is installed on the system, that
# path should be used automatically instead.
LUA_INSTALL_DIR=install/lua

CXXFLAGS=-std=c++11 -g -Wall -Werror -D__STDC_LIMIT_MACROS -I$(LUA_INSTALL_DIR)/include

all: $(TARGET) luaminify

%.cc.o: %.cc
	${CXX} $(CXXFLAGS) -c $< -o $@

# Generate strings from Lua files.
src/embedded/%.c: scripts/%.lua
	@mkdir -p "$(@D)"
	xxd -i $^ $@

src/embedded.cc.o: $(LUA_SCRIPTS_C)

$(TARGET): $(OBJECTS)
	${CXX} $(OBJECTS) -L$(LUA_INSTALL_DIR)/lib -llua -ldl -o $@

luaminify: tools/luaminify.cc.o
	${CXX} $^ -o $@

test: $(TARGET)
	@./test

clean:
	$(RM) $(TARGET) luaminify $(OBJECTS) $(LUA_SCRIPTS_C)
	${MAKE} -C src/lua clean
