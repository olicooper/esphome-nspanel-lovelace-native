#ifdef USE_NSPANEL_TFT_UPLOAD
#ifdef USE_ESP_IDF

// Adapted from: https://github.com/esphome/esphome/blob/5b6b7c0d15098f7477bae68329fe76a1d8993cf5/esphome/components/nextion/nextion_upload_idf.cpp
// See:
//  - https://nextion.ca/documentation/nextion-upload-protocol-v1-1/
//  - https://nextion.ca/documentation/nextion-upload-protocol-v1-0/
//  - https://github.com/itead/ITEADLIB_Arduino_Nextion/blob/master/NexUpload.cpp
//  - https://github.com/MMMZZZZ/Nexus/blob/master/Nexus.py
//  - https://nextion.tech/instruction-set/

#include "nspanel_lovelace.h"

#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <cinttypes>

#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"

namespace esphome {
namespace nspanel_lovelace {
static const char *const TAG = "nspanel_lovelace_upload";

// esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
//   switch (evt->event_id) {
//   case HTTP_EVENT_ERROR:
//     ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
//     break;
//   case HTTP_EVENT_ON_CONNECTED:
//     ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
//     break;
//   case HTTP_EVENT_HEADER_SENT:
//     ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
//     break;
//   case HTTP_EVENT_ON_HEADER:
//     ESP_LOGD(
//         TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
//         evt->header_value);
//     break;
//   case HTTP_EVENT_ON_DATA:
//     ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//     break;
//   case HTTP_EVENT_ON_FINISH:
//     ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
//     break;
//   case HTTP_EVENT_DISCONNECTED:
//     ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
//     break;
//   // case HTTP_EVENT_REDIRECT:
//   //   ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
//   //   break;
//   }
//   return ESP_OK;
// }

int NSPanelLovelace::upload_by_chunks_(esp_http_client_handle_t http_client, uint32_t &range_start) {
  uint32_t range_size = this->tft_size_ - range_start;
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());
  uint32_t range_end = ((this->upload_first_chunk_sent_ || this->tft_size_ < 4096) ? this->tft_size_ : 4096) - 1;
  ESP_LOGD(TAG, "Range start: %" PRIu32, range_start);
  if (range_size <= 0 || range_end <= range_start) {
    ESP_LOGD(TAG, "Range end: %" PRIu32, range_end);
    ESP_LOGD(TAG, "Range size: %" PRIu32, range_size);
    ESP_LOGE(TAG, "Invalid range");
    return -1;
  }

