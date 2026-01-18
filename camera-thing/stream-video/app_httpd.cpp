// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "board_config.h"
#include "esp32-hal-ledc.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "fb_gfx.h"
#include "img_converters.h"
#include "sdkconfig.h"


#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#endif

typedef struct {
    httpd_req_t* req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

typedef struct {
    size_t size;   // number of values used for filtering
    size_t index;  // current value index
    size_t count;  // value count
    int sum;
    int* values;  // array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

static ra_filter_t* ra_filter_init(ra_filter_t* filter, size_t sample_size) {
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int*)malloc(sample_size * sizeof(int));
    if (!filter->values) {
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
static int ra_filter_run(ra_filter_t* filter, int value) {
    if (!filter->values) {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size) {
        filter->count++;
    }
    return filter->sum / filter->count;
}
#endif

static esp_err_t stream_handler(httpd_req_t* req) {
    camera_fb_t* fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t* _jpg_buf = NULL;
    char* part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            log_e("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
            if (fb->format != PIXFORMAT_JPEG) {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted) {
                    log_e("JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK) {
            size_t hlen = snprintf((char*)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char*)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len);
        }
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK) {
            log_e("Send frame failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;

        frame_time /= 1000;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
#endif
        log_i(
            "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)", (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time, avg_frame_time,
            1000.0 / avg_frame_time);
    }

#if defined(LED_GPIO_NUM)
    isStreaming = false;
    enable_led(false);
#endif

    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    httpd_uri_t audio_uri = {
        .uri = "/audio",
        .method = HTTP_GET,
        .handler = audio_stream_handler,
        .user_ctx = NULL};

    ra_filter_init(&ra_filter, 20);

    config.server_port += 1;
    config.ctrl_port += 1;
    log_i("Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &audio_uri);
    }
}

void setupLedFlash() {
#if defined(LED_GPIO_NUM)
    ledcAttach(LED_GPIO_NUM, 5000, 8);
#else
    log_i("LED flash is disabled -> LED_GPIO_NUM undefined");
#endif
}


// Audio streaming
#define AUDIO_CHUNK_TIME    0.5f     
#define AUDIO_SAMPLE_RATE   16000U
#define AUDIO_SAMPLE_BITS   16
#define AUDIO_HEADER_SIZE   44
#define AUDIO_GAIN_SHIFT    2    

static bool audio_streaming = false;

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate);

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
  // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
  uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
  uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
  const uint8_t set_wav_header[] = {
    'R', 'I', 'F', 'F', // ChunkID
    file_size, file_size >> 8, file_size >> 16, file_size >> 24, // ChunkSize
    'W', 'A', 'V', 'E', // Format
    'f', 'm', 't', ' ', // Subchunk1ID
    0x10, 0x00, 0x00, 0x00, // Subchunk1Size (16 for PCM)
    0x01, 0x00, // AudioFormat (1 for PCM)
    0x01, 0x00, // NumChannels (1 channel)
    sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
    byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24, // ByteRate
    0x02, 0x00, // BlockAlign
    0x10, 0x00, // BitsPerSample (16 bits)
    'd', 'a', 't', 'a', // Subchunk2ID
    wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24, // Subchunk2Size
  };
  memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

sp_err_t record_wav_chunk(uint8_t **out_buf, size_t *out_len) {
  uint32_t record_size = (uint32_t)(AUDIO_SAMPLE_RATE * (AUDIO_SAMPLE_BITS / 8) * AUDIO_CHUNK_TIME);
  uint32_t total_size  = AUDIO_HEADER_SIZE + record_size;

  uint8_t *buf = (uint8_t *)ps_malloc(total_size);
  if (!buf) {
    return ESP_ERR_NO_MEM;
  }

  generate_wav_header(buf, record_size, AUDIO_SAMPLE_RATE);

  uint8_t *audio_ptr = buf + AUDIO_HEADER_SIZE;
  size_t   sample_size = 0;

  esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, audio_ptr, record_size, &sample_size, portMAX_DELAY);

  if (sample_size == 0) {
    free(buf);
    return ESP_FAIL;
  }

  for (uint32_t i = 0; i < sample_size; i += AUDIO_SAMPLE_BITS / 8) {
    (*(uint16_t *)(audio_ptr + i)) <<= AUDIO_GAIN_SHIFT;
  }

  *out_buf = buf;
  *out_len = AUDIO_HEADER_SIZE + sample_size;
  return ESP_OK;
}

static esp_err_t audio_stream_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  audio_streaming = true;

  httpd_resp_set_type(req, "audio/wav");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (audio_streaming && res == ESP_OK) {
    uint8_t *wav_buf = NULL;
    size_t   wav_len = 0;

    if (record_wav_chunk(&wav_buf, &wav_len) != ESP_OK) {
      res = ESP_FAIL;
      break;
    }
    res = httpd_resp_send_chunk(req, (const char *)wav_buf, wav_len);
    free(wav_buf);

    if (res != ESP_OK) {
      break;
    }
  }

  if (res == ESP_OK) {
    httpd_resp_send_chunk(req, NULL, 0);
  }

  audio_streaming = false;
  return res;
}
