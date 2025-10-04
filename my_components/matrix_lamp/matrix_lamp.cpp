#include "esphome.h"
#include "common.h"
#include "constants.h"
#include "utility.h"
#include "effect_ticker.h"

namespace esphome {
namespace matrix_lamp {

using namespace fastled_helper;

void MatrixLamp::setup() {
  randomSeed(micros());
  restoreSettings();

  loadingFlag = true;

  #if defined(MATRIX_LAMP_USE_DISPLAY)
  this->hide_icon();

  this->last_anim_time = 0;
  this->brightness = 255;
  this->target_brightness = 255;

  #ifdef MATRIX_LAMP_BITMAP_MODE
  this->bitmap = nullptr;
  #endif

  #if defined(USE_API)
  register_service(&MatrixLamp::set_brightness, "brightness", {"value"});
  register_service(&MatrixLamp::show_icon, "show_icon", {"icon_name"});
  #ifdef MATRIX_LAMP_BITMAP_MODE
  register_service(&MatrixLamp::show_bitmap, "show_bitmap", {"bitmap"});
  #endif
  register_service(&MatrixLamp::hide_icon, "hide_icon");
  #endif
  #endif
  
  #ifdef USE_API_SERVICES
  // Set brightness for current effect
  register_service(&MatrixLamp::set_effect_intensity, "set_effect_intensity", {"value"});
  // Set speed for current effect
  register_service(&MatrixLamp::set_effect_speed, "set_effect_speed", {"value"});
  // Set scale for current effect
  register_service(&MatrixLamp::set_effect_scale, "set_effect_scale", {"value"});
  // Reset brightness, speed, scale to default for current effect
  register_service(&MatrixLamp::reset_effect_settings, "reset_effect_settings");
  #endif // #ifdef USE_API_SERVICES
  
}  // setup()

void MatrixLamp::dump_config() {
  ESP_LOGCONFIG(TAG, "MatrixLamp version: %s", MATRIX_LAMP_VERSION);
  ESP_LOGCONFIG(TAG, "             Width: %d", WIDTH);
  ESP_LOGCONFIG(TAG, "            Height: %d", HEIGHT);
  #if defined(MATRIX_LAMP_USE_DISPLAY)
  ESP_LOGCONFIG(TAG, "           Display: YES");
  ESP_LOGCONFIG(TAG, "   Number of icons: %d", this->icon_count);
  #endif
  #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  ESP_LOGCONFIG(TAG, "Random mode enable: YES");
  #else
  ESP_LOGCONFIG(TAG, "Random mode enable: NO");
  #endif
  ESP_LOGCONFIG(TAG, "       Orientation: %d", ORIENTATION);
  ESP_LOGCONFIG(TAG, "       Matrix Type: %d", MATRIX_TYPE);
  ESP_LOGCONFIG(TAG, "   Number of modes: %d", MODE_AMOUNT);
}  // dump_config()

void MatrixLamp::on_shutdown()
{
  FreeLeds();
}

#if defined(MATRIX_LAMP_USE_DISPLAY)
void MatrixLamp::set_display(addressable_light::AddressableLightDisplay *display)
{
  this->display = display;
}

void MatrixLamp::add_icon(MatrixLamp_Icon *icon)
{
  this->icons[this->icon_count] = icon;
  ESP_LOGD(TAG, "Add Icon No.: %d name: %s frame_duration: %d ms", this->icon_count, icon->name.c_str(), icon->frame_duration);
  this->icon_count++;
}
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

void MatrixLamp::set_intensity(template_::TemplateNumber *intensity) {
  this->intensity = intensity;
} // set_intensity()

void MatrixLamp::set_scale(template_::TemplateNumber *scale) {
  this->scale = scale;
} // set_scale()

void MatrixLamp::set_speed(template_::TemplateNumber *speed) {
  this->speed = speed;
} // set_speed()

void MatrixLamp::reset_current_effect()
{
  currentMode = MODE_AMOUNT;
} // reset_current_effect()

void MatrixLamp::set_intensity_for_effect(uint8_t mode, uint8_t intensity)
{
  if (mode >= MODE_AMOUNT) {
    return;
  }
  if (intensity > 255) {
    intensity = 255;
  }
  modes[mode].Brightness = intensity;
  loadingFlag = true;
} // set_intensity_for_effect()

uint8_t MatrixLamp::get_intensity_for_effect(uint8_t mode)
{
  if (mode >= MODE_AMOUNT) {
    return 0;
  }
  return modes[mode].Brightness;
} // get_intensity_for_effect()

void MatrixLamp::set_scale_for_effect(uint8_t mode, uint8_t scale)
{
  if (mode >= MODE_AMOUNT) {
    return;
  }
  if (scale > 255) {
    scale = 255;
  }
  modes[mode].Scale = scale;
  loadingFlag = true;
} // set_scale_for_effect()

uint8_t MatrixLamp::get_scale_for_effect(uint8_t mode)
{
  if (mode >= MODE_AMOUNT) {
    return 0;
  }
  return modes[mode].Scale;
} // get_scale_for_effect()

void MatrixLamp::set_speed_for_effect(uint8_t mode, uint8_t speed)
{
  if (mode >= MODE_AMOUNT) {
    return;
  }
  if (speed > 100) {
    speed = 100;
  }
  modes[mode].Speed = speed;
  loadingFlag = true;
} // set_speed_for_effect()

uint8_t MatrixLamp::get_speed_for_effect(uint8_t mode)
{
  if (mode >= MODE_AMOUNT) {
    return 0;
  }
  return modes[mode].Speed;
} // get_speed_for_effect()

void MatrixLamp::set_scale_from_color_for_effect(uint8_t mode, Color color)
{
  if (mode >= MODE_AMOUNT) {
    return;
  }

  if (color.red == 255 && color.green == 255 && color.blue == 255)
  {
    this->set_scale_for_effect(mode, 0);
  }
  else
  {
    int hue;
    float saturation, value;
    rgb_to_hsv(color.red / 255, color.green / 255, color.blue / 255, hue, saturation, value);

    this->set_scale_for_effect(mode, remap(hue, 0, 360, 1, 100));
  }
} // set_scale_from_color_for_effect()

void MatrixLamp::set_custom_effect_mode(uint8_t mode)
{
  custom_eff = mode;
} // set_custom_effect_mode()

// Set / Get intensity for current effect
void MatrixLamp::set_effect_intensity(int32_t value)
{
  this->set_intensity_for_effect(currentMode, value);
} // set_effect_intensity()

uint8_t MatrixLamp::get_effect_intensity()
{
  return this->get_intensity_for_effect(currentMode);
} // get_effect_intensity()

// Set / Get speed for current effect
void MatrixLamp::set_effect_speed(int32_t value)
{
  this->set_speed_for_effect(currentMode, value);
} // set_effect_speed()

uint8_t MatrixLamp::get_effect_speed()
{
  return this->get_speed_for_effect(currentMode);
} // get_effect_speed()

// Set / Get scale for current effect
void MatrixLamp::set_effect_scale(int32_t value)
{
  this->set_scale_for_effect(currentMode, value);
} // set_effect_scale()

uint8_t MatrixLamp::get_effect_scale()
{
  return this->get_scale_for_effect(currentMode);
} // get_effect_scale()

// Reset brightness, speed, scale to default for current effect
void MatrixLamp::reset_effect_settings()
{
  if (currentMode >= MODE_AMOUNT) {
    return;
  }
  
  modes[currentMode].Brightness = pgm_read_byte(&defaultSettings[currentMode][0]);
  modes[currentMode].Speed      = pgm_read_byte(&defaultSettings[currentMode][1]);
  modes[currentMode].Scale      = pgm_read_byte(&defaultSettings[currentMode][2]);
}

#ifndef ORIENTATION
bool MatrixLamp::set_matrix_orientation(uint8_t orientation)
{
  if (orientation >= 1 && orientation <= 8)
  {
    ORIENTATION = orientation - 1;
    return true;
  }
  return false;
}
#endif // #ifndef ORIENTATION

#ifndef MATRIX_TYPE
bool MatrixLamp::set_matrix_type(uint8_t type)
{
  if (type >= 1 && type <= 2)
  {
    MATRIX_TYPE = type - 1;
    return true;
  }
  return false;
}
#endif // #ifndef MATRIX_TYPE

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
// Get random settings
bool MatrixLamp::get_random_settings()
{
  return random_settings;
}

// Set random settings
void MatrixLamp::set_random_settings(bool b)
{
  if (!b)
  {
    restoreSettings();
  }

  loadingFlag = true;

  random_settings = b;
  selectedSettings = b;
}
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

void MatrixLamp::ShowFrame(uint8_t CurrentMode, esphome::Color current_color, light::AddressableLight *p_it)
{
  InitLeds(p_it->size());

#if defined(MATRIX_LAMP_TRIGGERS)
  bool trigger = false;
#endif
  if (currentMode != CurrentMode)
  {
    currentMode = CurrentMode;
    loadingFlag = true;
#if defined(MATRIX_LAMP_TRIGGERS)
    trigger = true;
#endif

#ifdef DEF_SCANNER
    if (currentMode == EFF_SCANNER) {
      this->set_scale_from_color_for_effect(currentMode, current_color);
    }
#endif

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    selectedSettings = random_settings;
    if (!random_settings)
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    {
      if (this->intensity) {
        auto intensity = this->intensity->make_call();
        intensity.set_value(modes[currentMode].Brightness);
        intensity.perform();
      }
      if (this->speed) {
        auto speed = this->speed->make_call();
        speed.set_value(modes[currentMode].Speed);
        speed.perform();
      }
      if (this->scale) {
        auto scale = this->scale->make_call();
        scale.set_value(modes[currentMode].Scale);
        scale.perform();
      }
    }
  }

  effectsTick();

#if defined(MATRIX_LAMP_TRIGGERS)
  if (trigger) {
    trigger = false;
    for (auto *t : on_effect_start_triggers_) {
      t->process((uint8_t)currentMode, (uint8_t)modes[currentMode].Brightness, (uint8_t)modes[currentMode].Speed, (uint8_t)modes[currentMode].Scale);
    }
 }
#endif

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
  if (random_settings)
  {
    if (this->intensity && (modes[currentMode].Brightness != (int)this->intensity->state))
    {
      auto intensity = this->intensity->make_call();
      intensity.set_value(modes[currentMode].Brightness);
      intensity.perform();
    }
    if (this->speed && (modes[currentMode].Speed != (int)this->speed->state))
    {
      auto speed = this->speed->make_call();
      speed.set_value(modes[currentMode].Speed);
      speed.perform();
    }
    if (this->scale && (modes[currentMode].Scale != (int)this->scale->state))
    {
      auto scale = this->scale->make_call();
      scale.set_value(modes[currentMode].Scale);
      scale.perform();
    }
  }
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

  for (uint32_t i = 0; i < p_it->size(); i++)
  {
    (*p_it)[i] = Color(leds[i].r, leds[i].g, leds[i].b);
  }

  #if defined delay
  delay(1);
  #else
  esphome::delay(1);
  #endif

  if (this->intensity && (modes[currentMode].Brightness != (int)this->intensity->state))
  {
    modes[currentMode].Brightness = (int)this->intensity->state;
    loadingFlag = true; // без перезапуска эффекта ничего и не увидишь
  }
  if (this->speed && (modes[currentMode].Speed != (int)this->speed->state))
  {
    modes[currentMode].Speed = (int)this->speed->state;
    loadingFlag = true; // без перезапуска эффекта ничего и не увидишь
  }
  if (this->speed && (modes[currentMode].Scale != (int)this->scale->state))
  {
    modes[currentMode].Scale = (int)this->scale->state;
    loadingFlag = true; // без перезапуска эффекта ничего и не увидишь
  }
}

#if defined(MATRIX_LAMP_USE_DISPLAY)
uint8_t MatrixLamp::find_icon(std::string name)
{
  for (uint8_t i = 0; i < this->icon_count; i++)
  {
    if (strcmp(this->icons[i]->name.c_str(), name.c_str()) == 0)
    {
      ESP_LOGD(TAG, "Icon: %s found id: %d", name.c_str(), i);
      return i;
    }
  }
  ESP_LOGW(TAG, "Icon: %s not found", name.c_str());
  return MAXICONS;
}

void MatrixLamp::show_icon(std::string iconname)
{
  this->hide_icon();
  if (iconname == "clean_out")
  {
    return;
  }

  uint8_t icon = this->find_icon(iconname);
  if (icon == MAXICONS)
  {
    ESP_LOGD(TAG, "Icon %d/%s not found => skip!", this->current_icon, iconname.c_str());
    return;
  }
  this->show_icon_by_index(icon);    
}

void MatrixLamp::show_icon_by_index(int32_t icon)
{
  if (icon < this->icon_count)
  {
    this->current_icon = icon;
    this->icons[icon]->set_frame(0);
    this->last_anim_time = millis();
    this->display->set_enabled(true);
    return;
  }
  this->hide_icon();
}

#ifdef MATRIX_LAMP_BITMAP_MODE
void MatrixLamp::show_bitmap(std::string bitmap)
{
  if (this->bitmap == NULL)
  {
    this->bitmap = new Color[256];
  }

  JsonDocument doc;
  deserializeJson(doc, bitmap);
  JsonArray array = doc.as<JsonArray>();
  // extract the values
  uint16_t i = 0;
  for (JsonVariant v : array)
  {
    uint16_t buf = v.as<int>();

    unsigned char b = (((buf) & 0x001F) << 3);
    unsigned char g = (((buf) & 0x07E0) >> 3); // Fixed: shift >> 5 and << 2
    unsigned char r = (((buf) & 0xF800) >> 8); // shift >> 11 and << 3
    Color c = Color(r, g, b);

    this->bitmap[i++] = c;
  }
  this->current_icon = BITMAPICON;
  this->display->set_enabled(true);
}
#endif

void MatrixLamp::hide_icon()
{
  this->current_icon = MAXICONS;
  this->display->set_enabled(false);
#ifdef MATRIX_LAMP_BITMAP_MODE
  if (this->bitmap != NULL)
  {
    delete[] this->bitmap;
    this->bitmap = nullptr;
  }
#endif
}

void MatrixLamp::set_brightness(int32_t value)
{
  if (value < 256)
  {
    this->target_brightness = value;
    float br = (float)value / (float)255;
    ESP_LOGI(TAG, "Display: set_brightness %d => %.2f %%", value, 100 * br);
  }
}

void MatrixLamp::update_screen()
{
  if (this->current_icon < this->icon_count)
  {
    if (millis() - this->last_anim_time >= this->icons[this->current_icon]->frame_duration)
    {
      this->icons[this->current_icon]->next_frame();
      this->last_anim_time = millis();
    }
  }
}

void MatrixLamp::Display()
{
  if (this->brightness != this->target_brightness)
  {
    this->brightness = this->brightness + (this->target_brightness < this->brightness ? -1 : 1);
    float br = (float)this->brightness / (float)255;
    this->display->get_light()->set_correction(br, br, br);
  }

  if (this->current_icon == MAXICONS)
  {
    return;
  }

#ifdef MATRIX_LAMP_BITMAP_MODE
  if (this->current_icon == BITMAPICON)
  {
    if (this->bitmap != NULL)
    {
      for (uint8_t x = 0; x < 16; x++)
      {
        for (uint8_t y = 0; y < 16; y++)
        {
          this->display->draw_pixel_at(x, y, this->bitmap[x + y * 16]);
        }
      }
    }
  }
  else
  {
#endif
    this->display->image(4, 4, this->icons[this->current_icon]);
#ifdef MATRIX_LAMP_BITMAP_MODE
  }
#endif
  this->update_screen();
}
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

#if defined(MATRIX_LAMP_TRIGGERS)
void MatrixLampEffectStartTrigger::process(uint8_t effect, uint8_t intensity, uint8_t speed, uint8_t scale)
{
  this->trigger(effect, intensity, speed, scale);
}
#endif

}  // namespace matrix_lamp
}  // namespace esphome