  ESP_LOGD(TAG, "Opening HTTP connetion");
  esp_err_t err;
  if ((err = esp_http_client_open(http_client, 0)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    return -1;
  }

  ESP_LOGD(TAG, "Fetch content length");
  const int chunk_size = esp_http_client_fetch_headers(http_client);
  ESP_LOGD(TAG, "content_length = %d", chunk_size);
  if (chunk_size <= 0) {
    ESP_LOGE(TAG, "Failed to get chunk's content length: %d", chunk_size);
    return -1;
  }

  // Allocate the buffer dynamically
  ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
  uint8_t *buffer = allocator.allocate(4096);
  if (!buffer) {
    ESP_LOGE(TAG, "Failed to allocate upload buffer");
    return -1;
  }

  std::string recv_string;
  while (true) {
    App.feed_wdt();
    yield();
    const uint16_t buffer_size =
        this->content_length_ < 4096 ? this->content_length_ : 4096;  // Limits buffer to the remaining data
    ESP_LOGV(TAG, "Fetching %" PRIu16 " bytes from HTTP", buffer_size);
    uint16_t read_len = 0;
    int partial_read_len = 0;
    uint8_t retries = 0;
    // Attempt to read the chunk with retries.
    while (retries < 5 && read_len < buffer_size) {
      partial_read_len =
          esp_http_client_read(http_client, reinterpret_cast<char *>(buffer) + read_len, buffer_size - read_len);
      if (partial_read_len > 0) {
        read_len += partial_read_len;  // Accumulate the total read length.
        ESP_LOGV(TAG, "Read data rlen %" PRIu16 ", b 0x%" PRIx8, read_len, buffer[0]);
        // Reset retries on successful read.
        retries = 0;
      } else {
        ESP_LOGW(TAG, "Failed to read data err %" PRIi16 ", rl %" PRIu16, partial_read_len, read_len);
        // If no data was read, increment retries.
        retries++;
        vTaskDelay(pdMS_TO_TICKS(2));  // NOLINT
      }
      App.feed_wdt();  // Feed the watchdog timer.
    }
    if (read_len != buffer_size) {
      // Did not receive the full package within the timeout period
      ESP_LOGE(TAG, "Failed to read full package, received only %" PRIu16 " of %" PRIu16 " bytes, retries %" PRIi16 ", errno %" PRIi16 ", done %" PRIu8, read_len,
               buffer_size, retries, esp_http_client_get_errno(http_client), esp_http_client_is_complete_data_received(http_client));
      // Deallocate buffer
      allocator.deallocate(buffer, 4096);
      buffer = nullptr;
      return -1;
    }
    ESP_LOGV(TAG, "%d bytes fetched, writing it to UART", read_len);
    if (read_len > 0) {
      this->write_array(buffer, buffer_size);
      App.feed_wdt();
      this->recv_ret_string_(recv_string, this->upload_first_chunk_sent_ ? 500 : 5000, true);
      this->content_length_ -= read_len;
      const float upload_percentage = 100.0f * (this->tft_size_ - this->content_length_) / this->tft_size_;
#ifdef USE_PSRAM
      ESP_LOGD(TAG,
          "Uploaded %0.2f%%, remaining %" PRIu32 " bytes, free heap: %zu (DRAM) + %zu (PSRAM) bytes, lblk %zu",
          upload_percentage, this->content_length_,
          heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
          heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
          heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
#else
      ESP_LOGD(TAG,
          "Uploaded %0.2f%%, remaining %" PRIu32 " bytes, free heap: %" PRIu32 " bytes, lblk %zu",
          upload_percentage, this->content_length_,
          esp_get_free_heap_size(MALLOC_CAP_DEFAULT),
          heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
#endif
      this->upload_first_chunk_sent_ = true;
      if (recv_string[0] == 0x08 && recv_string.size() == 5) {  // handle partial upload request
        ESP_LOGD(TAG, "recv_string [%s]",
                 format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_string.data()), recv_string.size()).c_str());
        uint32_t result = 0;
        for (int j = 0; j < 4; ++j) {
          result += static_cast<uint8_t>(recv_string[j + 1]) << (8 * j);
        }
        if (result > 0) {
          ESP_LOGI(TAG, "Nextion reported new range %" PRIu32, result);
          this->content_length_ = this->tft_size_ - result;
          range_start = result;
          
          // Because the TFT requested a new upload start point, close the HTTP connection
          // and start a new HTTP request with the updated Range header.
          esp_http_client_close(http_client);
          char range_header[32];
          sprintf(range_header, "bytes=%" PRIu32 "-%" PRIu32, range_start, this->tft_size_);
          ESP_LOGD(TAG, "Requesting range: %s", range_header);
          esp_http_client_set_header(http_client, "Range", range_header);
        } else {
          range_start = range_end + 1;
        }
        // Deallocate buffer
        allocator.deallocate(buffer, 4096);
        buffer = nullptr;
        return range_end + 1;
      } else if (recv_string[0] != 0x05 && recv_string[0] != 0x08) {  // 0x05 == "ok"
        ESP_LOGE(TAG, "Invalid response from Nextion: [%s]",
                 format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_string.data()), recv_string.size()).c_str());
        // Deallocate buffer
        allocator.deallocate(buffer, 4096);
        buffer = nullptr;
        return -1;
      }

      recv_string.clear();
    } else if (read_len == 0) {
      ESP_LOGD(TAG, "End of HTTP response reached");
      break;  // Exit the loop if there is no more data to read
    } else {
      ESP_LOGE(TAG, "Failed to read from HTTP client, error code: %" PRIu16, read_len);
      break;  // Exit the loop on error
    }
  }
  range_start = range_end + 1;
  // Deallocate buffer
  allocator.deallocate(buffer, 4096);
  buffer = nullptr;
  return range_end + 1;
}

