#include "dashboard_ui.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {

constexpr char kTag[] = "dashboard_ui";
constexpr int kGaugeCount = 6;
constexpr int kRawMinimum = 0;
constexpr int kRawMaximum = 4095;
constexpr int kScreenWidth = 240;
constexpr int kScreenHeight = 240;
constexpr int kCenterX = 120;
constexpr int kCenterY = 112;
constexpr float kNeedleRadius = 72.0F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kNeedleStartDegrees = 135.0F;
constexpr float kNeedleSweepDegrees = 270.0F;

struct GaugeDefinition {
  const char* title;
  const char* unit;
  int minimum;
  int maximum;
};

constexpr GaugeDefinition kGauges[kGaugeCount] = {
  { "Gauge 1", "",  0,  40  },
  { "Gauge 2", "",  20, 100 },
  { "Gauge 3", "",  0,  60  },
  { "Gauge 4", "",  0,  80  },
  { "Gauge 5", "%", 0,  70  },
  { "Gauge 6", "",  80, 160 },
};

int clamp_gauge(int gauge) {
  if (gauge < 0) {
    return 0;
  }
  if (gauge >= kGaugeCount) {
    return kGaugeCount - 1;
  }
  return gauge;
}

int map_value(int input, int input_minimum, int input_maximum, int output_minimum, int output_maximum) {
  if (input_maximum <= input_minimum) {
    return output_minimum;
  }

  if (input < input_minimum) {
    input = input_minimum;
  } else if (input > input_maximum) {
    input = input_maximum;
  }

  const int64_t input_range = input_maximum - input_minimum;
  const int64_t output_range = output_maximum - output_minimum;
  return output_minimum + static_cast<int>(((input - input_minimum) * output_range) / input_range);
}

class DashboardUi;

struct ButtonContext {
  DashboardUi* ui;
  uint8_t gauge;
};

class DashboardUi {
public:
  esp_err_t init() {
    if (!lvgl_port_lock(1000)) {
      return ESP_ERR_TIMEOUT;
    }

    const bool created = create_screen();
    lvgl_port_unlock();
    return created ? ESP_OK : ESP_ERR_NO_MEM;
  }

  void select_gauge(int gauge) {
    selected_gauge_ = clamp_gauge(gauge);
    update_gauge();
    update_button_styles();
  }

  void advance_demo() {
    raw_value_ += 48;
    if (raw_value_ > kRawMaximum) {
      raw_value_ = kRawMinimum;
    }
    update_gauge();
  }

private:
  static void demo_timer_cb(lv_timer_t* timer) {
    auto* ui = static_cast<DashboardUi*>(lv_timer_get_user_data(timer));
    ui->advance_demo();
  }

