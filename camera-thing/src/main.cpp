#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "driver/i2s.h"

// WiFi credentials
const char* ssid = "Poly pixel";
const char* password = "H0lypassword";

// ===================
// XIAO ESP32S3 Sense Camera Pins
// ===================
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

// ===================
// XIAO ESP32S3 Sense PDM Microphone Pins
// ===================
#define I2S_PDM_CLK_PIN  42
#define I2S_PDM_DATA_PIN 41

// ===================
// Stream Configuration
// ===================
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t audio_httpd = NULL;
static bool i2s_initialized = false;

// ===================
// Running Average Filter
// ===================
typedef struct {
    size_t size;
    size_t index;
    size_t count;
    int sum;
    int* values;
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

// ===================
// Video Stream Handler
// ===================
static esp_err_t stream_handler(httpd_req_t* req) {
    camera_fb_t* fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t* _jpg_buf = NULL;
    char part_buf[128];

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
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
            if (fb->format != PIXFORMAT_JPEG) {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted) {
                    Serial.println("JPEG compression failed");
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
            size_t hlen = snprintf(part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
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
            Serial.println("Send frame failed");
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;

        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        Serial.printf("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)\n",
            (uint32_t)_jpg_buf_len, (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
            avg_frame_time, 1000.0 / avg_frame_time);
    }

    return res;
}

// ===================
// Audio Streaming
// ===================
#define AUDIO_CHUNK_TIME    0.1f     // 100ms chunks
#define AUDIO_SAMPLE_RATE   16000U
#define AUDIO_SAMPLE_BITS   16
#define AUDIO_HEADER_SIZE   44
#define AUDIO_GAIN_SHIFT    2
#define AUDIO_CHUNK_SIZE    ((uint32_t)(AUDIO_SAMPLE_RATE * (AUDIO_SAMPLE_BITS / 8) * AUDIO_CHUNK_TIME))

static bool audio_streaming = false;
static uint8_t *audio_buffer = NULL;  // Pre-allocated buffer

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate) {
    uint32_t file_size = wav_size + AUDIO_HEADER_SIZE - 8;
    uint32_t byte_rate = AUDIO_SAMPLE_RATE * AUDIO_SAMPLE_BITS / 8;
    const uint8_t set_wav_header[] = {
        'R', 'I', 'F', 'F',
        (uint8_t)file_size, (uint8_t)(file_size >> 8), (uint8_t)(file_size >> 16), (uint8_t)(file_size >> 24),
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        0x10, 0x00, 0x00, 0x00,
        0x01, 0x00,
        0x01, 0x00,
        (uint8_t)sample_rate, (uint8_t)(sample_rate >> 8), (uint8_t)(sample_rate >> 16), (uint8_t)(sample_rate >> 24),
        (uint8_t)byte_rate, (uint8_t)(byte_rate >> 8), (uint8_t)(byte_rate >> 16), (uint8_t)(byte_rate >> 24),
        0x02, 0x00,
        0x10, 0x00,
        'd', 'a', 't', 'a',
        (uint8_t)wav_size, (uint8_t)(wav_size >> 8), (uint8_t)(wav_size >> 16), (uint8_t)(wav_size >> 24),
    };
    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

esp_err_t record_wav_chunk(uint8_t **out_buf, size_t *out_len) {
    uint32_t record_size = (uint32_t)(AUDIO_SAMPLE_RATE * (AUDIO_SAMPLE_BITS / 8) * AUDIO_CHUNK_TIME);
    uint32_t total_size = AUDIO_HEADER_SIZE + record_size;

    uint8_t *buf = (uint8_t *)ps_malloc(total_size);  // Use PSRAM if available
    if (!buf) {
        buf = (uint8_t *)malloc(total_size);
        if (!buf) {
            Serial.println("Failed to allocate audio buffer");
            return ESP_ERR_NO_MEM;
        }
    }

    uint8_t *audio_ptr = buf + AUDIO_HEADER_SIZE;
    size_t sample_size = 0;

    // Use timeout instead of blocking forever (1 second timeout)
    esp_err_t err = i2s_read(I2S_NUM_0, audio_ptr, record_size, &sample_size, pdMS_TO_TICKS(1000));

    if (err != ESP_OK || sample_size == 0) {
        Serial.printf("I2S read failed: err=%d, size=%d\n", err, sample_size);
        free(buf);
        return ESP_FAIL;
    }

    // Apply gain
    for (uint32_t i = 0; i < sample_size; i += AUDIO_SAMPLE_BITS / 8) {
        (*(uint16_t *)(audio_ptr + i)) <<= AUDIO_GAIN_SHIFT;
    }

    // Generate header with actual recorded size
    generate_wav_header(buf, sample_size, AUDIO_SAMPLE_RATE);

    *out_buf = buf;
    *out_len = AUDIO_HEADER_SIZE + sample_size;
    return ESP_OK;
}

static esp_err_t audio_stream_handler(httpd_req_t *req) {
    Serial.println(">>> Audio handler called");
    Serial.flush();

    if (!i2s_initialized) {
        Serial.println("Audio requested but I2S not initialized");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Audio not available");
        return ESP_FAIL;
    }

    // Allocate buffer if not already done
    Serial.printf(">>> i2s_initialized=%d, audio_buffer=%p\n", i2s_initialized, audio_buffer);
    Serial.flush();

    if (!audio_buffer) {
        Serial.println(">>> Allocating audio buffer...");
        Serial.flush();
        audio_buffer = (uint8_t *)ps_malloc(AUDIO_CHUNK_SIZE);
        if (!audio_buffer) {
            audio_buffer = (uint8_t *)malloc(AUDIO_CHUNK_SIZE);
        }
        if (!audio_buffer) {
            Serial.println("Failed to allocate audio buffer");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory error");
            return ESP_FAIL;
        }
        Serial.println(">>> Buffer allocated OK");
        Serial.flush();
    }

    esp_err_t res = ESP_OK;
    audio_streaming = true;

    // Use octet-stream for raw binary PCM data
    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");

    Serial.println("Audio streaming started");

    // Continuous audio streaming loop
    int read_attempts = 0;
    while (audio_streaming) {
        size_t bytes_read = 0;

        // Read audio data - timeout must be longer than chunk time
        esp_err_t err = i2s_read(I2S_NUM_0, audio_buffer, AUDIO_CHUNK_SIZE, &bytes_read, pdMS_TO_TICKS(500));

        read_attempts++;

        // Log any anomalies
        if (err != ESP_OK || bytes_read != AUDIO_CHUNK_SIZE) {
            Serial.printf("I2S issue #%d: err=%d, expected=%d, got=%d\n",
                read_attempts, err, AUDIO_CHUNK_SIZE, bytes_read);
        }

        if (err != ESP_OK || bytes_read == 0) {
            // This shouldn't happen - log and send silence
            memset(audio_buffer, 0, AUDIO_CHUNK_SIZE);
            bytes_read = AUDIO_CHUNK_SIZE;
        }

        // Skip gain for now - test raw audio
        // int16_t *samples = (int16_t *)audio_buffer;
        // for (size_t i = 0; i < bytes_read / 2; i++) {
        //     samples[i] = samples[i] << AUDIO_GAIN_SHIFT;
        // }

        // Send audio chunk
        res = httpd_resp_send_chunk(req, (const char *)audio_buffer, bytes_read);

        if (res != ESP_OK) {
            Serial.println("Client disconnected");
            break;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);
    audio_streaming = false;
    Serial.println("Audio streaming stopped");
    return res;
}

// ===================
// I2S PDM Microphone Init
// ===================
void i2s_init() {
    Serial.println("Starting I2S PDM microphone init...");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 16,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    Serial.println("Installing I2S driver...");
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("I2S driver install failed: %d\n", err);
        return;
    }
    Serial.println("I2S driver installed OK");

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PDM_CLK_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_PDM_DATA_PIN
    };

    Serial.printf("Setting I2S pins (CLK=%d, DATA=%d)...\n", I2S_PDM_CLK_PIN, I2S_PDM_DATA_PIN);
    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("I2S set pin failed: %d\n", err);
        return;
    }

    i2s_initialized = true;
    Serial.println("I2S PDM microphone initialized successfully");
}

// ===================
// Camera Init
// ===================
void camera_init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_VGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    // Check for PSRAM
    if (psramFound()) {
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }

    Serial.println("Camera initialized successfully");
}

