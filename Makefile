END_STATES ?= test_ds1/end_states.txt
INPUT ?= test_ds1/in.txt

COMPOSE := docker-compose
SERVICE := app

.DEFAULT_GOAL := help

.PHONY: help build run test1 test2 clean

help:
	@printf '%s\n' \
		'Targets:' \
		'  make build                         Build Docker image' \
		'  make run                           Run app with default test_ds1 paths' \
		'  make run INPUT=path/to/in.txt      Run app with custom input file' \
		'  make run END_STATES=path INPUT=path Run app with custom end states and input' \
		'  make test1                         Run test_ds1' \
		'  make test2                         Run test_ds2' \
		'  make clean                         Stop compose containers and remove orphans'

build:
	$(COMPOSE) build $(SERVICE)

run:
	$(COMPOSE) run --rm --build $(SERVICE) $(END_STATES) $(INPUT)

test1:
	$(COMPOSE) run --rm --build $(SERVICE) test_ds1/end_states.txt test_ds1/in.txt

test2:
	$(COMPOSE) run --rm --build $(SERVICE) test_ds2/end_states.txt test_ds2/in.txt

clean:
	$(COMPOSE) down --remove-orphans