  static void button_event_cb(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
      return;
    }
    auto* context = static_cast<ButtonContext*>(lv_event_get_user_data(event));
    context->ui->select_gauge(context->gauge);
  }

  bool create_screen() {
    lv_obj_t* screen = lv_screen_active();
    if (screen == nullptr) {
      return false;
    }
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x10141B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

    arc_ = lv_arc_create(screen);
    if (arc_ == nullptr) {
      return false;
    }
    lv_obj_set_size(arc_, 202, 202);
    lv_obj_set_pos(arc_, 19, 12);
    lv_arc_set_rotation(arc_, 135);
    lv_arc_set_bg_angles(arc_, 0, 270);
    lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_, lv_color_hex(0x28313C), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_, lv_color_hex(0x00D9A6), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc_, true, LV_PART_INDICATOR);

    needle_ = lv_line_create(screen);
    if (needle_ == nullptr) {
      return false;
    }
    lv_obj_set_size(needle_, kScreenWidth, kScreenHeight);
    lv_obj_set_pos(needle_, 0, 0);
    lv_obj_set_style_line_width(needle_, 4, LV_PART_MAIN);
    lv_obj_set_style_line_color(needle_, lv_color_hex(0xFF4D5A), LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle_, true, LV_PART_MAIN);

    hub_ = lv_obj_create(screen);
    if (hub_ == nullptr) {
      return false;
    }
    lv_obj_set_size(hub_, 16, 16);
    lv_obj_set_pos(hub_, kCenterX - 8, kCenterY - 8);
    lv_obj_set_style_radius(hub_, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(hub_, lv_color_hex(0xFF4D5A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hub_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(hub_, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(hub_, lv_color_hex(0x10141B), LV_PART_MAIN);

    title_ = lv_label_create(screen);
    if (title_ == nullptr) {
      return false;
    }
    lv_obj_set_width(title_, 180);
    lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_, lv_color_hex(0xDDE7F0), LV_PART_MAIN);

    value_ = lv_label_create(screen);
    if (value_ == nullptr) {
      return false;
    }
    lv_obj_set_width(value_, 140);
    lv_obj_align(value_, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(value_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    unit_ = lv_label_create(screen);
    if (unit_ == nullptr) {
      return false;
    }
    lv_obj_set_width(unit_, 120);
    lv_obj_align(unit_, LV_ALIGN_CENTER, 0, 34);
    lv_obj_set_style_text_align(unit_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(unit_, lv_color_hex(0x8EA0AE), LV_PART_MAIN);

    for (int index = 0; index < kGaugeCount; ++index) {
      ButtonContext& context = button_contexts_[index];
      context.ui = this;
      context.gauge = static_cast<uint8_t>(index);

      lv_obj_t* button = lv_button_create(screen);
      if (button == nullptr) {
        return false;
      }
      lv_obj_set_size(button, 32, 24);
      lv_obj_set_pos(button, 4 + index * 39, 211);
      lv_obj_add_event_cb(button, button_event_cb, LV_EVENT_CLICKED, &context);
      lv_obj_set_style_radius(button, 5, LV_PART_MAIN);
      lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
      lv_obj_set_style_border_color(button, lv_color_hex(0x344250), LV_PART_MAIN);

      lv_obj_t* button_label = lv_label_create(button);
      if (button_label == nullptr) {
        return false;
      }
      lv_label_set_text_fmt(button_label, "%d", index + 1);
      lv_obj_center(button_label);
      lv_obj_set_style_text_color(button_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      buttons_[index] = button;
    }

    update_gauge();
    update_button_styles();
    return lv_timer_create(demo_timer_cb, 33, this) != nullptr;
  }

  void update_gauge() {
    const GaugeDefinition& gauge = kGauges[selected_gauge_];
    const int display_value = map_value(raw_value_, kRawMinimum, kRawMaximum, gauge.minimum, gauge.maximum);
    const float ratio = static_cast<float>(display_value - gauge.minimum) / static_cast<float>(gauge.maximum - gauge.minimum);
    const float angle = (kNeedleStartDegrees + kNeedleSweepDegrees * ratio) * (kPi / 180.0F);

    lv_arc_set_range(arc_, gauge.minimum, gauge.maximum);
    lv_arc_set_value(arc_, display_value);
    needle_points_[0] = { kCenterX, kCenterY };
    needle_points_[1] = {
      static_cast<lv_value_precise_t>(kCenterX + kNeedleRadius * std::cos(angle)),
      static_cast<lv_value_precise_t>(kCenterY + kNeedleRadius * std::sin(angle)),
    };
    lv_line_set_points_mutable(needle_, needle_points_, 2);

    lv_label_set_text(title_, gauge.title);
    if (selected_gauge_ == 5) {
      char value_text[24];
      std::snprintf(value_text, sizeof(value_text), "%.1f", display_value / 10.0F);
      lv_label_set_text(value_, value_text);
    } else {
      lv_label_set_text_fmt(value_, "%d", display_value);
    }
    lv_label_set_text(unit_, gauge.unit);
  }

  void update_button_styles() {
    for (int index = 0; index < kGaugeCount; ++index) {
      const bool selected = index == selected_gauge_;
      lv_obj_set_style_bg_color(buttons_[index], lv_color_hex(selected ? 0x00A77F : 0x1B232D), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(buttons_[index], LV_OPA_COVER, LV_PART_MAIN);
    }
  }

  int selected_gauge_ = 0;
  int raw_value_ = 0;
  lv_obj_t* arc_ = nullptr;
  lv_obj_t* needle_ = nullptr;
  lv_obj_t* hub_ = nullptr;
  lv_obj_t* title_ = nullptr;
  lv_obj_t* value_ = nullptr;
  lv_obj_t* unit_ = nullptr;
  lv_obj_t* buttons_[kGaugeCount] = {};
  ButtonContext button_contexts_[kGaugeCount] = {};
  lv_point_precise_t needle_points_[2] = {};
};

DashboardUi s_dashboard_ui;

} // namespace

extern "C" esp_err_t dashboard_ui_init() {
  ESP_LOGI(kTag, "Creating LVGL gauge demo");
  return s_dashboard_ui.init();
}
