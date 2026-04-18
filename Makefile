END_STATES  ?= test_ds1/end_states.txt
INPUTS      ?= test_ds1/in.txt
OUT         ?= out/out.csv
DOCKER_OUT  ?= /out/out.csv

BUILD_RELEASE ?= build-release
BUILD_DEBUG   ?= build-debug

COMPOSE := docker-compose
SERVICE := app

APP_RELEASE := $(BUILD_RELEASE)/app
APP_DEBUG   := $(BUILD_DEBUG)/app

.DEFAULT_GOAL := native-release

.PHONY: native-release native-debug native-run native-run-debug native-clean \
        test1 test2 test3 test4 test5 test-all \
        build run run-debug \
        docker-test1 docker-test2 docker-test3 docker-test4 docker-test5 docker-test-all \
        clean

DIFF = diff --strip-trailing-cr

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

native-clean:
	rm -rf $(BUILD_RELEASE) $(BUILD_DEBUG)

# ── Тесты ────────────────────────────────────────────────────────────────────

test1: native-release
	$(APP_RELEASE) test_ds1/end_states.txt out/out_ds1.csv test_ds1/in.txt
	$(DIFF) test_ds1/out.csv out/out_ds1.csv

test2: native-release
	$(APP_RELEASE) test_ds2/end_states.txt out/out_ds2.csv test_ds2/in.txt
	$(DIFF) test_ds2/out.csv out/out_ds2.csv

test3: native-release
	$(APP_RELEASE) datasets/3/end_states.txt out/out_ds3.csv datasets/3/in.txt

test4: native-release
	$(APP_RELEASE) datasets/4/end_states.txt out/out_ds4.csv datasets/4/in.txt

test5: native-release
	$(APP_RELEASE) datasets/5/end_states.txt out/out_ds5.csv \
		datasets/5/in1.txt datasets/5/in2.txt datasets/5/in3.txt

test-all: test1 test2 test3 test4 test5

# ── Docker ───────────────────────────────────────────────────────────────────

build:
	$(COMPOSE) build $(SERVICE)

run:
	time $(COMPOSE) run --rm --build $(SERVICE) $(END_STATES) $(DOCKER_OUT) $(INPUTS)

run-debug:
	time $(COMPOSE) run --rm --build --entrypoint /app/build-debug/app $(SERVICE) $(END_STATES) $(DOCKER_OUT) $(INPUTS)

docker-test1:
	$(COMPOSE) run --rm --build $(SERVICE) test_ds1/end_states.txt /out/out_ds1.csv test_ds1/in.txt
	$(DIFF) test_ds1/out.csv out/out_ds1.csv

docker-test2:
	$(COMPOSE) run --rm --build $(SERVICE) test_ds2/end_states.txt /out/out_ds2.csv test_ds2/in.txt
	$(DIFF) test_ds2/out.csv out/out_ds2.csv

docker-test3:
	$(COMPOSE) run --rm --build $(SERVICE) datasets/3/end_states.txt /out/out_ds3.csv datasets/3/in.txt

docker-test4:
	$(COMPOSE) run --rm --build $(SERVICE) datasets/4/end_states.txt /out/out_ds4.csv datasets/4/in.txt

docker-test5:
	$(COMPOSE) run --rm --build $(SERVICE) datasets/5/end_states.txt /out/out_ds5.csv \
		datasets/5/in1.txt datasets/5/in2.txt datasets/5/in3.txt

docker-test-all: docker-test1 docker-test2 docker-test3 docker-test4 docker-test5

clean:
	$(COMPOSE) down --remove-orphans
