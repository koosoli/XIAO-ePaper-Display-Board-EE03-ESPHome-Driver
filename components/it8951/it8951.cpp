#include "it8951.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace it8951 {

static const char *const TAG = "it8951";
static const uint32_t IT8951_SPI_PROBE_FREQUENCY = 1000000;
static const uint32_t IT8951_SPI_RUN_FREQUENCY = 4000000;

void IT8951Display::spi_send_word_(uint16_t word) { SPI.transfer16(word); }

uint16_t IT8951Display::spi_recv_word_() { return SPI.transfer16(0); }

void IT8951Display::lcd_wait_for_ready_() {
  const uint32_t start = millis();
  while (digitalRead(IT8951_PIN_BUSY) == LOW) {
    if (millis() - start > 3000) {
      ESP_LOGE(TAG, "HRDY timeout!");
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void IT8951Display::hardware_reset_() {
  digitalWrite(IT8951_PIN_CS, HIGH);
  digitalWrite(IT8951_PIN_RST, HIGH);
  delay(50);
  digitalWrite(IT8951_PIN_RST, LOW);
  delay(10);
  digitalWrite(IT8951_PIN_RST, HIGH);
  delay(10);
}

void IT8951Display::power_cycle_() {
  digitalWrite(IT8951_PIN_CS, HIGH);
  digitalWrite(IT8951_PIN_RST, HIGH);
  digitalWrite(IT8951_PIN_EN, LOW);
  delay(100);
  digitalWrite(IT8951_PIN_EN, HIGH);
  delay(500);
  this->hardware_reset_();
  delay(1500);
}

void IT8951Display::lcd_write_cmd_code_(uint16_t cmd) {
  digitalWrite(IT8951_PIN_CS, LOW);
  SPI.beginTransaction(SPISettings(this->spi_frequency_, MSBFIRST, SPI_MODE0));
  this->lcd_wait_for_ready_();
  this->spi_send_word_(0x6000);
  this->lcd_wait_for_ready_();
  this->spi_send_word_(cmd);
  SPI.endTransaction();
  digitalWrite(IT8951_PIN_CS, HIGH);
}

void IT8951Display::lcd_write_data_(uint16_t data) {
  digitalWrite(IT8951_PIN_CS, LOW);
  SPI.beginTransaction(SPISettings(this->spi_frequency_, MSBFIRST, SPI_MODE0));
  this->lcd_wait_for_ready_();
  this->spi_send_word_(0x0000);
  this->lcd_wait_for_ready_();
  this->spi_send_word_(data);
  SPI.endTransaction();
  digitalWrite(IT8951_PIN_CS, HIGH);
}

void IT8951Display::lcd_write_n_data_(uint16_t *buf, uint32_t word_count) {
  digitalWrite(IT8951_PIN_CS, LOW);
  SPI.beginTransaction(SPISettings(this->spi_frequency_, MSBFIRST, SPI_MODE0));
  this->lcd_wait_for_ready_();
  this->spi_send_word_(0x0000);
  this->lcd_wait_for_ready_();
  for (uint32_t i = 0; i < word_count; i++) {
    this->spi_send_word_(buf[i]);
  }
  SPI.endTransaction();
  digitalWrite(IT8951_PIN_CS, HIGH);
}

uint16_t IT8951Display::lcd_read_data_() {
  digitalWrite(IT8951_PIN_CS, LOW);
  SPI.beginTransaction(SPISettings(this->spi_frequency_, MSBFIRST, SPI_MODE0));
  this->lcd_wait_for_ready_();
  this->spi_send_word_(0x1000);
  this->spi_recv_word_();
  this->lcd_wait_for_ready_();
  const uint16_t data = this->spi_recv_word_();
  SPI.endTransaction();
  digitalWrite(IT8951_PIN_CS, HIGH);
  return data;
}

void IT8951Display::lcd_read_n_data_(uint16_t *buf, uint32_t word_count) {
  digitalWrite(IT8951_PIN_CS, LOW);
  SPI.beginTransaction(SPISettings(this->spi_frequency_, MSBFIRST, SPI_MODE0));
  this->lcd_wait_for_ready_();
  this->spi_send_word_(0x1000);
  this->lcd_wait_for_ready_();
  this->spi_recv_word_();
  this->lcd_wait_for_ready_();
  for (uint32_t i = 0; i < word_count; i++) {
    buf[i] = this->spi_recv_word_();
  }
  SPI.endTransaction();
  digitalWrite(IT8951_PIN_CS, HIGH);
}

void IT8951Display::lcd_sys_run_() { this->lcd_write_cmd_code_(IT8951_TCON_SYS_RUN); }

void IT8951Display::write_vcom_(uint16_t selector, uint16_t value) {
  this->lcd_write_cmd_code_(USDEF_I80_CMD_VCOM);
  this->lcd_write_data_(selector);
  this->lcd_write_data_(value);
}

bool IT8951Display::has_valid_dev_info_() const {
  return this->dev_info_.panel_width > 0 && this->dev_info_.panel_width < 10000 &&
         this->dev_info_.panel_height > 0 && this->dev_info_.panel_height < 10000;
}

void IT8951Display::log_dev_info_words_(const char *label) {
  uint16_t *raw = reinterpret_cast<uint16_t *>(&this->dev_info_);
  ESP_LOGI(TAG, "[%s] DevInfo raw [W=%u H=%u BufL=0x%04X BufH=0x%04X]", label, raw[0], raw[1], raw[2], raw[3]);
  ESP_LOGI(TAG, "[%s] DevInfo FW: %02X %02X %02X %02X %02X %02X %02X %02X", label, raw[4], raw[5], raw[6], raw[7],
           raw[8], raw[9], raw[10], raw[11]);
  ESP_LOGI(TAG, "[%s] DevInfo LUT: %02X %02X %02X %02X %02X %02X %02X %02X", label, raw[12], raw[13], raw[14],
           raw[15], raw[16], raw[17], raw[18], raw[19]);
}

bool IT8951Display::probe_controller_(const char *label, bool send_sys_run, int vcom_selector) {
  this->probe_path_ = label;
  this->probe_vcom_ = 0;
  memset(&this->dev_info_, 0, sizeof(this->dev_info_));

  if (send_sys_run) {
    ESP_LOGI(TAG, "[%s] Sending SYS_RUN wake command", label);
    this->lcd_sys_run_();
    delay(10);
  }

  if (vcom_selector > 0) {
    ESP_LOGI(TAG, "[%s] Writing VCOM=%u with selector 0x%04X", label, this->vcom_, vcom_selector);
    this->write_vcom_(vcom_selector, this->vcom_);
    this->lcd_write_cmd_code_(USDEF_I80_CMD_VCOM);
    this->lcd_write_data_(0x0000);
    this->probe_vcom_ = this->lcd_read_data_();
    ESP_LOGI(TAG, "[%s] VCOM read-back: %u (0x%04X)", label, this->probe_vcom_, this->probe_vcom_);
    if (this->probe_vcom_ == this->vcom_) {
      this->vcom_write_selector_ = vcom_selector;
    }
  }

  this->get_it8951_system_info_();
  this->log_dev_info_words_(label);
  return this->has_valid_dev_info_();
}

void IT8951Display::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IT8951...");

  pinMode(IT8951_PIN_CS, OUTPUT);
  pinMode(IT8951_PIN_EN, OUTPUT);
  pinMode(IT8951_PIN_RST, OUTPUT);
  pinMode(IT8951_PIN_BUSY, INPUT);

  digitalWrite(IT8951_PIN_CS, HIGH);
  digitalWrite(IT8951_PIN_EN, HIGH);
  digitalWrite(IT8951_PIN_RST, HIGH);
  ESP_LOGI(TAG, "ENABLE HIGH, RESET HIGH, CS HIGH");

  this->spi_frequency_ = IT8951_SPI_PROBE_FREQUENCY;
  SPI.begin(IT8951_PIN_SCK, IT8951_PIN_MISO, IT8951_PIN_MOSI, -1);
  ESP_LOGI(TAG, "SPI bus initialized at %u Hz probe speed", this->spi_frequency_);

  struct ProbeAttempt {
    const char *label;
    bool send_sys_run;
    int vcom_selector;
  };
  static const ProbeAttempt attempts[] = {
      {"cold read", false, 0},
      {"wake then read", true, 0},
      {"wake + VCOM 0x0001", true, 0x0001},
      {"wake + VCOM 0x0002", true, 0x0002},
  };

  bool found_device = false;
  for (size_t i = 0; i < sizeof(attempts) / sizeof(attempts[0]); i++) {
    ESP_LOGI(TAG, "Probe attempt %u: %s", static_cast<unsigned>(i + 1), attempts[i].label);
    this->power_cycle_();
    ESP_LOGI(TAG, "[%s] Power cycle complete, HRDY=%s", attempts[i].label,
             digitalRead(IT8951_PIN_BUSY) ? "HIGH" : "LOW");
    this->lcd_wait_for_ready_();
    if (this->probe_controller_(attempts[i].label, attempts[i].send_sys_run, attempts[i].vcom_selector)) {
      found_device = true;
      break;
    }
  }

  if (!found_device) {
    ESP_LOGE(TAG, "Invalid panel size! The EE03 holds MISO low when the IT8951 stays silent.");
    this->fail_reason_ = "SPI communication failed - IT8951 never returned valid device info";
    this->mark_failed();
    return;
  }

  if (this->vcom_write_selector_ == 0) {
    ESP_LOGI(TAG, "Panel answered before VCOM was verified, trying preferred selector 0x0002");
    this->write_vcom_(0x0002, this->vcom_);
    this->lcd_write_cmd_code_(USDEF_I80_CMD_VCOM);
    this->lcd_write_data_(0x0000);
    this->probe_vcom_ = this->lcd_read_data_();
    if (this->probe_vcom_ == this->vcom_) {
      this->vcom_write_selector_ = 0x0002;
    } else {
      ESP_LOGW(TAG, "Selector 0x0002 read-back was %u, trying selector 0x0001", this->probe_vcom_);
      this->write_vcom_(0x0001, this->vcom_);
      this->lcd_write_cmd_code_(USDEF_I80_CMD_VCOM);
      this->lcd_write_data_(0x0000);
      this->probe_vcom_ = this->lcd_read_data_();
      if (this->probe_vcom_ == this->vcom_) {
        this->vcom_write_selector_ = 0x0001;
      }
    }
    ESP_LOGI(TAG, "Post-detect VCOM read-back: %u (0x%04X)", this->probe_vcom_, this->probe_vcom_);
  }

  this->img_buf_addr_ = (uint32_t(this->dev_info_.img_buf_addr_h) << 16) | this->dev_info_.img_buf_addr_l;
  ESP_LOGI(TAG, "Panel: %dx%d, ImgBuf: 0x%08X", this->dev_info_.panel_width, this->dev_info_.panel_height,
           this->img_buf_addr_);

  this->it8951_write_reg_(I80CPCR, 0x0001);
  this->lcd_write_cmd_code_(USDEF_I80_CMD_TEMP);
  this->lcd_write_data_(0x0001);
  this->lcd_write_data_(14);

  this->spi_frequency_ = IT8951_SPI_RUN_FREQUENCY;
  ESP_LOGI(TAG, "Switching SPI speed to %u Hz for image transfers", this->spi_frequency_);

  const uint32_t buffer_size = (uint32_t(this->get_width_internal()) * this->get_height_internal()) / 2;
  this->framebuffer_ = static_cast<uint8_t *>(heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM));
  if (this->framebuffer_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes in PSRAM!", buffer_size);
    this->fail_reason_ = "PSRAM allocation failed";
    this->mark_failed();
    return;
  }
  memset(this->framebuffer_, 0xFF, buffer_size);

  ESP_LOGCONFIG(TAG, "IT8951 initialization complete");
}

void IT8951Display::update() {
  if (this->framebuffer_ == nullptr) {
    ESP_LOGW(TAG, "Skipping update because the framebuffer is not available");
    return;
  }

  this->do_update_();

  ESP_LOGD(TAG, "Transferring image to IT8951...");

  const uint16_t w = this->get_width_internal();
  const uint16_t h = this->get_height_internal();

  this->set_img_buf_base_addr_(this->img_buf_addr_);
  this->it8951_load_img_area_start_(IT8951_LDIMG_L_ENDIAN, IT8951_4BPP, 0, 0, 0, w, h);

  const uint16_t width_in_words = (w + 3) / 4;
  this->lcd_write_n_data_(reinterpret_cast<uint16_t *>(this->framebuffer_), uint32_t(width_in_words) * h);

  this->lcd_write_cmd_code_(IT8951_TCON_LD_IMG_END);
  this->it8951_display_area_(0, 0, w, h, 2);

  ESP_LOGV(TAG, "Display update triggered");
}

void IT8951Display::dump_config() {
  LOG_DISPLAY("", "IT8951", this);
  ESP_LOGCONFIG(TAG, "  SPI Frequency: %u Hz", this->spi_frequency_);
  ESP_LOGCONFIG(TAG, "  VCOM: %u", this->vcom_);
  ESP_LOGCONFIG(TAG, "  VCOM selector: 0x%04X", this->vcom_write_selector_);
  ESP_LOGCONFIG(TAG, "  Probe path: %s", this->probe_path_ != nullptr ? this->probe_path_ : "none");
  ESP_LOGCONFIG(TAG, "  DevInfo Panel: %ux%u", this->dev_info_.panel_width, this->dev_info_.panel_height);
  ESP_LOGCONFIG(TAG, "  DevInfo ImgBuf: 0x%04X%04X", this->dev_info_.img_buf_addr_h, this->dev_info_.img_buf_addr_l);
  ESP_LOGCONFIG(TAG, "  Buffer allocated: %s", this->framebuffer_ != nullptr ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Busy pin: %s", digitalRead(IT8951_PIN_BUSY) ? "HIGH (ready)" : "LOW (busy)");
  ESP_LOGCONFIG(TAG, "  VCOM read-back: %u (0x%04X)", this->probe_vcom_, this->probe_vcom_);
  if (this->fail_reason_ != nullptr) {
    ESP_LOGE(TAG, "  FAILURE REASON: %s", this->fail_reason_);
  }
  LOG_UPDATE_INTERVAL(this);
}

void IT8951Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= this->get_width() || y < 0 || y >= this->get_height()) {
    return;
  }
  if (this->framebuffer_ == nullptr) {
    return;
  }

  const uint32_t pos = (x + y * this->get_width()) / 2;
  const uint8_t pixel_val = color.is_on() ? 0x00 : 0x0F;

  if ((x % 2) == 0) {
    this->framebuffer_[pos] = (this->framebuffer_[pos] & 0x0F) | (pixel_val << 4);
  } else {
    this->framebuffer_[pos] = (this->framebuffer_[pos] & 0xF0) | pixel_val;
  }
}

