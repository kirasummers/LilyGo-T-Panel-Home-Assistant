#include "cst3240_touchscreen.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cst3240 {

static const char *const TAG = "cst3240.touchscreen";

void CST3240Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CST3240 Touchscreen at 0x%02X...", this->address_);

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    this->set_timeout(50, [this] { this->reset_device_(); });
  } else {
    this->continue_setup_();
  }
}

void CST3240Touchscreen::reset_device_() {
  pulse_reset(this->reset_pin_, {.low_ms = 10, .high_after_ms = 50});
  this->set_timeout(200, [this] { this->continue_setup_(); });
}

void CST3240Touchscreen::continue_setup_() {
  ESP_LOGCONFIG(TAG, "Continuing CST3240 setup...");

  uint8_t dummy = 0;
  // Enter normal mode (0xD109)
  this->write_register16(CST_WM_NORMAL, &dummy, 0);

  // Send initial sync byte to 0xD000
  uint8_t sync = CST3240_SYNC_BYTE;
  this->write_register16(CST_REG_TOUCH_INFORMATION, &sync, 1);

  // Default coordinate calibration to 480x480 if not set
  if (this->x_raw_max_ == this->x_raw_min_) {
    this->x_raw_min_ = 0;
    this->x_raw_max_ = (this->display_ != nullptr) ? this->display_->get_width() : 480;
  }
  if (this->y_raw_max_ == this->y_raw_min_) {
    this->y_raw_min_ = 0;
    this->y_raw_max_ = (this->display_ != nullptr) ? this->display_->get_height() : 480;
  }

  if (this->display_ != nullptr) {
    this->display_width_ = this->display_->get_width();
    this->display_height_ = this->display_->get_height();
  } else {
    this->display_width_ = 480;
    this->display_height_ = 480;
  }

  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);
    ESP_LOGI(TAG, "CST3240 interrupt attached to pin %d", this->interrupt_pin_->get_pin());
  }

  this->setup_done_ = true;
  ESP_LOGI(TAG, "CST3240 initialized at 0x%02X, resolution %dx%d", this->address_, this->x_raw_max_, this->y_raw_max_);
}

void CST3240Touchscreen::update_touches() {
  if (!this->setup_done_) {
    this->skip_update_ = true;
    return;
  }

  uint8_t touch_data[CST3240_TOUCH_DATA_SIZE] = {0};

  if (i2c::ERROR_OK != this->read_register16(CST_REG_TOUCH_INFORMATION, touch_data, CST3240_TOUCH_DATA_SIZE)) {
    ESP_LOGW(TAG, "Failed to read touch data from 0x%02X", this->address_);
    this->status_set_warning();
    this->skip_update_ = true;
    return;
  }

  this->status_clear_warning();

  uint8_t touch_cnt = touch_data[CST_REG_FINGER_COUNT_IDX] & 0x0F;

  if (touch_cnt > 0 && touch_cnt <= CST3240_TOUCH_MAX_POINTS) {
    uint8_t data_idx = 0;
    for (uint8_t i = 0; i < touch_cnt; i++) {
      uint8_t id = touch_data[data_idx] >> 4;
      int16_t x = (touch_data[data_idx + 1] << 4) | ((touch_data[data_idx + 3] >> 4) & 0x0F);
      int16_t y = (touch_data[data_idx + 2] << 4) | (touch_data[data_idx + 3] & 0x0F);
      int16_t z = touch_data[data_idx + 4];

      if (x < 0) x = 0;
      if (x > this->x_raw_max_) x = this->x_raw_max_;
      if (y < 0) y = 0;
      if (y > this->y_raw_max_) y = this->y_raw_max_;

      this->add_raw_touch_position_(id, x, y, z);
      ESP_LOGD(TAG, "Touch %d (id=%d) -> X=%d Y=%d", i, id, x, y);
      data_idx += (i == 0) ? 7 : 5;
    }
  }

  // Acknowledge/clear touch registers so chip is ready for next read
  uint8_t zero = ZERO_BYTE;
  uint8_t sync = CST3240_SYNC_BYTE;
  this->write_register16(CST_REG_TOUCH_FINGER_NUMBER, &zero, 1);
  this->write_register16(CST_REG_TOUCH_INFORMATION, &sync, 1);
}

void CST3240Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "CST3240 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  X/Y resolution: %d/%d", this->x_raw_max_, this->y_raw_max_);
}

}  // namespace cst3240
}  // namespace esphome
