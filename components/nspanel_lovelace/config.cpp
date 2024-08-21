#include "config.h"

namespace esphome {
namespace nspanel_lovelace {

Configuration::Configuration() :
    temperature_unit_(temperature_unit_t::celcius),
    model_(nspanel_model_t::unknown),
    version_(0) { }

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
    Configuration::instance()->model_ = nspanel_model_t::us_p;
  else if (model_str == "us-l")
    Configuration::instance()->model_ = nspanel_model_t::us_l;
  else if (model_str == "eu")
    Configuration::instance()->model_ =  nspanel_model_t::eu;
  else
    Configuration::instance()->model_ =  nspanel_model_t::unknown;
}

nspanel_model_t Configuration::get_model() {
  return Configuration::instance()->model_;
}

std::string Configuration::get_model_str() {
  if (Configuration::instance()->model_ == nspanel_model_t::us_p)
    return "us-p";
  if (Configuration::instance()->model_ == nspanel_model_t::us_l)
    return "us-l";
  if (Configuration::instance()->model_ == nspanel_model_t::eu)
    return "eu";
  return "unknown";
}

uint16_t Configuration::get_version() {
  return Configuration::instance()->version_;
}

void Configuration::set_version(uint16_t version) {
  Configuration::instance()->version_ = version;
}

}
}