void IT8951Display::lcd_send_cmd_arg_(uint16_t cmd, uint16_t *args, uint16_t num_args) {
  this->lcd_write_cmd_code_(cmd);
  for (uint16_t i = 0; i < num_args; i++) {
    this->lcd_write_data_(args[i]);
  }
}

uint16_t IT8951Display::it8951_read_reg_(uint16_t addr) {
  this->lcd_write_cmd_code_(IT8951_TCON_REG_RD);
  this->lcd_write_data_(addr);
  return this->lcd_read_data_();
}

void IT8951Display::it8951_write_reg_(uint16_t addr, uint16_t val) {
  this->lcd_write_cmd_code_(IT8951_TCON_REG_WR);
  this->lcd_write_data_(addr);
  this->lcd_write_data_(val);
}

void IT8951Display::get_it8951_system_info_() {
  memset(&this->dev_info_, 0, sizeof(this->dev_info_));
  this->lcd_write_cmd_code_(USDEF_I80_CMD_GET_DEV_INFO);
  this->lcd_read_n_data_(reinterpret_cast<uint16_t *>(&this->dev_info_), sizeof(IT8951DevInfo) / 2);
}

void IT8951Display::set_img_buf_base_addr_(uint32_t addr) {
  const uint16_t hi = (addr >> 16) & 0xFFFF;
  const uint16_t lo = addr & 0xFFFF;
  this->it8951_write_reg_(LISAR + 2, hi);
  this->it8951_write_reg_(LISAR, lo);
}

void IT8951Display::it8951_load_img_area_start_(uint16_t endian, uint16_t pix_fmt, uint16_t rotate, uint16_t x,
                                                 uint16_t y, uint16_t w, uint16_t h) {
  uint16_t args[5];
  args[0] = (endian << 8) | (pix_fmt << 4) | rotate;
  args[1] = x;
  args[2] = y;
  args[3] = w;
  args[4] = h;
  this->lcd_send_cmd_arg_(IT8951_TCON_LD_IMG_AREA, args, 5);
}

void IT8951Display::it8951_display_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode) {
  this->lcd_write_cmd_code_(USDEF_I80_CMD_DPY_AREA);
  this->lcd_write_data_(x);
  this->lcd_write_data_(y);
  this->lcd_write_data_(w);
  this->lcd_write_data_(h);
  this->lcd_write_data_(mode);
}

}  // namespace it8951
}  // namespace esphome
