print-%:; @echo $($*)

UNAME = $(shell uname -s)

CCFLAGS =
LDFLAGS =
EMCCFLAGS =
EMLDFLAGS =

EMCC = emcc

ifeq ($(UNAME), Darwin)
	CC = $(shell brew --prefix llvm)/bin/clang
	LD = $(shell brew --prefix llvm)/bin/clang

	CCFLAGS += -I$(shell brew --prefix llvm)/include
	LDFLAGS += -L$(shell brew --prefix llvm)/lib
	LDFLAGS += -framework OpenGL
else
	CC = clang
	LD = clang
endif

ifeq ($(UNAME), Darwin)
	SHDC = lib/sokol-tools-bin/bin/osx_arm64/sokol-shdc
else
	# TODO
endif

# library paths
PATH_LIB   = lib
PATH_CJAM = $(PATH_LIB)/cjam
PATH_CGLM  = $(PATH_LIB)/cglm
PATH_SOKOL = $(PATH_LIB)/sokol
PATH_CUTE = $(PATH_LIB)/cute_headers

INCFLAGS  = -iquotesrc
INCFLAGS += -I$(PATH_CJAM)
INCFLAGS += -I$(PATH_CGLM)/include
INCFLAGS += -I$(PATH_SOKOL)
INCFLAGS += -I$(PATH_CUTE)

EMCCFLAGS += -std=gnu2x
EMCCFLAGS += -g3
EMCCFLAGS += -Wall
EMCCFLAGS += -Wextra
EMCCFLAGS += -Wpedantic
EMCCFLAGS += -Wno-unused-parameter
EMCCFLAGS += -Wno-gnu
EMCCFLAGS += -fsanitize=undefined,address,leak

# emcc specifics
EMCCLFAGS += -s TOTAL_MEMORY=33554432
EMCCFLAGS += -s ALLOW_MEMORY_GROWTH
EMCCFLAGS += -s USE_SDL_IMAGE=2
EMCCFLAGS += -s SDL2_IMAGE_FORMATS='["png"]'
EMCCFLAGS += --preload-file res
EMCCFLAGS += -s USE_SDL=2
EMCCFLAGS += -s FULL_ES3

CCFLAGS += -std=c2x
CCFLAGS += -O0
CCFLAGS += -g
CCFLAGS += -Wall
CCFLAGS += -Wextra
CCFLAGS += -Wpedantic
CCFLAGS += -Wno-unused-parameter
CCFLAGS += -Wno-gnu

CCFLAGS += -fsanitize=undefined,address,leak
LDFLAGS += -fsanitize=undefined,address,leak

CCFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)
LDFLAGS += -lSDL2_Image

LDFLAGS += -framework AudioUnit
LDFLAGS += -framework AudioToolbox

ifeq ($(TARGET),RELEASE)
EMCCFLAGS += --shell-file emscripten/shell-release.html
else
EMCCFLAGS += --shell-file emscripten/shell.html
endif

BIN 	= bin

SRC = $(shell find src -name "*.c")
DEP = $(SRC:%.c=%.d)
OBJ = $(SRC:%.c=%.o)
OUT	= bin/index.html
EXE = bin/game

SHADERS = src/shader

SHDSRC = $(shell find $(SHADERS) -name "*.glsl")
SHDOUT = $(SHDSRC:%.glsl=%.glsl.h)

$(SHDOUT): %.glsl.h: %.glsl
	$(SHDC) --input $^ --output $@ --slang glsl300es:glsl330 --format=sokol_impl

shaders: $(SHDOUT)

-include $(DEP)

all: dirs test

$(BIN):
	$(shell mkdir -p  $@)

dirs: $(BIN) FORCE
	$(shell mkdir -p $(BIN))
	$(shell mkdir -p $(BIN)/src)
	rsync -a --include '*/' --exclude '*' "src" "bin"

$(OBJ): %.o: %.c
	$(CC) -o $@ -MMD -c $(CCFLAGS) $(INCFLAGS) $<

native: dirs shaders $(OBJ)
	$(LD) -o $(EXE) $(LDFLAGS) $(filter %.o,$^)

soloud:
	$(EMCC) -o bin/soloud.a \
		-iquotelib/soloud/include \
		lib/soloud/src/core/*.cpp \
		lib/soloud/src/filter/*.cpp \
		lib/soloud/src/backend/sdl2_static/*.cpp \
		lib/soloud/src/audiosource/wav/*.c

build: dirs shaders soloud
	$(EMCC) -o $(OUT) -MMD $(EMCCFLAGS) $(INCFLAGS) $(EMLDFLAGS) lib/soloud $(SRC)

package: build
	cd $(BIN) && zip index.zip index.data index.html index.js index.wasm

run: build

clean:
	find src/shader -name "*.glsl.h" -type f -delete
	find src -name "*.o" -type f -delete
	find src -name "*.d" -type f -delete
	rm -rf bin

FORCE: ;
