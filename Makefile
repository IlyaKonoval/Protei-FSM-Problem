END_STATES ?= testdata/ds1/end_states.txt
INPUTS     ?= testdata/ds1/in.txt
OUT        ?= /tmp/out.csv

BUILD_RELEASE ?= build-release
BUILD_DEBUG   ?= build-debug

COMPOSE := docker-compose
SERVICE := app

APP_RELEASE := $(BUILD_RELEASE)/app
APP_DEBUG   := $(BUILD_DEBUG)/app

.DEFAULT_GOAL := native-release

.PHONY: native-release native-debug native-run native-clean \
        native-test1 native-test2 native-test3 native-test4 native-test5 native-test-all \
        build run run-debug \
        test1 test2 test3 test4 test5 test-all \
        clean

# ── Native ───────────────────────────────────────────────────────────────────

native-release:
	cmake -S . -B $(BUILD_RELEASE) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_RELEASE) --parallel

native-debug:
	cmake -S . -B $(BUILD_DEBUG) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DEBUG) --parallel

native-run: native-release
	$(APP_RELEASE) $(END_STATES) $(OUT) $(INPUTS)

native-run-debug: native-debug
	$(APP_DEBUG) $(END_STATES) $(OUT) $(INPUTS)

native-test1: native-release
	$(APP_RELEASE) testdata/ds1/end_states.txt /tmp/out_ds1.csv testdata/ds1/in.txt

native-test2: native-release
	$(APP_RELEASE) testdata/ds2/end_states.txt /tmp/out_ds2.csv testdata/ds2/in.txt

native-test3: native-release
	$(APP_RELEASE) testdata/ds3/end_states.txt /tmp/out_ds3.csv testdata/ds3/in.txt

native-test4: native-release
	$(APP_RELEASE) testdata/ds4/end_states.txt /tmp/out_ds4.csv testdata/ds4/in.txt

native-test5: native-release
	$(APP_RELEASE) testdata/ds5/end_states.txt /tmp/out_ds5.csv \
		testdata/ds5/in1.txt testdata/ds5/in2.txt testdata/ds5/in3.txt

native-test-all: native-test1 native-test2 native-test3 native-test4 native-test5

native-clean:
	rm -rf $(BUILD_RELEASE) $(BUILD_DEBUG)

# ── Docker ───────────────────────────────────────────────────────────────────

build:
	$(COMPOSE) build $(SERVICE)

run:
	$(COMPOSE) run --rm --build $(SERVICE) $(END_STATES) $(OUT) $(INPUTS)

run-debug:
	$(COMPOSE) run --rm --build --entrypoint /app/build-debug/app $(SERVICE) $(END_STATES) $(OUT) $(INPUTS)

test1:
	$(COMPOSE) run --rm --build $(SERVICE) testdata/ds1/end_states.txt /tmp/out_ds1.csv testdata/ds1/in.txt

test2:
	$(COMPOSE) run --rm --build $(SERVICE) testdata/ds2/end_states.txt /tmp/out_ds2.csv testdata/ds2/in.txt

test3:
	$(COMPOSE) run --rm --build $(SERVICE) testdata/ds3/end_states.txt /tmp/out_ds3.csv testdata/ds3/in.txt

test4:
	$(COMPOSE) run --rm --build $(SERVICE) testdata/ds4/end_states.txt /tmp/out_ds4.csv testdata/ds4/in.txt

test5:
	$(COMPOSE) run --rm --build $(SERVICE) testdata/ds5/end_states.txt /tmp/out_ds5.csv \
		testdata/ds5/in1.txt testdata/ds5/in2.txt testdata/ds5/in3.txt

test-all: test1 test2 test3 test4 test5

clean:
	$(COMPOSE) down --remove-orphans
