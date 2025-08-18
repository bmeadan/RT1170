CXX      := g++
CXXFLAGS := -std=c++17 -O0 -g -pthread -DHOST_SIM -I. -I.gen/redirects -I..
LDFLAGS  :=

SRCS := \
  sim.cpp \
  ../controllers/FlipController.cpp

REDIR_HEADERS := \
  FreeRTOS.h \
  task.h \
  project.h \
  fsl_common.h \
  fsl_pwm.h \
  fsl_lpuart.h \
  fsl_flexspi.h \
  fsl_clock.h \
  fsl_gpio.h \
  fsl_iomuxc.h \
  motors/motors.h \
  motors/RSBL8512.h \
  motors/TvaiDriveMotor.h \
  motors/AlonDriveMotor.h \
  imu/imu.h \
  platform/platform.h \
  usb/host_video.h \
  audio/audio.h \
  lights/lights.h \
  flash/flash.h \
  serial/serial_lpuart_232.h \
  serial/serial_lpuart_485.h \
  flexspi/flexspi_nor_flash_ops.h

GEN_DIR := .gen/redirects

.PHONY: all clean run
all: sim

sim: $(GEN_DIR)/.done $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)

run: sim
	./sim

$(GEN_DIR)/.done: mock_all.h
	@mkdir -p $(GEN_DIR)
	@for h in $(REDIR_HEADERS); do \
	  subdir=$${h%/*}; \
	  if [ "$$subdir" != "$$h" ]; then mkdir -p "$(GEN_DIR)/$$subdir"; fi; \
	  printf '%s\n%s\n' '#pragma once' '#include <mock_all.h>' > "$(GEN_DIR)/$$h"; \
	done
	@touch $@

clean:
	rm -rf .gen sim
