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

INCFLAGS  = -iquotesrc
INCFLAGS += -I$(PATH_CJAM)
INCFLAGS += -I$(PATH_CGLM)/include
INCFLAGS += -I$(PATH_SOKOL)

EMCCFLAGS += -std=gnu2x
EMCCFLAGS += -g3
EMCCFLAGS += -Wall
EMCCFLAGS += -Wextra
EMCCFLAGS += -Wpedantic
EMCCFLAGS += -Wno-unused-parameter
EMCCFLAGS += -Wno-gnu
#EMCCFLAGS += -fsanitize=undefined,address,leak

# emcc specifics
EMCCLFAGS += -s INITIAL_MEMORY=256MB
EMCCFLAGS += -sINITIAL_MEMORY=256MB
EMCCLFAGS += -s TOTAL_MEMORY=256MB
# EMCCFLAGS += -s ALLOW_MEMORY_GROWTH
EMCCFLAGS += -s USE_SDL_IMAGE=2
EMCCFLAGS += -s SDL2_IMAGE_FORMATS='["png"]'
EMCCFLAGS += --preload-file res
EMCCFLAGS += -s USE_SDL=2
EMCCFLAGS += -s FULL_ES3
# EMCCFLAGS += --memoryprofiler

CCFLAGS += -std=c2x
CCFLAGS += -O2
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
LDFLAGS += -lstdc++

ifeq ($(UNAME), Darwin)
	LDFLAGS += -framework AudioUnit
	LDFLAGS += -framework AudioToolbox
endif

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

SOLOUD_SRC_GLOB = \
	lib/soloud/src/core/*.cpp \
	lib/soloud/src/filter/*.cpp \
	lib/soloud/src/c_api/*.cpp \
	lib/soloud/src/backend/sdl2_static/*.cpp \
	lib/soloud/src/audiosource/**/*.c \
	lib/soloud/src/audiosource/**/*.cpp
SOLOUD_SRC = $(shell find $(SOLOUD_SRC_GLOB))
SOLOUD_OBJ = $(addsuffix .o,$(SOLOUD_SRC))

$(SOLOUD_OBJ): %.o: %
	$(CC) -c -o $@ \
		-D WITH_SDL2_STATIC \
		-iquotelib/soloud/include \
		$(shell sdl2-config --cflags) \
		$<

soloud: $(SOLOUD_OBJ)
	$(EMCC) -r -o bin/soloud.o \
		-s USE_SDL=2\
		-D WITH_SDL2_STATIC \
		-s FULL_ES2=1 \
		-iquotelib/soloud/include \
		lib/soloud/src/core/*.cpp \
		lib/soloud/src/filter/*.cpp \
		lib/soloud/src/c_api/*.cpp \
		lib/soloud/src/backend/sdl2_static/*.cpp \
		lib/soloud/src/audiosource/wav/*.c \
		lib/soloud/src/audiosource/wav/*.cpp
	emar rcs bin/soloud.a bin/soloud.o
	ar rcs bin/libsoloud_native.a $(SOLOUD_OBJ)

$(OBJ): %.o: %.c
	$(CC) -o $@ -MMD -c $(CCFLAGS) $(INCFLAGS) $<

native: dirs shaders $(OBJ)
	$(LD) -o $(EXE) $(LDFLAGS) bin/libsoloud_native.a $(filter %.o,$^)

build: dirs shaders
	$(EMCC) -o $(OUT) -MMD $(EMCCFLAGS) $(INCFLAGS) $(EMLDFLAGS) bin/soloud.a $(SRC)

package: build
	cd $(BIN) && zip index.zip index.data index.html index.js index.wasm

run: build

clean:
	find src/shader -name "*.glsl.h" -type f -delete
	find src -name "*.o" -type f -delete
	find src -name "*.d" -type f -delete
	rm -rf bin

FORCE: ;
