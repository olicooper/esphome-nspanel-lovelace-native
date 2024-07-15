#include "config.h"

namespace esphome {
namespace nspanel_lovelace {

Configuration::Configuration() :
    temperature_unit_(temperature_unit_t::celcius),
    model_(nspanel_model_t::eu) { }

Configuration *Configuration::instance() {
  static std::unique_ptr<Configuration> cfg;

  if (cfg == nullptr) cfg.reset(new Configuration());
  return cfg.get();
}

void Configuration::set_temperature_unit(temperature_unit_t unit) {
  Configuration::instance()->temperature_unit_ = unit;
}

temperature_unit_t Configuration::get_temperature_unit() {
  return Configuration::instance()->temperature_unit_;
}

std::string Configuration::get_temperature_unit_str() {
  return Configuration::instance()->temperature_unit_ ==
    temperature_unit_t::fahrenheit ? "°F" : "°C";
}

void Configuration::set_model(nspanel_model_t model) {
  Configuration::instance()->model_ = model;
}

void Configuration::set_model(const std::string &model_str) {
  if (model_str == "us-p")
    Configuration::instance()->model_ = nspanel_model_t::eu;
  else if (model_str == "us-l")
    Configuration::instance()->model_ = nspanel_model_t::us_l;
  else
    Configuration::instance()->model_ =  nspanel_model_t::eu;
}

nspanel_model_t Configuration::get_model() {
  return Configuration::instance()->model_;
}

std::string Configuration::get_model_str() {
  if (Configuration::instance()->model_ == nspanel_model_t::us_p)
    return "us-p";
  if (Configuration::instance()->model_ == nspanel_model_t::us_l)
    return "us-l";
  return "eu";
}

}
}