bool NSPanelLovelace::upload_tft(const std::string &url /*, uint32_t baud_rate*/) {
  ESP_LOGI(TAG, "Nextion TFT upload requested");
  ESP_LOGD(TAG, "URL: %s", url.c_str());

  if (this->is_updating_) {
    ESP_LOGW(TAG, "Currently uploading");
    return false;
  }

  if (!network::is_connected()) {
    ESP_LOGE(TAG, "Network is not connected");
    return false;
  }

  this->is_updating_ = true;

  std::string recv_res;
  if (Configuration::get_model() != nspanel_model_t::unknown) {
    this->send_nextion_command_("DRAKJHSUYDGBNCJHGJKSHBDN");
    this->send_nextion_command_("recmod=0");
    this->send_nextion_command_("recmod=0");
    this->send_nextion_command_("connect");
    ESP_LOGI(TAG, "TFT (v%" PRIu16 ") connected at %" PRIu32 " baud",
      Configuration::get_version(), this->parent_->get_baud_rate());
  }
  // Figure out the current TFT baud rate if using the stock TFT firmware
  else {
    uint32_t baudrates[7] = {115200,19200,9600,57600,38400,4800,2400};
    bool found = false;
    for (uint8_t i = 0; i < (sizeof(baudrates) / sizeof(uint32_t)); i++) {
      this->parent_->set_baud_rate(baudrates[i]);
      this->parent_->load_settings();
      uint8_t d;
      while (this->available()) {
        App.feed_wdt();
        this->read_byte(&d);
      };
      this->send_nextion_command_("DRAKJHSUYDGBNCJHGJKSHBDN");
      this->send_nextion_command_("connect");
      this->send_nextion_command_("\u00ff\u00ffconnect");
      this->recv_ret_string_(recv_res, 100, false);
      if (recv_res.find("comok") != std::string::npos) {
        found = true;
        ESP_LOGI(TAG, "TFT connected at %" PRIu32 " baud: '%s'",
          this->parent_->get_baud_rate(), recv_res.c_str());
        this->send_nextion_command_(""); // initial empty command recommended by nextion
        break;
      }
      ESP_LOGD(TAG, "TFT response: '%s'",
        format_hex_pretty(
          reinterpret_cast<const uint8_t *>(recv_res.data()),
          recv_res.size()).c_str());
      recv_res.clear();
      vTaskDelay(pdMS_TO_TICKS((1000000 / baudrates[i]) + 30));  // NOLINT
    }
    if (!found) {
      // Sometimes the model is not known even though the panel is running
      // custom FW and there is no way to find this information out,
      // so we proceed with the FW upload anyway to account for this.
      ESP_LOGW(TAG,
        "Failed to establish TFT connection, reverting to %" PRIu32
        " baud and attempting update anyway",
        this->default_baud_rate_);
      this->parent_->set_baud_rate(this->default_baud_rate_);
      this->parent_->load_settings();
      // return this->upload_end_(false);
    }
  }

  // Define the configuration for the HTTP client
  ESP_LOGD(TAG, "Initializing HTTP client");
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  // config.host = nullptr;
  config.cert_pem = nullptr;
  config.method = HTTP_METHOD_HEAD;
  config.timeout_ms = 15000;
  config.disable_auto_redirect = false;
  config.max_redirection_count = 10;
  config.is_async = false;
  config.keep_alive_enable = true;
  // config.event_handler = _http_event_handler;

  // Initialize the HTTP client with the configuration
  esp_http_client_handle_t http_client = esp_http_client_init(&config);
  if (!http_client) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return this->upload_end_(false);
  }
  
  esp_err_t err;
  // Perform the HTTP request
  ESP_LOGD(TAG, "Check if the client could connect");
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());
  err = esp_http_client_perform(http_client);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(http_client);
    return this->upload_end_(false);
  }

  // Check the HTTP Status Code
  ESP_LOGD(TAG, "Check the HTTP Status Code");
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());
  int status_code = esp_http_client_get_status_code(http_client);
  if (status_code != 200 && status_code != 206) {
    return this->upload_end_(false);
  }

  this->tft_size_ = esp_http_client_get_content_length(http_client);

  ESP_LOGD(TAG, "TFT file size: %zu bytes", this->tft_size_);
  if (this->tft_size_ < 4096 || this->tft_size_ > 134217728) {
    ESP_LOGE(TAG, "File size check failed.");
    ESP_LOGD(TAG, "Close HTTP connection");
    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);
    ESP_LOGD(TAG, "Connection closed");
    return this->upload_end_(false);
  } else {
    ESP_LOGD(TAG, "File size check passed. Proceeding...");
  }
  this->content_length_ = this->tft_size_;

  // The Nextion will ignore the upload command if it is sleeping
  ESP_LOGD(TAG, "Wake-up Nextion");
  if (Configuration::get_model() != nspanel_model_t::unknown) {
    // This command targets nspanel firmware
    this->send_nextion_command_("dimmode~100~100");
  } else {
    // These commands target the stock firmware
    this->send_nextion_command_("dims=100");
    this->send_nextion_command_("sleep=0");
  }
  vTaskDelay(pdMS_TO_TICKS(250));  // NOLINT

  App.feed_wdt();
  // Tells the Nextion the content length of the tft file and baud rate it will be sent at
  // Once the Nextion accepts the command it will wait until the file is successfully uploaded
  // If it fails for any reason a power cycle of the display will be needed
  char command[32] = "whmi-wris";
  auto sz = snprintf(
    &command[9],
    sizeof(command) - 10, " %" PRIu32 ",%" PRIu32 ",1",
    this->content_length_, /*baud_rate*/ this->update_baud_rate_);

  // Clear serial receive buffer
  ESP_LOGD(TAG, "Clear serial receive buffer");
  uint8_t d;
  while (this->available()) {
    App.feed_wdt();
    this->read_byte(&d);
  };
  vTaskDelay(pdMS_TO_TICKS(250));  // NOLINT
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());

  this->send_nextion_command_(command);

  if (this->parent_->get_baud_rate() != this->update_baud_rate_) {
    this->parent_->set_baud_rate(this->update_baud_rate_);
    this->parent_->load_settings();
  }

  ESP_LOGD(TAG, "Waiting for upgrade response");
  recv_res.clear();
  this->recv_ret_string_(recv_res, 5000, true);  // This can take some time to return

  // The Nextion display will, if it's ready to accept data, send a 0x05 byte.
  ESP_LOGD(TAG, "Upgrade response is [%s] - %zu byte(s)",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(recv_res.data()), recv_res.size()).c_str(),
           recv_res.length());
  ESP_LOGD(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());

  if (recv_res.find(0x05) != std::string::npos) {
    ESP_LOGD(TAG, "Preparation for TFT upload done");
  } else {
    ESP_LOGE(TAG, "Preparation for TFT upload failed %d \"%s\"", recv_res[0], recv_res.c_str());
    ESP_LOGD(TAG, "Close HTTP connection");
    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);
    ESP_LOGD(TAG, "Connection closed");
    return this->upload_end_(false);
  }

  ESP_LOGD(TAG, "Change the method to GET before starting the download");
  esp_err_t set_method_result = esp_http_client_set_method(http_client, HTTP_METHOD_GET);
  if (set_method_result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set HTTP method to GET: %s", esp_err_to_name(set_method_result));
    return this->upload_end_(false);
  }

  ESP_LOGI(TAG, "Uploading TFT to Nextion:");
  ESP_LOGI(TAG, "  URL: %s", url.c_str());
  ESP_LOGI(TAG, "  File size: %" PRIu32 " bytes", this->content_length_);
  ESP_LOGI(TAG, "  Free heap: %" PRIu32, esp_get_free_heap_size());

  // Proceed with the content download as before

  ESP_LOGD(TAG, "Starting transfer by chunks loop");

  uint32_t position = 0;
  while (this->content_length_ > 0) {
    int upload_result = upload_by_chunks_(http_client, position);
    if (upload_result < 0) {
      ESP_LOGE(TAG, "Error uploading TFT to Nextion!");
      ESP_LOGD(TAG, "Close HTTP connection");
      esp_http_client_close(http_client);
      esp_http_client_cleanup(http_client);
      ESP_LOGD(TAG, "Connection closed");
      return this->upload_end_(false);
    }
    App.feed_wdt();
    yield();
    ESP_LOGD(TAG, "Free heap: %" PRIu32 ", Bytes left: %" PRIu32, esp_get_free_heap_size(), this->content_length_);
  }

  ESP_LOGD(TAG, "Successfully uploaded TFT to Nextion!");

  ESP_LOGD(TAG, "Close HTTP connection");
  esp_http_client_close(http_client);
  esp_http_client_cleanup(http_client);
  ESP_LOGD(TAG, "Connection closed");
  return this->upload_end_(true);
}

bool NSPanelLovelace::upload_end_(bool successful) {
  if (successful)
    ESP_LOGI(TAG, "Nextion TFT upload finished");
  else
    ESP_LOGW(TAG, "Nextion TFT upload failed");
  this->is_updating_ = false;

  // Make sure we are running with the configured baud rate
  // so we can communicate normally with the TFT again
  if (this->parent_->get_baud_rate() != this->default_baud_rate_) {
    this->parent_->set_baud_rate(this->default_baud_rate_);
    this->parent_->load_settings();
  }
  this->soft_reset_display();
  // todo: Why do we need to reset the ESP after a TFT update?
  //       The TFT should send us the startup command when reset
  // if (successful) {
  //   ESP_LOGD(TAG, "Restarting ESPHome");
  //   delay(1500);  // NOLINT
  //   App.safe_reboot();
  // } else {
  //   ESP_LOGE(TAG, "Nextion TFT upload failed");
  // }
  return successful;
}

}  // namespace nspanel_lovelace
}  // namespace esphome

#endif // USE_ESP_IDF
#endif // USE_NSPANEL_TFT_UPLOAD