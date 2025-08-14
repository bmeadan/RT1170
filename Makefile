.phony: all format venv

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CLANG_FORMAT := $(shell find ~/.vscode/extensions -name "clang-format")
SRC_FILES := $(shell find $(MAKEFILE_DIR)src -name "*.c" -o -name "*.h" -o  -name "*.cpp" -o -name "*.hpp")
PROJECTS_FILES := $(shell find $(MAKEFILE_DIR)projects -name "*.c" -o -name "*.h" -o  -name "*.cpp" -o -name "*.hpp")
DEMO_DIR := $(MAKEFILE_DIR)demo

all:

format:
	@${CLANG_FORMAT} -i ${SRC_FILES} ${PROJECTS_FILES}

venv:
	@cd $(DEMO_DIR) && python3 -m venv venv && . venv/bin/activate && pip install -r requirements.txt

demo-remote:
	@. $(DEMO_DIR)/venv/bin/activate && python -m demo.main_remote

demo-platform:
	@. $(DEMO_DIR)/venv/bin/activate && python -m demo.main_platform

demo-firmware:
	@. $(DEMO_DIR)/venv/bin/activate && python -m demo.main_firmware

demo-remote-watch:
	@. $(DEMO_DIR)/venv/bin/activate && nodemon -w $(DEMO_DIR) --exec "python -m demo.main_remote || exit 1" --ext py 

demo-platform-watch:
	@. $(DEMO_DIR)/venv/bin/activate && nodemon -w $(DEMO_DIR) --exec "python -m demo.main_platform || exit 1" --ext py

demo-firmware-watch:
	@. $(DEMO_DIR)/venv/bin/activate && nodemon -w $(DEMO_DIR) --exec "python -m demo.main_firmware || exit 1" --ext py
