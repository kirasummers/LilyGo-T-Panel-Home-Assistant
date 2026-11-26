#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cst3240 {

static const uint8_t CST3240_TOUCH_MAX_POINTS = 5;
static const uint8_t CST3240_TOUCH_DATA_SIZE = CST3240_TOUCH_MAX_POINTS * 5 + 2;

static const uint16_t CST_REG_TOUCH_INFORMATION = 0xD000;
static const uint16_t CST_REG_TOUCH_FINGER_NUMBER = 0xD005;
static const uint16_t CST_REG_FINGER_COUNT_IDX = 5;

static const uint16_t CST_WM_NORMAL = 0xD109;

static const uint8_t CST3240_SYNC_BYTE = 0xAB;
static const uint8_t ZERO_BYTE = 0;

class CST3240Touchscreen : public touchscreen::Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  bool can_proceed() override { return this->setup_done_; }

  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }

 protected:
  void update_touches() override;
  void reset_device_();
  void continue_setup_();

  bool setup_done_{false};

  InternalGPIOPin *interrupt_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
};

}  // namespace cst3240
}  // namespace esphome