// ===================
// Index Page Handler
// ===================
static const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 Stream</title>
    <style>
        body { font-family: Arial; text-align: center; background: #1a1a1a; color: #fff; margin: 20px; }
        img { max-width: 100%; border: 2px solid #444; }
        button { padding: 15px 30px; font-size: 18px; margin: 10px; cursor: pointer;
                 background: #4CAF50; color: white; border: none; border-radius: 5px; }
        button:hover { background: #45a049; }
        button:disabled { background: #666; }
        #status { margin: 10px; padding: 10px; background: #333; border-radius: 5px; }
    </style>
</head>
<body>
    <h1>ESP32-S3 Camera + Audio</h1>
    <div>
        <img id="stream" src="/stream" />
    </div>
    <div>
        <button id="audioBtn" onclick="toggleAudio()">Start Audio</button>
    </div>
    <div id="status">Audio: Stopped</div>

    <script>
        let audioCtx = null;
        let isPlaying = false;
        let nextStartTime = 0;

        async function toggleAudio() {
            const btn = document.getElementById('audioBtn');
            const status = document.getElementById('status');

            if (isPlaying) {
                isPlaying = false;
                if (audioCtx) {
                    audioCtx.close();
                    audioCtx = null;
                }
                btn.textContent = 'Start Audio';
                status.textContent = 'Audio: Stopped';
                return;
            }

            isPlaying = true;
            btn.textContent = 'Stop Audio';
            status.textContent = 'Audio: Connecting...';

            try {
                audioCtx = new (window.AudioContext || window.webkitAudioContext)();
                const LATENCY_BUFFER = 0.1;  // 100ms safety margin
                nextStartTime = audioCtx.currentTime + LATENCY_BUFFER;

                const response = await fetch(window.location.protocol + '//' + window.location.hostname + ':82/audio');
                if (!response.ok) throw new Error('HTTP ' + response.status);

                const reader = response.body.getReader();
                status.textContent = 'Audio: Buffering...';

                let chunks = [];
                let totalBytes = 0;
                let initialBufferFilled = false;
                const INITIAL_BUFFER = 6400;   // 200ms initial buffer
                const CHUNK_THRESHOLD = 3200;  // 100ms chunks after initial

                while (isPlaying) {
                    const { done, value } = await reader.read();
                    if (done) {
                        status.textContent = 'Audio: Stream ended';
                        break;
                    }

                    if (!value || value.length === 0) continue;

                    // Accumulate data
                    chunks.push(value);
                    totalBytes += value.length;

                    // Wait for initial buffer, then process in chunks
                    const threshold = initialBufferFilled ? CHUNK_THRESHOLD : INITIAL_BUFFER;
                    if (totalBytes >= threshold) {
                        initialBufferFilled = true;
                        // Combine chunks
                        const combined = new Uint8Array(totalBytes);
                        let offset = 0;
                        for (const chunk of chunks) {
                            combined.set(chunk, offset);
                            offset += chunk.length;
                        }
                        chunks = [];
                        totalBytes = 0;

                        // Ensure even number of bytes for Int16
                        const len = combined.length - (combined.length % 2);
                        if (len === 0) continue;

                        // Convert Int16 PCM to Float32
                        const dataView = new DataView(combined.buffer, combined.byteOffset, len);
                        const float32 = new Float32Array(len / 2);
                        for (let i = 0; i < float32.length; i++) {
                            float32[i] = dataView.getInt16(i * 2, true) / 32768.0;
                        }

                        // Create and schedule audio buffer
                        const buffer = audioCtx.createBuffer(1, float32.length, 16000);
                        buffer.getChannelData(0).set(float32);

                        const source = audioCtx.createBufferSource();
                        source.buffer = buffer;

                        // Use gain node for fade-in on underrun recovery
                        const gainNode = audioCtx.createGain();
                        source.connect(gainNode);
                        gainNode.connect(audioCtx.destination);

                        // Schedule playback - add margin if we fell behind
                        if (nextStartTime < audioCtx.currentTime) {
                            // Buffer underrun - reset with safety margin and fade in
                            nextStartTime = audioCtx.currentTime + 0.05;
                            gainNode.gain.setValueAtTime(0, nextStartTime);
                            gainNode.gain.linearRampToValueAtTime(1, nextStartTime + 0.05);
                        } else {
                            gainNode.gain.setValueAtTime(1, nextStartTime);
                        }
                        source.start(nextStartTime);
                        nextStartTime += buffer.duration;

                        status.textContent = 'Audio: Playing';
                    }
                }
            } catch (e) {
                console.error('Audio error:', e);
                status.textContent = 'Audio: Error - ' + e.message;
            }

            isPlaying = false;
            btn.textContent = 'Start Audio';
            if (audioCtx) {
                audioCtx.close();
                audioCtx = null;
            }
        }
    </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, index_html, strlen(index_html));
}

// ===================
// Start HTTP Server
// ===================
void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.server_port = 81;
    config.ctrl_port = 32768;

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    httpd_uri_t audio_uri = {
        .uri = "/audio",
        .method = HTTP_GET,
        .handler = audio_stream_handler,
        .user_ctx = NULL
    };

    ra_filter_init(&ra_filter, 20);

    // Start main server on port 81 (index + video stream)
    Serial.printf("Starting stream server on port: %d\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }

    // Start audio server on port 82 (separate to avoid blocking)
    config.server_port = 82;
    config.ctrl_port = 32769;
    Serial.printf("Starting audio server on port: %d\n", config.server_port);
    if (httpd_start(&audio_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(audio_httpd, &audio_uri);
    }
}

// ===================
// Setup & Loop
// ===================
void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    // Initialize camera
    camera_init();

    // Initialize I2S microphone
    i2s_init();

    // Connect to WiFi
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());

    // Start streaming server
    startCameraServer();

    Serial.print("Camera Stream: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":81/stream");

    Serial.print("Audio Stream: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":82/audio");
}

void loop() {
    delay(10000);
}
