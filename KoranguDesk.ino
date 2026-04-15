#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "time.h"
#include <math.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <WebSocketsClient.h>
#include <WiFiUdp.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ==================================================
// 1. ASSETS & CONFIG
// ==================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH_PIN 7

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- WEATHER ICONS ---
const unsigned char bmp_clear[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0xc0, 0x80, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x00, 0xff, 0xff, 0x00, 0x3e, 0xff, 0xff, 0x7c, 0x3e, 0xff, 0xff, 0x7c, 0x3e, 0xff, 0xff, 0x7c, 0x00, 0xff, 0xff, 0x00, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x06, 0xff, 0xff, 0x60, 0x00, 0xff, 0xff, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x01, 0x0f, 0xf0, 0x80, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char bmp_clouds[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfe, 0x0f, 0xff, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0x80, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char bmp_rain[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 0xf8, 0x00, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0xff, 0x80, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x0c, 0x00, 0x00, 0x60, 0x0c, 0x00, 0x00, 0xe0, 0x1c, 0x00, 0x00, 0xc0, 0x18, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03, 0x80, 0x70, 0x00, 0x03, 0x00, 0x60, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char mini_sun[] PROGMEM = {0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x10, 0x08, 0x04, 0x20, 0x03, 0xc0, 0x27, 0xe4, 0x07, 0xe0, 0x07, 0xe0, 0x27, 0xe4, 0x03, 0xc0, 0x04, 0x20, 0x10, 0x08, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00};
const unsigned char mini_cloud[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfe, 0x1f, 0xfc, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char mini_rain[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc, 0x7f, 0xfe, 0x3f, 0xfe, 0x1f, 0xfc, 0x00, 0x00, 0x44, 0x44, 0x22, 0x22, 0x11, 0x11};
const unsigned char bmp_tiny_drop[] PROGMEM = {0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x7c, 0x38, 0x00};

// --- EMOTION PARTICLES (16x16) ---
const unsigned char bmp_heart[] PROGMEM = {0x00, 0x00, 0x0c, 0x60, 0x1e, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char bmp_zzz[] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7e, 0x00, 0x00, 0x3c, 0x00, 0x0c, 0x00, 0x18, 0x00, 0x30, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char bmp_anger[] PROGMEM = {0x00, 0x00, 0x11, 0x10, 0x2a, 0x90, 0x44, 0x40, 0x80, 0x20, 0x80, 0x20, 0x44, 0x40, 0x2a, 0x90, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// --- KORANGU RUN SPRITES ---
const unsigned char sprite_korangu_run_1[] PROGMEM = {
    0x3c, 0x7e, 0xff, 0xdb, 0xff, 0x3c, 0x66, 0xc3};
const unsigned char sprite_korangu_run_2[] PROGMEM = {
    0x3c, 0x7e, 0xff, 0xdb, 0xff, 0x3c, 0xc3, 0x66};
const unsigned char sprite_korangu_jump[] PROGMEM = {
    0x3c, 0x7e, 0xff, 0xdb, 0xff, 0x18, 0x3c, 0x18};
const unsigned char sprite_cactus[] PROGMEM = {
    0x18, 0x18, 0x3c, 0x3c, 0x18, 0x7e, 0x18, 0x18, 0x3c, 0x3c, 0x18, 0x24};

// ==================================================
// --- LYRICS SIDE MONKEY SPRITES (16x16) WITH TAIL ---
// ==================================================

// Frame 1: Standing tall, mouth open singing 'O', tail curled HIGH LEFT
const unsigned char sprite_lyric_monkey_1[] PROGMEM = {
    0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf8, 0x39, 0x9c,
    0x3f, 0xfc, 0x0f, 0xf0, 0x63, 0xc0, 0x9f, 0xf8,
    0x8f, 0xf8, 0x93, 0xc0, 0x63, 0xc0, 0x3e, 0x70,
    0x06, 0x60, 0x06, 0x60, 0x0f, 0xf0, 0x00, 0x00};

// Frame 2: Bobbing down, smiling, tail swishing LOW LEFT
const unsigned char sprite_lyric_monkey_2[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf8,
    0x39, 0x9c, 0x3f, 0xfc, 0x0f, 0xf0, 0x03, 0xc0,
    0x1f, 0xf8, 0x3f, 0xf8, 0x63, 0xc0, 0x93, 0xc0,
    0x8f, 0xe0, 0x4f, 0xe0, 0x3f, 0xf0, 0x00, 0x00};

// Frame 3: Standing tall, mouth open singing 'O', tail curled HIGH RIGHT
const unsigned char sprite_lyric_monkey_3[] PROGMEM = {
    0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf8, 0x39, 0x9c,
    0x3f, 0xfc, 0x0f, 0xf0, 0x03, 0xc6, 0x1f, 0xf9,
    0x1f, 0xf1, 0x03, 0xc9, 0x03, 0xc6, 0x0e, 0x7c,
    0x06, 0x60, 0x06, 0x60, 0x0f, 0xf0, 0x00, 0x00};

// Frame 4: Bobbing down, smiling, tail swishing LOW RIGHT
const unsigned char sprite_lyric_monkey_4[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf8,
    0x39, 0x9c, 0x3f, 0xfc, 0x0f, 0xf0, 0x03, 0xc0,
    0x1f, 0xf8, 0x1f, 0xfc, 0x03, 0xc6, 0x03, 0xc9,
    0x07, 0xf1, 0x07, 0xf2, 0x0f, 0xfc, 0x00, 0x00};

// --- GLOBALS ---
int currentPage = 0;
bool highBrightness = true;
unsigned long lastPageSwitch = 0;

// --- TOUCH INPUT & MENU NAVIGATION ---
bool touchRawState = false;
bool touchStableState = false;
unsigned long touchLastRawChange = 0;
unsigned long touchPressStart = 0;
bool touchHoldHandled = false;
bool pendingSingleTap = false;
int pendingTapPage = -1;
unsigned long lastTapReleaseTime = 0;

const unsigned long TOUCH_DEBOUNCE_MS = 25;
const unsigned long HOLD_TO_MENU_MS = 550;
const unsigned long MENU_CONFIRM_HOLD_MS = 550;
const unsigned long MENU_IDLE_TIMEOUT_MS = 8000;
const unsigned long DOUBLE_TAP_WINDOW_MS = 280;

bool inMenuOverlay = false;
int menuSelection = 0;
const int MENU_ACTION_BRIGHTNESS = -1;
const int MENU_ACTION_CONFIG_MODE = -2;
const int MENU_ACTION_RESET_DEFAULTS = -3;
const int MENU_ITEM_COUNT_ONLINE = 11;
const int MENU_ITEM_COUNT_OFFLINE = 9;

const int MENU_ACTIONS_ONLINE[MENU_ITEM_COUNT_ONLINE] = {
  0, 1, 2, 3, 4, 5, 6, 7,
  MENU_ACTION_BRIGHTNESS, MENU_ACTION_CONFIG_MODE, MENU_ACTION_RESET_DEFAULTS};
const char *const MENU_LABELS_ONLINE[MENU_ITEM_COUNT_ONLINE] = {
  "Face",
  "Clock",
  "Weather",
  "World Clock",
  "Forecast",
  "Lyrics",
  "Health Stats",
  "Run Game",
  "Brightness",
  "Config Mode",
  "Reset Defaults"};

const int MENU_ACTIONS_OFFLINE[MENU_ITEM_COUNT_OFFLINE] = {
  0, 1, 2, 3, 4, 7,
  MENU_ACTION_BRIGHTNESS, MENU_ACTION_CONFIG_MODE, MENU_ACTION_RESET_DEFAULTS};
const char *const MENU_LABELS_OFFLINE[MENU_ITEM_COUNT_OFFLINE] = {
  "Face",
  "Clock",
  "Weather",
  "World Clock",
  "Forecast",
  "Run Game",
  "Brightness",
  "Config Mode",
  "Reset Defaults"};

unsigned long menuLastAction = 0;

// --- LYRICS ENGINE ---
String currentLyric = "";
int lyricTransitionType = 0;
unsigned long lyricStartTime = 0;

// --- CYBERDECK STATS ---
int pc_cpu = 0;
int pc_ram = 0;
int pc_up = 0;
int pc_down = 0;

// --- KORANGU RUN VARIABLES ---
float kY = 48;
float kVelY = 0;
float gravity = 0.6;
float jumpForce = -6.5;
bool isJumping = false;

float obsX = 128;
float obsSpeed = 3.5;
int kScore = 0;
int kHighScore = 0;
bool isGameOver = false;

// --- TELEPATHIC LINK VARIABLES ---
unsigned long lastUdpCheck = 0;
WiFiUDP udp;
String pc_ip = "";
WebSocketsClient webSocket;
bool wsConnected = false;
int lastReportedPage = -1;
unsigned long lastWsLoop = 0;
const unsigned long WS_LOOP_INTERVAL = 40;
const uint16_t WS_DEFAULT_PORT = 8765;
uint16_t wsServerPort = WS_DEFAULT_PORT;
unsigned long wsConnectBackoffMs = 1200;
unsigned long wsDiscoveryResetDeadline = 0;
unsigned long lastWsManualBegin = 0;
const unsigned long WS_RECONNECT_BASE_MS = 1200;
const unsigned long WS_RECONNECT_MAX_MS = 15000;
const unsigned long WS_DISCOVERY_RESET_MAX_MS = 45000;
bool pageStateResyncPending = false;
unsigned long pageStateResyncAt = 0;
const unsigned long PAGE_STATE_RESYNC_DELAY_MS = 900;

// --- CONNECTIVITY WATCHDOG ---
unsigned long lastWiFiWatchdog = 0;
unsigned long lastWiFiReconnectAttempt = 0;
int wifiReconnectAttempts = 0;
const unsigned long WIFI_WATCHDOG_INTERVAL = 5000;
const unsigned long WIFI_RECONNECT_BASE_MS = 3000;
const unsigned long WIFI_RECONNECT_MAX_MS = 30000;

// --- RUNTIME TELEMETRY COUNTERS (PHASE 7) ---
unsigned long statLoopCount = 0;
unsigned long statMenuOpenCount = 0;
unsigned long statMenuTapCount = 0;
unsigned long statDoubleTapSwapCount = 0;
unsigned long statPageTransitionCount = 0;
unsigned long statPageStateSendCount = 0;
unsigned long statPageStateResyncCount = 0;
unsigned long statWsDiscoveryHitCount = 0;
unsigned long statWsDiscoveryResetCount = 0;
unsigned long statWsConnectCount = 0;
unsigned long statWsDisconnectCount = 0;
unsigned long statWiFiReconnectCount = 0;
unsigned long statWiFiRadioResetCount = 0;
unsigned long statWeatherOkCount = 0;
unsigned long statWeatherRetryCount = 0;
unsigned long statWeatherConfigErrCount = 0;
unsigned long statWeatherOfflineCount = 0;
unsigned long lastTelemetryLogAt = 0;
const unsigned long TELEMETRY_LOG_INTERVAL_MS = 60000;

// MOODS
#define MOOD_NORMAL 0
#define MOOD_HAPPY 1
#define MOOD_SURPRISED 2
#define MOOD_SLEEPY 3
#define MOOD_ANGRY 4
#define MOOD_SAD 5
#define MOOD_EXCITED 6
#define MOOD_LOVE 7
#define MOOD_SUSPICIOUS 8
#define MOOD_FIGHTING_SLEEP 9
#define MOOD_RELAXED 10
int currentMood = MOOD_NORMAL;

// --- KORANGU'S METABOLISM & BRAIN ---
float energyLevel = 100.0;
unsigned long lastMetabolismTick = 0;
unsigned long lastThoughtTime = 0;
unsigned long nextThoughtInterval = 5000;
unsigned long pettingPauseUntil = 0;
int pettingCombo = 0;
unsigned long lastPetTime = 0;

String city;        // Loaded from Preferences
String countryCode; // Loaded from Preferences
String apiKey;      // Loaded from Preferences or defaults
String wifiSsid;    // Loaded from Preferences
String wifiPass;    // Loaded from Preferences
const unsigned long WEATHER_UPDATE_OK_MS = 600000;
const unsigned long WEATHER_UPDATE_RETRY_MS = 60000;
const unsigned long WEATHER_UPDATE_CONFIG_MS = 300000;
const unsigned long WEATHER_STALE_AFTER_MS = 900000;
unsigned long weatherUpdateInterval = WEATHER_UPDATE_RETRY_MS;
unsigned long weatherLastAttemptMs = 0;
unsigned long weatherLastSuccessMs = 0;
float temperature = 0.0;
float feelsLike = 0.0;
int humidity = 0;
String weatherMain = "Loading";
String weatherDesc = "Wait...";
String weatherRuntimeMessage = "Syncing...";

enum WeatherRuntimeState
{
  WEATHER_STATE_LOADING = 0,
  WEATHER_STATE_OK = 1,
  WEATHER_STATE_RETRY = 2,
  WEATHER_STATE_CONFIG_ERROR = 3,
  WEATHER_STATE_OFFLINE = 4
};

enum WeatherFetchStatus
{
  WEATHER_FETCH_OK = 0,
  WEATHER_FETCH_RETRY = 1,
  WEATHER_FETCH_CONFIG_ERROR = 2,
  WEATHER_FETCH_OFFLINE = 3
};

volatile int weatherRuntimeState = WEATHER_STATE_LOADING;

struct ForecastDay
{
  String dayName;
  int temp;
  String iconType;
};
ForecastDay fcast[3];
const char *ntpServer = "pool.ntp.org";
String tzString; // Loaded from Preferences (e.g. "IST-5:30")

SemaphoreHandle_t weatherMutex = nullptr;
TaskHandle_t weatherTaskHandle = nullptr;
void weatherWorkerTask(void *parameter);

struct WeatherViewData
{
  float temperature;
  float feelsLike;
  int humidity;
  String weatherMain;
  String weatherDesc;
  String runtimeMessage;
  int runtimeState;
  unsigned long lastSuccessMs;
  ForecastDay forecast[3];
};

// ==================================================
// 2. ULTRA PRO PHYSICS ENGINE
// ==================================================

struct Eye
{
  float x, y, w, h;
  float targetX, targetY, targetW, targetH;

  float pupilX, pupilY, pupilW, pupilH;
  float targetPupilX, targetPupilY, targetPupilW, targetPupilH;

  float topLid, targetTopLid;
  float bottomLid, targetBottomLid;

  // --- NEW: Physics Eyelid Angles (Eyebrows!) ---
  float lidAngle, targetLidAngle;

  float velX = 0, velY = 0, velW = 0, velH = 0;
  float pVelX = 0, pVelY = 0, pVelW = 0, pVelH = 0;
  float tLidVel = 0, bLidVel = 0, angleVel = 0;

  float k = 0.12;
  float d = 0.60;
  float pk = 0.08;
  float pd = 0.50;

  bool blinking;
  unsigned long lastBlink;
  unsigned long nextBlinkTime;

  void init(float _x, float _y, float _w, float _h)
  {
    x = targetX = _x;
    y = targetY = _y;
    w = targetW = _w;
    h = targetH = _h;
    pupilW = targetPupilW = _w / 2.2;
    pupilH = targetPupilH = _h / 2.2;
    topLid = targetTopLid = 0;
    bottomLid = targetBottomLid = 0;
    lidAngle = targetLidAngle = 0; // Initialize flat
    nextBlinkTime = millis() + random(1000, 4000);
  }

  void update()
  {
    velX = (velX + (targetX - x) * k) * d;
    x += velX;
    velY = (velY + (targetY - y) * k) * d;
    y += velY;
    velW = (velW + (targetW - w) * k) * d;
    w += velW;
    velH = (velH + (targetH - h) * k) * d;
    h += velH;

    pVelX = (pVelX + (targetPupilX - pupilX) * pk) * pd;
    pupilX += pVelX;
    pVelY = (pVelY + (targetPupilY - pupilY) * pk) * pd;
    pupilY += pVelY;
    pVelW = (pVelW + (targetPupilW - pupilW) * pk) * pd;
    pupilW += pVelW;
    pVelH = (pVelH + (targetPupilH - pupilH) * pk) * pd;
    pupilH += pVelH;

    tLidVel = (tLidVel + (targetTopLid - topLid) * k) * d;
    topLid += tLidVel;
    bLidVel = (bLidVel + (targetBottomLid - bottomLid) * k) * d;
    bottomLid += bLidVel;

    // Smooth Eyebrow Rotation!
    angleVel = (angleVel + (targetLidAngle - lidAngle) * k) * d;
    lidAngle += angleVel;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

// ==================================================
// 2b. CONFIG PORTAL (WiFi + API Key via local web)
// ==================================================
#define CONFIG_AP_SSID "KoranguDesk"
#define CONFIG_AP_PASS "12345678"
#define CONFIG_HOLD_MS 3000

Preferences prefs;
WebServer configServer(80);
bool inConfigMode = false;

const int MAX_SSID_CACHE = 18;
String ssidCache[MAX_SSID_CACHE];
int ssidCacheCount = 0;

String escapeHtml(const String &input)
{
  String out = "";
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); i++)
  {
    char c = input.charAt(i);
    if (c == '&')
      out += "&amp;";
    else if (c == '<')
      out += "&lt;";
    else if (c == '>')
      out += "&gt;";
    else if (c == '"')
      out += "&quot;";
    else if (c == '\'')
      out += "&#39;";
    else
      out += c;
  }
  return out;
}

int indexOfCachedSsid(const String &ssid)
{
  for (int i = 0; i < ssidCacheCount; i++)
  {
    if (ssidCache[i] == ssid)
      return i;
  }
  return -1;
}

void addCachedSsid(String ssid)
{
  ssid.trim();
  if (ssid.length() == 0)
    return;
  if (indexOfCachedSsid(ssid) >= 0)
    return;

  if (ssidCacheCount < MAX_SSID_CACHE)
  {
    ssidCache[ssidCacheCount++] = ssid;
  }
  else
  {
    for (int i = 1; i < MAX_SSID_CACHE; i++)
      ssidCache[i - 1] = ssidCache[i];
    ssidCache[MAX_SSID_CACHE - 1] = ssid;
  }
}

void parseSsidCacheBlob(const String &blob)
{
  ssidCacheCount = 0;
  int start = 0;

  while (start <= blob.length())
  {
    int end = blob.indexOf('\n', start);
    if (end < 0)
      end = blob.length();

    String token = blob.substring(start, end);
    token.trim();
    addCachedSsid(token);

    if (end >= blob.length())
      break;
    start = end + 1;
  }
}

String buildSsidCacheBlob()
{
  String blob = "";
  for (int i = 0; i < ssidCacheCount; i++)
  {
    if (i > 0)
      blob += "\n";
    blob += ssidCache[i];
  }
  return blob;
}

void loadSsidCacheFromPrefs()
{
  prefs.begin("deskbuddy", true);
  String cacheBlob = prefs.getString("ssidcache", "");
  String savedSsid = prefs.getString("ssid", "");
  prefs.end();

  parseSsidCacheBlob(cacheBlob);
  addCachedSsid(savedSsid);
}

void saveSsidCacheToPrefs()
{
  prefs.begin("deskbuddy", false);
  prefs.putString("ssidcache", buildSsidCacheBlob());
  prefs.end();
}

void scanNearbySsidsAndCache()
{
  int found = WiFi.scanNetworks();
  if (found > 0)
  {
    for (int i = 0; i < found; i++)
      addCachedSsid(WiFi.SSID(i));
  }
  WiFi.scanDelete();
  saveSsidCacheToPrefs();
}

String buildSsidOptionsHtml(const String &selectedSsid)
{
  String options = "<option value=\"\">-- Select scanned network --</option>";
  for (int i = 0; i < ssidCacheCount; i++)
  {
    String escaped = escapeHtml(ssidCache[i]);
    options += "<option value=\"";
    options += escaped;
    options += "\"";
    if (ssidCache[i] == selectedSsid)
      options += " selected";
    options += ">";
    options += escaped;
    options += "</option>";
  }
  return options;
}

void loadConfig()
{
  prefs.begin("deskbuddy", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  apiKey = prefs.getString("apikey", "");
  city = prefs.getString("city", "");
  countryCode = prefs.getString("country", "");
  tzString = prefs.getString("tz", "");
  prefs.end();

  wifiSsid.trim();
  wifiPass.trim();
  apiKey.trim();
  city.trim();
  countryCode.trim();
  countryCode.toUpperCase();
  tzString.trim();

  if (wifiSsid.isEmpty())
  {
    wifiSsid = "PinkLips";
    wifiPass = "KadakMaal";
    apiKey = "85570f2039d11a98001e357baff79de6";
    city = "Durgapur";
    countryCode = "IN";
    tzString = "IST-5:30";
  }
  else
  {
    if (apiKey.isEmpty())
      apiKey = "85570f2039d11a98001e357baff79de6";
    if (city.isEmpty())
      city = "Durgapur";
    if (countryCode.isEmpty())
      countryCode = "IN";
    if (tzString.isEmpty())
      tzString = "IST-5:30";
  }
}

void saveConfig(const String &s, const String &p, const String &ak,
                const String &cty, const String &ctry, const String &tz)
{
  prefs.begin("deskbuddy", false);
  prefs.putString("ssid", s);
  prefs.putString("pass", p);
  prefs.putString("apikey", ak);
  prefs.putString("city", cty);
  prefs.putString("country", ctry);
  prefs.putString("tz", tz);
  prefs.end();
}

void handleConfigRoot()
{
  loadSsidCacheFromPrefs();

  prefs.begin("deskbuddy", true);
  String sSsid = prefs.getString("ssid", "");
  String sApik = prefs.getString("apikey", "");
  String sCity = prefs.getString("city", "");
  String sCtry = prefs.getString("country", "IN");
  String sTz = prefs.getString("tz", "IST-5:30");
  prefs.end();

  String ssidOptions = buildSsidOptionsHtml(sSsid);
  String escSsid = escapeHtml(sSsid);
  String escApik = escapeHtml(sApik);
  String escCity = escapeHtml(sCity);
  String escCtry = escapeHtml(sCtry);
  String escTz = escapeHtml(sTz);

  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Virtual Pet Setup</title>
<style>
body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; max-width: 420px; margin: 30px auto; padding: 20px; background: #fff0f5; color: #333; }
h1 { color: #ff69b4; text-align: center; margin-bottom: 5px; font-size: 28px; }
p.subtitle { text-align: center; color: #888; margin-top: 0; margin-bottom: 25px; font-size: 14px; }
.container { background: #ffffff; padding: 25px; border-radius: 16px; box-shadow: 0 8px 20px rgba(0,0,0,0.08); }
input, select { width: 100%; padding: 12px; margin: 8px 0 16px; border: 2px solid #ffe4e1; border-radius: 8px; box-sizing: border-box; background: #fafafa; font-size: 15px; transition: 0.3s; }
input:focus, select:focus { outline: none; border-color: #ff69b4; background: #fff; box-shadow: 0 0 5px rgba(255,105,180,0.3); }
button { width: 100%; padding: 14px; background: #ff69b4; color: #fff; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; transition: 0.3s; margin-top: 10px; }
button:hover { background: #ff1493; transform: translateY(-2px); }
.secondary { background: #fff; color: #ff1493; border: 2px solid #ffb6d9; margin-top: 0; margin-bottom: 12px; }
.secondary:hover { background: #fff5fb; transform: translateY(-1px); }
.pw-wrap { position: relative; }
.pw-wrap input { padding-right: 72px; }
.pw-toggle { position: absolute; right: 8px; top: 10px; width: auto; padding: 6px 10px; margin: 0; font-size: 12px; background: #fff; color: #ff1493; border: 1px solid #ffb6d9; border-radius: 6px; }
.pw-toggle:hover { background: #fff5fb; transform: none; }
label { display: block; margin-top: 5px; color: #555; font-weight: bold; font-size: 13px; text-transform: uppercase; letter-spacing: 0.5px; }
.section { margin-top: 20px; padding-top: 20px; border-top: 2px dashed #ffe4e1; }
.section-title { color: #ff69b4; font-size: 16px; font-weight: bold; margin-bottom: 12px; display: flex; align-items: center; }
.helper { color: #888; font-size: 12px; margin-top: -10px; margin-bottom: 12px; }
</style></head><body>
<div class="container">
<h1>KoranguDesk Setup</h1>
<p class="subtitle">Connect your buddy to the internet!</p>
<form action="/scan" method="POST">
<button type="submit" class="secondary">Scan / Refresh WiFi List</button>
</form>
<p class="helper">Cached networks: )rawliteral";
  html += String(ssidCacheCount);
  html += R"rawliteral(</p>
<form action="/save" method="POST">
<label>Scanned WiFi Networks</label>
<select id="ssidSelect" name="ssid_select">)rawliteral";
  html += ssidOptions;
  html += R"rawliteral(</select>
<label>Or Enter WiFi Name Manually</label><input id="ssidManual" name="ssid_manual" placeholder="Enter WiFi Name" value=")rawliteral";
  html += escSsid;
  html += R"rawliteral(">
<label>WiFi Password</label>
<div class="pw-wrap">
<input id="wifiPass" name="pass" type="password" placeholder="Enter WiFi Password">
<button id="toggleWifiPassBtn" type="button" class="pw-toggle" onclick="toggleWifiPass()">Show</button>
</div>
<div class="section"><div class="section-title">Weather Settings</div>
<label>OpenWeather API Key</label><input name="apikey" placeholder="Paste API key here" value=")rawliteral";
  html += escApik;
  html += R"rawliteral(">
<label>City Name</label><input name="city" placeholder="e.g. Panagarh" value=")rawliteral";
  html += escCity;
  html += R"rawliteral(">
<label>Country Code</label><input name="country" placeholder="e.g. IN" value=")rawliteral";
  html += escCtry;
  html += R"rawliteral(">
</div>
<div class="section"><div class="section-title">Time Settings</div>
<label>Timezone</label><input name="tz" placeholder="e.g. IST-5:30" value=")rawliteral";
  html += escTz;
  html += R"rawliteral(">
</div>
<button type="submit">Save & Wake Up!</button>
</form></div>
<script>
function toggleWifiPass() {
  var passInput = document.getElementById('wifiPass');
  var toggleBtn = document.getElementById('toggleWifiPassBtn');
  if (!passInput || !toggleBtn) return;
  var reveal = passInput.type === 'password';
  passInput.type = reveal ? 'text' : 'password';
  toggleBtn.textContent = reveal ? 'Hide' : 'Show';
}
var ssidSelect = document.getElementById('ssidSelect');
var ssidManual = document.getElementById('ssidManual');
if (ssidSelect && ssidManual) {
  ssidSelect.addEventListener('change', function () {
    if (ssidSelect.value) {
      ssidManual.value = ssidSelect.value;
    }
  });
}
</script>
</body></html>)rawliteral";

  configServer.send(200, "text/html", html);
}

void sendConfigNoticePage(const String &title, const String &message, bool success)
{
  String escTitle = escapeHtml(title);
  String escMessage = escapeHtml(message);

  String html =
      "<html><body style='font-family:sans-serif;background:#fff0f5;color:#333;text-align:center;padding:50px;'>"
      "<div style='background:#fff;padding:30px;border-radius:16px;box-shadow:0 8px 20px rgba(0,0,0,0.08);display:inline-block;max-width:420px;'>"
      "<h2 style='font-size:30px;margin-bottom:10px;'>";
  html += escTitle;
  html += "</h2><p style='color:#666;font-size:17px;'>";
  html += escMessage;
  html += "</p>";

  if (!success)
  {
    html += "<p style='margin-top:20px;'><a href='/' style='color:#ff1493;text-decoration:none;font-weight:bold;'>Back to Setup</a></p>";
  }

  html += "</div></body></html>";
  configServer.send(success ? 200 : 400, "text/html", html);
}

void handleConfigScan()
{
  scanNearbySsidsAndCache();
  configServer.sendHeader("Location", "/", true);
  configServer.send(303, "text/plain", "Scan complete");
}

void handleConfigSave()
{
  String s = configServer.arg("ssid_manual");
  if (s.length() == 0 && configServer.hasArg("ssid_select"))
    s = configServer.arg("ssid_select");
  if (s.length() == 0 && configServer.hasArg("ssid"))
    s = configServer.arg("ssid"); // Backward compatibility.

  s.trim();
  if (s.length() == 0)
  {
    sendConfigNoticePage("SSID Required", "Choose a scanned network or type WiFi name manually.", false);
    return;
  }

  String p = configServer.arg("pass");
  String ak = configServer.arg("apikey");
  String cty = configServer.arg("city");
  String ctr = configServer.arg("country");
  String tz = configServer.arg("tz");

  p.trim();
  ak.trim();
  cty.trim();
  ctr.trim();
  ctr.toUpperCase();
  tz.trim();

  prefs.begin("deskbuddy", true);
  if (ak.isEmpty())
    ak = prefs.getString("apikey", "85570f2039d11a98001e357baff79de6");
  if (cty.isEmpty())
    cty = prefs.getString("city", "Durgapur");
  if (ctr.isEmpty())
    ctr = prefs.getString("country", "IN");
  if (tz.isEmpty())
    tz = prefs.getString("tz", "IST-5:30");
  prefs.end();

  addCachedSsid(s);
  saveSsidCacheToPrefs();

  saveConfig(s, p, ak, cty, ctr, tz);
  sendConfigNoticePage("All Saved!", "Your pet is waking up now with the new settings.", true);
  delay(2000);
  ESP.restart();
}

void startConfigPortal()
{
  inConfigMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASS);

  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(0, 0);
  display.print("Config mode\n\nConnect to:\n");
  display.print(CONFIG_AP_SSID);
  display.print("\n\nScanning WiFi...");
  display.display();

  loadSsidCacheFromPrefs();
  scanNearbySsidsAndCache();

  configServer.on("/", HTTP_GET, handleConfigRoot);
  configServer.on("/scan", HTTP_POST, handleConfigScan);
  configServer.on("/save", HTTP_POST, handleConfigSave);
  configServer.begin();
  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(0, 0);
  display.print("Config mode\n\nConnect to:\n");
  display.print(CONFIG_AP_SSID);
  display.print("\n\nThen open:\n192.168.4.1");
  display.display();
}

// ==================================================
// 3. LOGIC & NETWORK
// ==================================================
const unsigned char *getBigIcon(String w)
{
  if (w == "Clear")
    return bmp_clear;
  if (w == "Clouds")
    return bmp_clouds;
  if (w == "Rain" || w == "Drizzle")
    return bmp_rain;
  return bmp_clouds;
}
const unsigned char *getMiniIcon(String w)
{
  if (w == "Clear")
    return mini_sun;
  if (w == "Rain" || w == "Drizzle" || w == "Thunderstorm")
    return mini_rain;
  return mini_cloud;
}

void reportCurrentPageToServer(bool force = false)
{
  if (!wsConnected)
    return;

  if (force || currentPage != lastReportedPage)
  {
    char pageStateMsg[20];
    snprintf(pageStateMsg, sizeof(pageStateMsg), "PAGE_STATE,%d", currentPage);
    webSocket.sendTXT(pageStateMsg);
    lastReportedPage = currentPage;
    statPageStateSendCount++;
  }
}

unsigned long nextWsBackoff(unsigned long currentMs)
{
  if (currentMs < WS_RECONNECT_BASE_MS)
    return WS_RECONNECT_BASE_MS;

  unsigned long nextMs = currentMs * 2;
  if (nextMs < currentMs || nextMs > WS_RECONNECT_MAX_MS)
    nextMs = WS_RECONNECT_MAX_MS;
  return nextMs;
}

void refreshWsDiscoveryDeadline()
{
  unsigned long resetDelay = 12000UL + wsConnectBackoffMs;
  if (resetDelay > WS_DISCOVERY_RESET_MAX_MS)
    resetDelay = WS_DISCOVERY_RESET_MAX_MS;
  wsDiscoveryResetDeadline = millis() + resetDelay;
}

void schedulePageStateResync(unsigned long delayMs = PAGE_STATE_RESYNC_DELAY_MS)
{
  pageStateResyncPending = true;
  pageStateResyncAt = millis() + delayMs;
}

void flushPendingPageStateResync()
{
  if (!pageStateResyncPending || !wsConnected)
    return;

  if (millis() < pageStateResyncAt)
    return;

  reportCurrentPageToServer(true);
  statPageStateResyncCount++;
  pageStateResyncPending = false;
}

void logRuntimeTelemetry()
{
  unsigned long now = millis();
  if (now - lastTelemetryLogAt < TELEMETRY_LOG_INTERVAL_MS)
    return;

  lastTelemetryLogAt = now;

  Serial.println("---- Runtime stats ----");
  Serial.print("loop=");
  Serial.print(statLoopCount);
  Serial.print(" pageTx=");
  Serial.print(statPageStateSendCount);
  Serial.print(" resyncTx=");
  Serial.print(statPageStateResyncCount);
  Serial.print(" wsConn=");
  Serial.print(statWsConnectCount);
  Serial.print(" wsDisc=");
  Serial.print(statWsDisconnectCount);
  Serial.print(" wsBeacon=");
  Serial.print(statWsDiscoveryHitCount);
  Serial.print(" wsReset=");
  Serial.println(statWsDiscoveryResetCount);

  Serial.print("wifiRetry=");
  Serial.print(statWiFiReconnectCount);
  Serial.print(" wifiHardReset=");
  Serial.print(statWiFiRadioResetCount);
  Serial.print(" menuOpen=");
  Serial.print(statMenuOpenCount);
  Serial.print(" menuTap=");
  Serial.print(statMenuTapCount);
  Serial.print(" dblSwap=");
  Serial.println(statDoubleTapSwapCount);

  Serial.print("weather ok=");
  Serial.print(statWeatherOkCount);
  Serial.print(" retry=");
  Serial.print(statWeatherRetryCount);
  Serial.print(" cfg=");
  Serial.print(statWeatherConfigErrCount);
  Serial.print(" offline=");
  Serial.print(statWeatherOfflineCount);
  Serial.print(" heap=");
  Serial.println(ESP.getFreeHeap());
}

void updateMoodBasedOnWeather(const String &mainCondition, float tempValue)
{
  int m = MOOD_NORMAL;
  if (mainCondition == "Clear")
    m = MOOD_HAPPY;
  else if (mainCondition == "Rain" || mainCondition == "Drizzle")
    m = MOOD_SAD;
  else if (mainCondition == "Thunderstorm")
    m = MOOD_SURPRISED;
  else if (mainCondition == "Clouds")
    m = MOOD_NORMAL;
  else if (tempValue > 25)
    m = MOOD_EXCITED;
  else if (tempValue < 5)
    m = MOOD_SLEEPY;
  currentMood = m;
}

bool weatherLock(uint32_t timeoutMs = 40)
{
  if (weatherMutex == nullptr)
    return false;
  return xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void weatherUnlock()
{
  if (weatherMutex != nullptr)
    xSemaphoreGive(weatherMutex);
}

void setWeatherRuntimeState(int state, const String &message, unsigned long intervalMs, bool markSuccess)
{
  unsigned long now = millis();
  if (weatherLock())
  {
    weatherRuntimeState = state;
    weatherRuntimeMessage = message;
    weatherUpdateInterval = intervalMs;
    weatherLastAttemptMs = now;
    if (markSuccess)
      weatherLastSuccessMs = now;
    weatherUnlock();
  }
  else
  {
    weatherRuntimeState = state;
    weatherRuntimeMessage = message;
    weatherUpdateInterval = intervalMs;
    weatherLastAttemptMs = now;
    if (markSuccess)
      weatherLastSuccessMs = now;
  }
}

void copyWeatherView(WeatherViewData &out)
{
  if (weatherLock())
  {
    out.temperature = temperature;
    out.feelsLike = feelsLike;
    out.humidity = humidity;
    out.weatherMain = weatherMain;
    out.weatherDesc = weatherDesc;
    out.runtimeMessage = weatherRuntimeMessage;
    out.runtimeState = weatherRuntimeState;
    out.lastSuccessMs = weatherLastSuccessMs;
    for (int i = 0; i < 3; i++)
      out.forecast[i] = fcast[i];
    weatherUnlock();
  }
  else
  {
    out.temperature = temperature;
    out.feelsLike = feelsLike;
    out.humidity = humidity;
    out.weatherMain = weatherMain;
    out.weatherDesc = weatherDesc;
    out.runtimeMessage = weatherRuntimeMessage;
    out.runtimeState = weatherRuntimeState;
    out.lastSuccessMs = weatherLastSuccessMs;
    for (int i = 0; i < 3; i++)
      out.forecast[i] = fcast[i];
  }
}

String weatherStateTag(const WeatherViewData &view)
{
  if (view.runtimeState == WEATHER_STATE_LOADING)
    return "SYNC";
  if (view.runtimeState == WEATHER_STATE_CONFIG_ERROR)
    return "CFG";
  if (view.runtimeState == WEATHER_STATE_OFFLINE)
    return "OFF";
  if (view.runtimeState == WEATHER_STATE_RETRY)
    return "RETRY";

  if (view.lastSuccessMs > 0)
  {
    unsigned long age = millis() - view.lastSuccessMs;
    if (age > WEATHER_STALE_AFTER_MS)
      return "STALE";
  }

  return "";
}

void startWeatherWorker()
{
  if (weatherMutex == nullptr)
    weatherMutex = xSemaphoreCreateMutex();

  if (weatherTaskHandle == nullptr)
  {
    BaseType_t createResult = xTaskCreate(
        weatherWorkerTask,
        "weatherWorker",
        8192,
        nullptr,
        1,
        &weatherTaskHandle);

    if (createResult != pdPASS)
    {
      weatherTaskHandle = nullptr;
      Serial.println("Failed to start weather worker task");
    }
  }
}

void toggleBrightness()
{
  highBrightness = !highBrightness;
  display.setContrast(highBrightness ? 255 : 1);
}

void transitionToPage(int requestedPage, bool forceReport = false)
{
  if (requestedPage < 0)
    requestedPage = 0;
  if (requestedPage > 7)
    requestedPage = 7;
  if (!wsConnected && (requestedPage == 5 || requestedPage == 6))
    requestedPage = 0;

  bool changed = (currentPage != requestedPage);
  currentPage = requestedPage;
  if (changed)
    statPageTransitionCount++;

  pendingSingleTap = false;
  pendingTapPage = -1;
  lastPageSwitch = millis();
  lastSaccade = 0;

  if (currentPage == 5)
  {
    if (currentLyric.length() == 0)
      currentLyric = "[music]";
    lyricStartTime = millis();
    lyricTransitionType = random(0, 4);
  }

  if (inMenuOverlay)
    inMenuOverlay = false;

  if (changed || forceReport)
  {
    reportCurrentPageToServer(true);
    schedulePageStateResync();
  }
}

int getMenuItemCount()
{
  return wsConnected ? MENU_ITEM_COUNT_ONLINE : MENU_ITEM_COUNT_OFFLINE;
}

int getMenuActionAt(int index)
{
  if (wsConnected)
    return MENU_ACTIONS_ONLINE[index];
  return MENU_ACTIONS_OFFLINE[index];
}

const char *getMenuLabelAt(int index)
{
  if (wsConnected)
    return MENU_LABELS_ONLINE[index];
  return MENU_LABELS_OFFLINE[index];
}

void normalizeMenuSelection()
{
  int count = getMenuItemCount();
  if (count <= 0)
  {
    menuSelection = 0;
    return;
  }

  if (menuSelection < 0)
    menuSelection = 0;
  if (menuSelection >= count)
    menuSelection = count - 1;
}

int findMenuIndexByAction(int action)
{
  int count = getMenuItemCount();
  for (int i = 0; i < count; i++)
  {
    if (getMenuActionAt(i) == action)
      return i;
  }
  return 0;
}

void enterMenuOverlay()
{
  inMenuOverlay = true;
  statMenuOpenCount++;
  menuSelection = findMenuIndexByAction(currentPage);
  normalizeMenuSelection();
  pendingSingleTap = false;
  pendingTapPage = -1;
  menuLastAction = millis();
  Serial.println("Menu opened");
}

void handleGameTap(unsigned long now)
{
  Serial.println("Game tap detected!");
  if (isGameOver)
  {
    isGameOver = false;
    kScore = 0;
    obsX = 128;
    obsSpeed = 3.5;
    kY = 48;
    kVelY = 0;
    isJumping = false;
  }
  else if (!isJumping)
  {
    kVelY = jumpForce;
    isJumping = true;
  }

  lastPageSwitch = now;
}

void handlePetTap(unsigned long now)
{
  if (currentPage != 0)
    return;

  Serial.println("Face tap detected!");

  if (wsConnected)
  {
    char petMsg[48];
    snprintf(petMsg, sizeof(petMsg), "I just got petted! Combo: %d", pettingCombo);
    webSocket.sendTXT(petMsg);
  }

  energyLevel += 15.0;
  if (energyLevel > 100.0)
    energyLevel = 100.0;

  if (now - lastPetTime < 5000)
    pettingCombo++;
  else
    pettingCombo = 1;

  lastPetTime = now;

  if (pettingCombo == 1)
    currentMood = MOOD_HAPPY;
  else if (pettingCombo == 2)
    currentMood = MOOD_RELAXED;
  else if (pettingCombo == 3)
    currentMood = MOOD_LOVE;
  else
    currentMood = MOOD_EXCITED;

  lastSaccade = now + 4000;
  pettingPauseUntil = now + 4000;
}

void handleSingleTapAction(int sourcePage, unsigned long now)
{
  if (sourcePage == 0)
    handlePetTap(now);
}

void handleQuickSwapDoubleTap()
{
  int nextPage = 0;
  if (currentPage == 0)
    nextPage = 1;
  else if (currentPage == 1)
    nextPage = 2;
  else if (currentPage == 2)
    nextPage = 0;

  statDoubleTapSwapCount++;
  transitionToPage(nextPage, true);
}

void startConfigModeFromMenu()
{
  Serial.println("Menu action: config mode");
  wsConnected = false;
  pc_ip = "";
  lastReportedPage = -1;
  startConfigPortal();
}

void resetConfigToDefaults()
{
  Serial.println("Menu action: reset defaults");
  prefs.begin("deskbuddy", false);
  prefs.clear();
  prefs.end();

  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(0, 8);
  display.print("Defaults reset");
  display.setCursor(0, 24);
  display.print("Rebooting...");
  display.display();
  delay(1200);
  ESP.restart();
}

void handleMenuTap()
{
  int count = getMenuItemCount();
  if (count <= 0)
    return;

  statMenuTapCount++;
  normalizeMenuSelection();
  menuSelection++;
  if (menuSelection >= count)
    menuSelection = 0;
  menuLastAction = millis();
}

void commitMenuSelection()
{
  normalizeMenuSelection();
  int action = getMenuActionAt(menuSelection);
  inMenuOverlay = false;
  pendingSingleTap = false;
  pendingTapPage = -1;
  menuLastAction = millis();

  if (action == MENU_ACTION_BRIGHTNESS)
    toggleBrightness();
  else if (action == MENU_ACTION_CONFIG_MODE)
    startConfigModeFromMenu();
  else if (action == MENU_ACTION_RESET_DEFAULTS)
    resetConfigToDefaults();
  else
    transitionToPage(action, true);
}

void handleTouch()
{
  unsigned long now = millis();
  bool currentRead = digitalRead(TOUCH_PIN);

  if (currentRead != touchRawState)
  {
    touchRawState = currentRead;
    touchLastRawChange = now;
  }

  if ((now - touchLastRawChange) < TOUCH_DEBOUNCE_MS)
  {
    if (inMenuOverlay && !touchStableState && (now - menuLastAction > MENU_IDLE_TIMEOUT_MS))
      inMenuOverlay = false;
    return;
  }

  if (touchStableState != touchRawState)
  {
    touchStableState = touchRawState;

    if (touchStableState)
    {
      touchPressStart = now;
      touchHoldHandled = false;
    }
    else
    {
      unsigned long pressDuration = now - touchPressStart;

      if (!touchHoldHandled && pressDuration < HOLD_TO_MENU_MS)
      {
        if (inMenuOverlay)
          handleMenuTap();
        else if (currentPage == 7)
          handleGameTap(now);
        else
        {
          if (pendingSingleTap && (now - lastTapReleaseTime <= DOUBLE_TAP_WINDOW_MS))
          {
            pendingSingleTap = false;
            pendingTapPage = -1;
            handleQuickSwapDoubleTap();
          }
          else
          {
            pendingSingleTap = true;
            pendingTapPage = currentPage;
            lastTapReleaseTime = now;
          }
        }
      }

      menuLastAction = now;
    }
  }

  if (touchStableState && !touchHoldHandled)
  {
    unsigned long pressDuration = now - touchPressStart;

    if (!inMenuOverlay && pressDuration >= HOLD_TO_MENU_MS)
    {
      enterMenuOverlay();
      touchHoldHandled = true;
    }
    else if (inMenuOverlay && pressDuration >= MENU_CONFIRM_HOLD_MS)
    {
      commitMenuSelection();
      touchHoldHandled = true;
    }
  }

  if (inMenuOverlay && !touchStableState && (now - menuLastAction > MENU_IDLE_TIMEOUT_MS))
    inMenuOverlay = false;

  if (pendingSingleTap && (now - lastTapReleaseTime > DOUBLE_TAP_WINDOW_MS))
  {
    int sourcePage = pendingTapPage;
    pendingSingleTap = false;
    pendingTapPage = -1;
    handleSingleTapAction(sourcePage, now);
  }
}

WeatherFetchStatus getWeatherAndForecast(String &statusMessage)
{
  statusMessage = "";

  if (WiFi.status() != WL_CONNECTED)
  {
    statusMessage = "No WiFi";
    return WEATHER_FETCH_OFFLINE;
  }

  String cityQ = city;
  String countryQ = countryCode;
  cityQ.trim();
  countryQ.trim();
  countryQ.toUpperCase();

  if (cityQ.isEmpty())
    cityQ = "Durgapur";

  String query = cityQ;
  if (!countryQ.isEmpty())
    query += "," + countryQ;
  query.replace(" ", "%20");

  bool currentOk = false;
  float newTemp = temperature;
  float newFeelsLike = feelsLike;
  int newHumidity = humidity;
  String newMain = weatherMain;
  String newDesc = weatherDesc;

  HTTPClient http;
  http.setConnectTimeout(2500);
  http.setTimeout(4500);
  String url;
  url.reserve(query.length() + apiKey.length() + 96);
  url = "http://api.openweathermap.org/data/2.5/weather?q=" + query + "&appid=" + apiKey + "&units=metric";
  http.begin(url);
  int weatherCode = http.GET();
  if (weatherCode == 200)
  {
    String payload = http.getString();
    JSONVar myObject = JSON.parse(payload);
    if (JSON.typeof(myObject) != "undefined")
    {
      newTemp = double(myObject["main"]["temp"]);
      newFeelsLike = double(myObject["main"]["feels_like"]);
      newHumidity = int(myObject["main"]["humidity"]);
      newMain = (const char *)myObject["weather"][0]["main"];
      newDesc = (const char *)myObject["weather"][0]["description"];
      if (newDesc.length() > 0)
        newDesc.setCharAt(0, toupper(newDesc.charAt(0)));
      currentOk = true;
    }
    else
    {
      statusMessage = "Bad weather JSON";
    }
  }
  else
  {
    if (weatherCode == 401)
      statusMessage = "Bad API key";
    else if (weatherCode == 404)
      statusMessage = "City not found";
    else if (weatherCode < 0)
      statusMessage = "Weather net err";
    else
      statusMessage = "Weather " + String(weatherCode);

    Serial.print("Weather request failed. HTTP code: ");
    Serial.println(weatherCode);
    String errorBody = http.getString();
    if (errorBody.length() > 0)
    {
      Serial.print("Weather error body: ");
      Serial.println(errorBody);
    }

    http.end();
    if (weatherCode == 401 || weatherCode == 404)
      return WEATHER_FETCH_CONFIG_ERROR;
    if (statusMessage.length() == 0)
      statusMessage = "Weather request failed";
    return WEATHER_FETCH_RETRY;
  }
  http.end();

  if (!currentOk)
  {
    if (statusMessage.length() == 0)
      statusMessage = "Weather parse failed";
    return WEATHER_FETCH_RETRY;
  }

  ForecastDay newForecast[3];
  bool forecastOk = false;

  url = "http://api.openweathermap.org/data/2.5/forecast?q=" + query + "&appid=" + apiKey + "&units=metric";
  http.setConnectTimeout(2500);
  http.setTimeout(4500);
  http.begin(url);
  int forecastCode = http.GET();
  if (forecastCode == 200)
  {
    String payload = http.getString();
    JSONVar fo = JSON.parse(payload);
    if (JSON.typeof(fo) != "undefined")
    {
      struct tm t;
      int today = 0;
      if (getLocalTime(&t))
        today = t.tm_wday;

      const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      int indices[3] = {7, 15, 23};
      for (int i = 0; i < 3; i++)
      {
        int idx = indices[i];
        newForecast[i].temp = (int)double(fo["list"][idx]["main"]["temp"]);
        newForecast[i].iconType = (const char *)fo["list"][idx]["weather"][0]["main"];
        int nextDayIndex = (today + i + 1) % 7;
        newForecast[i].dayName = days[nextDayIndex];
      }
      forecastOk = true;
    }
  }
  else
  {
    Serial.print("Forecast request failed. HTTP code: ");
    Serial.println(forecastCode);
    statusMessage = "Forecast stale";
  }
  http.end();

  if (weatherLock(80))
  {
    temperature = newTemp;
    feelsLike = newFeelsLike;
    humidity = newHumidity;
    weatherMain = newMain;
    weatherDesc = newDesc;
    if (forecastOk)
    {
      for (int i = 0; i < 3; i++)
        fcast[i] = newForecast[i];
    }
    weatherUnlock();
  }
  else
  {
    temperature = newTemp;
    feelsLike = newFeelsLike;
    humidity = newHumidity;
    weatherMain = newMain;
    weatherDesc = newDesc;
    if (forecastOk)
    {
      for (int i = 0; i < 3; i++)
        fcast[i] = newForecast[i];
    }
  }

  updateMoodBasedOnWeather(newMain, newTemp);

  if (statusMessage.length() == 0)
    statusMessage = "Updated";

  return WEATHER_FETCH_OK;
}

void weatherWorkerTask(void *parameter)
{
  (void)parameter;
  setWeatherRuntimeState(WEATHER_STATE_LOADING, "Syncing...", WEATHER_UPDATE_RETRY_MS, false);

  unsigned long nextRunAt = 0;

  while (true)
  {
    if (inConfigMode)
    {
      vTaskDelay(pdMS_TO_TICKS(300));
      continue;
    }

    unsigned long now = millis();
    if (now < nextRunAt)
    {
      unsigned long waitMs = nextRunAt - now;
      if (waitMs > 300)
        waitMs = 300;
      vTaskDelay(pdMS_TO_TICKS(waitMs));
      continue;
    }

    String statusMessage = "";
    WeatherFetchStatus fetchStatus = getWeatherAndForecast(statusMessage);

    if (fetchStatus == WEATHER_FETCH_OK)
    {
      statWeatherOkCount++;
      setWeatherRuntimeState(WEATHER_STATE_OK, statusMessage, WEATHER_UPDATE_OK_MS, true);
      nextRunAt = millis() + WEATHER_UPDATE_OK_MS;
    }
    else if (fetchStatus == WEATHER_FETCH_CONFIG_ERROR)
    {
      statWeatherConfigErrCount++;
      if (statusMessage.length() == 0)
        statusMessage = "Check API/City";
      setWeatherRuntimeState(WEATHER_STATE_CONFIG_ERROR, statusMessage, WEATHER_UPDATE_CONFIG_MS, false);
      nextRunAt = millis() + WEATHER_UPDATE_CONFIG_MS;
    }
    else if (fetchStatus == WEATHER_FETCH_OFFLINE)
    {
      statWeatherOfflineCount++;
      setWeatherRuntimeState(WEATHER_STATE_OFFLINE, statusMessage, WEATHER_UPDATE_RETRY_MS, false);
      nextRunAt = millis() + WEATHER_UPDATE_RETRY_MS;
    }
    else
    {
      statWeatherRetryCount++;
      if (statusMessage.length() == 0)
        statusMessage = "Retrying";
      setWeatherRuntimeState(WEATHER_STATE_RETRY, statusMessage, WEATHER_UPDATE_RETRY_MS, false);
      nextRunAt = millis() + WEATHER_UPDATE_RETRY_MS;
    }

    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

// ==================================================
// 4. DRAWING & ANIMATION
// ==================================================

void drawEyelidMask(float x, float y, float w, float h, int mood, bool isLeft)
{
  int ix = (int)x;
  int iy = (int)y;
  int iw = (int)w;
  int ih = (int)h;
  display.setTextColor(SH110X_BLACK);

  // ANGRY: Sharp slanted cut
  if (mood == MOOD_ANGRY)
  {
    if (isLeft)
      for (int i = 0; i < 16; i++)
        display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SH110X_BLACK);
    else
      for (int i = 0; i < 16; i++)
        display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SH110X_BLACK);
  }
  // SAD: Inverse slanted cut
  else if (mood == MOOD_SAD)
  {
    if (isLeft)
      for (int i = 0; i < 16; i++)
        display.drawLine(ix, iy - 6 + i, ix + iw, iy + i, SH110X_BLACK);
    else
      for (int i = 0; i < 16; i++)
        display.drawLine(ix, iy + i, ix + iw, iy - 6 + i, SH110X_BLACK);
  }
  // HAPPY/LOVE: Cheek push up
  else if (mood == MOOD_HAPPY || mood == MOOD_LOVE || mood == MOOD_EXCITED)
  {
    display.fillRect(ix, iy + ih - 12, iw, 14, SH110X_BLACK);
    display.fillCircle(ix + iw / 2, iy + ih + 6, iw / 1.3, SH110X_BLACK); // Round cut
  }
  // SLEEPY: Heavy lids
  else if (mood == MOOD_SLEEPY)
  {
    display.fillRect(ix, iy, iw, ih / 2 + 2, SH110X_BLACK);
  }
  // SUSPICIOUS: One eye squint, one open
  else if (mood == MOOD_SUSPICIOUS)
  {
    if (isLeft)
      display.fillRect(ix, iy, iw, ih / 2 - 2, SH110X_BLACK);
    else
      display.fillRect(ix, iy + ih - 8, iw, 8, SH110X_BLACK);
  }
}

void drawUltraProEye(Eye &e, bool isLeft)
{
  int ix = (int)e.x;
  int iy = (int)e.y;
  int iw = (int)e.w;
  int ih = (int)e.h;
  int r = (iw < 20) ? 3 : 8;

  // 1. Draw White Sclera
  display.fillRoundRect(ix, iy, iw, ih, r, SH110X_WHITE);

  // 2. Calculate Pupil Position
  int cx = ix + iw / 2;
  int cy = iy + ih / 2;
  int pw = (int)e.pupilW;
  int ph = (int)e.pupilH;
  int px = cx + (int)e.pupilX - (pw / 2);
  int py = cy + (int)e.pupilY - (ph / 2);

  // Clamp pupil inside the eye so it doesn't break the illusion
  if (px < ix)
    px = ix;
  if (px + pw > ix + iw)
    px = ix + iw - pw;
  if (py < iy)
    py = iy;
  if (py + ph > iy + ih)
    py = iy + ih - ph;

  // 3. Draw Pupil & Glint (No duplicates!)
  if (currentMood == MOOD_LOVE)
  {
    // Draw the black heart bitmap as the pupil!
    display.drawBitmap(px, py, bmp_heart, 16, 16, SH110X_BLACK);
  }
  else
  {
    // Draw the normal black rounded pupil
    display.fillRoundRect(px, py, pw, ph, r / 2, SH110X_BLACK);

    // Dynamic Specular Glint!
    // ONLY draw the white reflection if the black pupil is bigger than 10px.
    // This perfectly fixes Surprised, Sleepy, and Suspicious!
    if (pw > 10 && ph > 10)
    {
      int glintR = pw / 5;
      if (glintR < 1)
        glintR = 1;
      if (glintR > 3)
        glintR = 3;

      int glintX = px + pw - (glintR * 2) - 1;
      int glintY = py + (glintR * 2) + 1;

      display.fillCircle(glintX, glintY, glintR, SH110X_WHITE);
    }
  }

  // 4. Smooth Physics Eyelids & Angled Eyebrows!
  int tLid = (int)e.topLid;
  int bLid = (int)e.bottomLid;
  int angle = (int)e.lidAngle;

  if (tLid > 0 || angle != 0)
  {
    // Base horizontal eyelid drop
    display.fillRect(ix, iy - 5, iw, tLid + 5, SH110X_BLACK);

    // Add the slanted "V" cuts using triangles
    if (angle > 0)
    {
      // ANGRY ( \ / )
      if (isLeft)
        display.fillTriangle(ix, iy + tLid, ix + iw, iy + tLid, ix + iw, iy + tLid + angle, SH110X_BLACK);
      else
        display.fillTriangle(ix, iy + tLid, ix + iw, iy + tLid, ix, iy + tLid + angle, SH110X_BLACK);
    }
    else if (angle < 0)
    {
      // SAD ( / \ )
      if (isLeft)
        display.fillTriangle(ix, iy + tLid, ix + iw, iy + tLid, ix, iy + tLid - angle, SH110X_BLACK);
      else
        display.fillTriangle(ix, iy + tLid, ix + iw, iy + tLid, ix + iw, iy + tLid - angle, SH110X_BLACK);
    }
  }

  if (bLid > 0)
  {
    display.fillRect(ix, iy + ih - bLid, iw, bLid + 2, SH110X_BLACK);
  }
}

void drawMouth(int mood)
{
  int mx = 64; // Center of screen
  int my = 50; // Tucked just below the newly raised eyes

  switch (mood)
  {
  case MOOD_HAPPY:
  case MOOD_EXCITED:
  case MOOD_LOVE:
    // Big wide smile (using a masked circle)
    display.fillCircle(mx, my - 3, 8, SH110X_WHITE);
    display.fillRect(mx - 9, my - 12, 18, 10, SH110X_BLACK); // mask top half
    break;

  case MOOD_SAD:
  case MOOD_ANGRY:
    // Frown
    display.fillCircle(mx, my + 5, 8, SH110X_WHITE);
    display.fillRect(mx - 9, my + 5, 18, 9, SH110X_BLACK); // mask bottom half
    break;

  case MOOD_SURPRISED:
    // Open mouth 'O'
    display.fillCircle(mx, my + 2, 5, SH110X_WHITE);
    display.fillCircle(mx, my + 2, 3, SH110X_BLACK);
    break;

  case MOOD_SLEEPY:
  case MOOD_FIGHTING_SLEEP:
    // Little sleepy 'o' drop
    display.fillCircle(mx, my + 2, 2, SH110X_WHITE);
    break;

  case MOOD_SUSPICIOUS:
    // Wry smirk (angled line)
    display.drawLine(mx + 2, my, mx + 8, my, SH110X_WHITE);
    break;

  case MOOD_NORMAL:
  case MOOD_RELAXED:
  default:
    // Cute little triangle smile (perfect for an adorable face!)
    display.fillTriangle(mx - 4, my, mx + 4, my, mx, my + 3, SH110X_WHITE);
    break;
  }
}

void runAutonomousBrain()
{
  unsigned long now = millis();

  // --- PAUSE BRAIN IF BEING PETTED ---
  if (now < pettingPauseUntil)
    return;

  // --- 1. THE METABOLISM (Runs every 1 second) ---
  if (now - lastMetabolismTick > 1000)
  {
    lastMetabolismTick = now;

    if (currentMood == MOOD_SLEEPY)
    {
      // SLOWER RECHARGE: Sleeps for ~3m 20s
      energyLevel += 0.5;

      if (energyLevel >= 100.0)
      {
        energyLevel = 100.0;
        currentMood = MOOD_NORMAL; // Wakes up fully rested!
        lastSaccade = 0;
      }
    }
    else
    {
      // SLOWER DRAIN: Stays awake for ~8m 20s
      energyLevel -= 0.2;
      if (energyLevel < 0)
        energyLevel = 0;
    }
  }

  // --- 2. THE DEEP SLEEP LOCK ---
  // If he is currently sleeping, STOP the brain here.
  // This prevents him from randomly waking up when his energy hits 20%!
  if (currentMood == MOOD_SLEEPY)
    return;

  // --- 3. EXHAUSTION OVERRIDES ---
  if (energyLevel <= 0)
  {
    currentMood = MOOD_SLEEPY; // Out of energy, forced to sleep
    return;
  }
  else if (energyLevel < 20)
  {
    // If energy drops below 20, he starts fighting sleep
    if (currentMood != MOOD_FIGHTING_SLEEP)
    {
      currentMood = MOOD_FIGHTING_SLEEP;
    }
    return; // Don't think about anything else while exhausted
  }

  // --- 4. NORMAL THOUGHTS (Only if energy >= 20 and awake) ---
  if (now - lastThoughtTime > nextThoughtInterval)
  {
    lastThoughtTime = now;
    nextThoughtInterval = random(5000, 15000);
    int diceRoll = random(100);

    // Massive variety of idle animations!
    if (diceRoll < 30)
      currentMood = MOOD_NORMAL;
    else if (diceRoll < 45)
      currentMood = MOOD_HAPPY;
    else if (diceRoll < 55)
      currentMood = MOOD_RELAXED;
    else if (diceRoll < 65)
      currentMood = MOOD_SUSPICIOUS;
    else if (diceRoll < 75)
      currentMood = MOOD_LOVE;
    else if (diceRoll < 82)
      currentMood = MOOD_SAD;
    else if (diceRoll < 90)
      currentMood = MOOD_EXCITED;
    else if (diceRoll < 95)
      currentMood = MOOD_SURPRISED;
    else
      currentMood = MOOD_ANGRY;

    lastSaccade = 0;
  }
}

// This function fires instantly whenever the PC sends a command!
String lyricStatusToText(const String &status)
{
  if (status == "NO_SONG")
    return "[No song playing]";
  if (status == "LYRICS_NOT_FOUND")
    return "[Lyrics not found]";
  if (status == "NO_MEDIA_SESSION")
    return "[No media session]";
  if (status == "LYRICS_FETCH_ERROR")
    return "[Lyrics fetch failed]";
  if (status == "WAITING_LINE")
    return "[music]";
  return "[music]";
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    statWsDisconnectCount++;
    pc_ip = "";
    wsConnected = false;
    lastReportedPage = -1;
    wsServerPort = WS_DEFAULT_PORT;
    wsConnectBackoffMs = nextWsBackoff(wsConnectBackoffMs);
    webSocket.setReconnectInterval(wsConnectBackoffMs);
    refreshWsDiscoveryDeadline();
    break;

  case WStype_CONNECTED:
    statWsConnectCount++;
    wsConnected = true;
    wsConnectBackoffMs = WS_RECONNECT_BASE_MS;
    webSocket.setReconnectInterval(wsConnectBackoffMs);
    wsDiscoveryResetDeadline = 0;
    webSocket.sendTXT("Hello PC, I am awake!");
    reportCurrentPageToServer(true);
    schedulePageStateResync();
    break;

  case WStype_TEXT:
  {
    String text = "";
    text.reserve(length + 1);
    for (size_t i = 0; i < length; i++)
      text += (char)payload[i];
    text.trim();
    if (text.length() == 0)
      break;

    if (text == "GET_PAGE_STATE")
    {
      reportCurrentPageToServer(true);
    }
    else if (text == "CONFIG_MODE")
    {
      webSocket.sendTXT("CONFIG_MODE,ACK");
      delay(60);
      wsConnected = false;
      pc_ip = "";
      lastReportedPage = -1;
      startConfigPortal();
    }
    // 1. Handle incoming Lyrics
    else if (text.startsWith("LYRIC:"))
    {
      String incomingLyric = text.substring(6);
      incomingLyric.trim();
      currentLyric = incomingLyric.length() > 0 ? incomingLyric : "[music]";
      if (currentPage == 5)
      {
        lyricStartTime = millis();
        lyricTransitionType = random(0, 4); // Pick a random animation!
      }
    }
    else if (text.startsWith("LYRIC_STATUS:"))
    {
      String statusCode = text.substring(13);
      statusCode.trim();
      currentLyric = lyricStatusToText(statusCode);
      if (currentPage == 5)
      {
        lyricStartTime = millis();
        lyricTransitionType = random(0, 4);
      }
    }
    else if (text.startsWith("STATS:"))
    {
      int parsed = sscanf(text.c_str(), "STATS:%d,%d,%d,%d", &pc_cpu, &pc_ram, &pc_up, &pc_down);
      if (parsed == 4)
      {
        if (pc_cpu < 0)
          pc_cpu = 0;
        if (pc_cpu > 100)
          pc_cpu = 100;
        if (pc_ram < 0)
          pc_ram = 0;
        if (pc_ram > 100)
          pc_ram = 100;
        if (pc_up < 0)
          pc_up = 0;
        if (pc_down < 0)
          pc_down = 0;

        if (pc_cpu > 95 && currentPage == 0)
        {
          currentMood = MOOD_ANGRY;
          lastSaccade = millis() + 3000;
        }
      }
    }
    // 2. Handle manual Page Swapping from dashboard
    else if (text.startsWith("PAGE,"))
    {
      int requestedPage = text.substring(5).toInt();
      transitionToPage(requestedPage, true);
    }
    // 3. Handle Mood & Energy changes
    else
    {
      int commaIndex = text.indexOf(',');
      if (commaIndex > 0)
      {
        int incomingMood = text.substring(0, commaIndex).toInt();
        int incomingEnergy = text.substring(commaIndex + 1).toInt();

        if (incomingMood != -1)
        {
          currentMood = incomingMood;
          lastSaccade = millis() + 4000;
          pettingPauseUntil = millis() + 4000;
        }
        if (incomingEnergy > 0)
        {
          energyLevel += incomingEnergy;
          if (energyLevel > 100.0)
            energyLevel = 100.0;
        }
      }
    }
    break;
  }
  }
}

void establishTelepathicLink()
{
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED)
  {
    if (pc_ip == "")
    {
      // Active discovery probe: helps when broadcast packets are dropped.
      if (now - lastUdpCheck > 2000)
      {
        udp.beginPacket(IPAddress(255, 255, 255, 255), 4210);
        udp.print("KORANGU_DISCOVER");
        udp.endPacket();
        lastUdpCheck = now;
      }

      // 1. Listen for the UDP Beacon
      int packetSize = udp.parsePacket();
      if (packetSize)
      {
        char incomingPacket[256];
        int len = udp.read(incomingPacket, 255);
        if (len >= 0)
          incomingPacket[(len < 255) ? len : 255] = 0;

        String beacon = String(incomingPacket);
        beacon.trim();

        if (beacon.startsWith("KORANGU_SERVER"))
        {
          statWsDiscoveryHitCount++;
          int beaconPort = WS_DEFAULT_PORT;
          int colonIndex = beacon.indexOf(':');
          if (colonIndex > 0)
          {
            int parsedPort = beacon.substring(colonIndex + 1).toInt();
            if (parsedPort > 0 && parsedPort <= 65535)
              beaconPort = parsedPort;
          }

          wsServerPort = (uint16_t)beaconPort;
          pc_ip = udp.remoteIP().toString();
          lastUdpCheck = now;
          Serial.print("SERVER FOUND! Connecting WebSocket to: ");
          Serial.print(pc_ip);
          Serial.print(":");
          Serial.println(wsServerPort);

          // 2. Open the Tunnel!
          webSocket.begin(pc_ip, wsServerPort, "/ws");
          webSocket.onEvent(webSocketEvent);
          webSocket.setReconnectInterval(wsConnectBackoffMs);
          lastWsManualBegin = now;
          refreshWsDiscoveryDeadline();
        }
      }
    }
    else
    {
      // 3. Keep the tunnel alive!
      if (now - lastWsLoop > WS_LOOP_INTERVAL)
      {
        webSocket.loop();
        lastWsLoop = now;
      }

      if (!wsConnected && (now - lastWsManualBegin >= wsConnectBackoffMs))
      {
        webSocket.begin(pc_ip, wsServerPort, "/ws");
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(wsConnectBackoffMs);
        lastWsManualBegin = now;
      }

      // If tunnel never comes up, clear host and rediscover.
      if (!wsConnected && wsDiscoveryResetDeadline > 0 && now >= wsDiscoveryResetDeadline)
      {
        Serial.println("WS reconnect stale. Restarting discovery...");
        statWsDiscoveryResetCount++;
        pc_ip = "";
        wsServerPort = WS_DEFAULT_PORT;
        wsDiscoveryResetDeadline = 0;
        lastUdpCheck = now;
      }
    }
  }
  else
  {
    wsConnected = false;
    pc_ip = "";
    lastReportedPage = -1;
    wsServerPort = WS_DEFAULT_PORT;
    wsDiscoveryResetDeadline = 0;
    lastWsManualBegin = 0;
  }
}

void runWiFiWatchdog()
{
  unsigned long now = millis();
  if (now - lastWiFiWatchdog < WIFI_WATCHDOG_INTERVAL)
    return;

  lastWiFiWatchdog = now;

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiReconnectAttempts = 0;
    return;
  }

  unsigned long retryDelay = WIFI_RECONNECT_BASE_MS << min(wifiReconnectAttempts, 3);
  if (retryDelay > WIFI_RECONNECT_MAX_MS)
    retryDelay = WIFI_RECONNECT_MAX_MS;

  if (now - lastWiFiReconnectAttempt < retryDelay)
    return;

  lastWiFiReconnectAttempt = now;
  wifiReconnectAttempts++;
  statWiFiReconnectCount++;

  Serial.print("WiFi watchdog reconnect attempt ");
  Serial.println(wifiReconnectAttempts);

  if (wifiReconnectAttempts % 4 == 0)
  {
    statWiFiRadioResetCount++;
    WiFi.disconnect(true, false);
    delay(20);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }
  else
  {
    WiFi.reconnect();
  }

  wsConnected = false;
  pc_ip = "";
  lastReportedPage = -1;
}

void updatePhysicsAndMood()
{
  unsigned long now = millis();
  breathVal = sin(now / 800.0) * 1.5; // Breathing effect

  // --- BLINK LOGIC ---
  // Don't blink if fast asleep or fighting it!
  bool canBlink = (currentMood != MOOD_SLEEPY && currentMood != MOOD_FIGHTING_SLEEP);

  if (canBlink)
  {
    if (now > leftEye.nextBlinkTime)
    {
      leftEye.blinking = true;
      leftEye.lastBlink = now;
      rightEye.blinking = true;
      leftEye.nextBlinkTime = now + random(2000, 6000);
    }
  }
  else
  {
    leftEye.blinking = false; // Force blink off when sleeping
    rightEye.blinking = false;
  }

  if (leftEye.blinking)
  {
    leftEye.targetH = 2;
    rightEye.targetH = 2; // Close
    if (now - leftEye.lastBlink > 120)
    {
      leftEye.blinking = false;
      rightEye.blinking = false;
    }
  }

  // --- SACCADE (Gaze) LOGIC ---
  if (!leftEye.blinking && now - lastSaccade > saccadeInterval)
  {
    lastSaccade = now;
    saccadeInterval = random(500, 3000);

    // Pick direction
    int dir = random(0, 10);
    float lx = 0, ly = 0;

    if (dir < 4)
    {
      lx = 0;
      ly = 0;
    } // Center
    else if (dir == 4)
    {
      lx = -6;
      ly = -4;
    } // TL
    else if (dir == 5)
    {
      lx = 6;
      ly = -4;
    } // TR
    else if (dir == 6)
    {
      lx = -6;
      ly = 4;
    } // BL
    else if (dir == 7)
    {
      lx = 6;
      ly = 4;
    } // BR
    else if (dir == 8)
    {
      lx = 8;
      ly = 0;
    } // R
    else if (dir == 9)
    {
      lx = -8;
      ly = 0;
    } // L

    // Move pupil target relative to center
    leftEye.targetPupilX = lx;
    leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx;
    rightEye.targetPupilY = ly;

    // Move eye container slightly (Head follow)
    leftEye.targetX = 22 + (lx * 0.3);
    leftEye.targetY = 10 + (ly * 0.3);
    rightEye.targetX = 78 + (lx * 0.3);
    rightEye.targetY = 10 + (ly * 0.3);
  }

  // --- MOOD SHAPES & AUTONOMOUS BRAIN ---
  if (!leftEye.blinking)
  {
    runAutonomousBrain();

    // UPDATE: Scaled down base sizes
    float baseW = 28;
    float baseH = 28 + breathVal;

    // Default resting state (Everything open)
    leftEye.targetW = rightEye.targetW = baseW;
    leftEye.targetH = rightEye.targetH = baseH;
    leftEye.targetPupilW = rightEye.targetPupilW = baseW / 2.2;
    leftEye.targetPupilH = rightEye.targetPupilH = baseH / 2.2;
    leftEye.targetTopLid = rightEye.targetTopLid = 0;
    leftEye.targetBottomLid = rightEye.targetBottomLid = 0;
    leftEye.targetLidAngle = rightEye.targetLidAngle = 0;

    switch (currentMood)
    {
    case MOOD_LOVE:
      leftEye.targetW = rightEye.targetW = 30;
      leftEye.targetH = rightEye.targetH = 30;
      leftEye.targetTopLid = rightEye.targetTopLid = 0;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 0;
      leftEye.targetPupilW = rightEye.targetPupilW = 16;
      leftEye.targetPupilH = rightEye.targetPupilH = 16;
      break;

    case MOOD_HAPPY:
      leftEye.targetBottomLid = rightEye.targetBottomLid = 10;
      leftEye.targetPupilW = rightEye.targetPupilW = 16;
      leftEye.targetPupilH = rightEye.targetPupilH = 16;
      break;

    case MOOD_EXCITED:
      leftEye.targetW = rightEye.targetW = 34;
      leftEye.targetH = rightEye.targetH = 36;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 6;
      leftEye.targetPupilW = rightEye.targetPupilW = 20;
      leftEye.targetPupilH = rightEye.targetPupilH = 20;
      leftEye.targetPupilX = rightEye.targetPupilX = random(-2, 3);
      leftEye.targetPupilY = rightEye.targetPupilY = random(-2, 3);
      break;

    case MOOD_SLEEPY:
      leftEye.targetTopLid = rightEye.targetTopLid = 20;
      leftEye.targetPupilW = rightEye.targetPupilW = 8;
      leftEye.targetPupilH = rightEye.targetPupilH = 8;
      break;

    case MOOD_SUSPICIOUS:
      leftEye.targetTopLid = 10;
      rightEye.targetTopLid = 4;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 4;
      leftEye.targetLidAngle = rightEye.targetLidAngle = 4;
      leftEye.targetPupilW = rightEye.targetPupilW = 12;
      leftEye.targetPupilH = rightEye.targetPupilH = 12;
      leftEye.targetPupilX = rightEye.targetPupilX = -6;
      leftEye.targetPupilY = rightEye.targetPupilY = 2;
      break;

    case MOOD_SURPRISED:
      leftEye.targetW = rightEye.targetW = 20;
      leftEye.targetH = rightEye.targetH = 34;
      leftEye.targetTopLid = rightEye.targetTopLid = 0;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 0;
      leftEye.targetLidAngle = rightEye.targetLidAngle = 0;
      leftEye.targetPupilW = rightEye.targetPupilW = 8;
      leftEye.targetPupilH = rightEye.targetPupilH = 8;
      leftEye.targetPupilX = rightEye.targetPupilX = 0;
      leftEye.targetPupilY = rightEye.targetPupilY = 0;
      break;

    case MOOD_ANGRY:
      leftEye.targetTopLid = rightEye.targetTopLid = 6;
      leftEye.targetLidAngle = rightEye.targetLidAngle = 8;
      leftEye.targetPupilW = rightEye.targetPupilW = 10;
      leftEye.targetPupilH = rightEye.targetPupilH = 10;
      break;

    case MOOD_SAD:
      leftEye.targetTopLid = rightEye.targetTopLid = 6;
      leftEye.targetLidAngle = rightEye.targetLidAngle = -7;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 4;
      leftEye.targetPupilW = rightEye.targetPupilW = 22;
      leftEye.targetPupilH = rightEye.targetPupilH = 22;
      leftEye.targetPupilY = rightEye.targetPupilY = 3;
      break;

    case MOOD_RELAXED:
    {
      leftEye.targetTopLid = rightEye.targetTopLid = 14;
      leftEye.targetBottomLid = rightEye.targetBottomLid = 0;
      leftEye.targetPupilW = rightEye.targetPupilW = 16;
      leftEye.targetPupilH = rightEye.targetPupilH = 16;
      float wiggle = sin(now / 400.0) * 2.0;
      leftEye.targetPupilX = rightEye.targetPupilX = wiggle;
      break;
    }

    case MOOD_FIGHTING_SLEEP:
    {
      int nodCycle = now % 3000;
      if (nodCycle < 2500)
      {
        float dropProgress = nodCycle / 2500.0;
        leftEye.targetTopLid = rightEye.targetTopLid = dropProgress * 24;
        leftEye.targetPupilW = rightEye.targetPupilW = 10;
        leftEye.targetPupilH = rightEye.targetPupilH = 10;
      }
      else
      {
        leftEye.targetTopLid = rightEye.targetTopLid = 2;
        leftEye.targetPupilW = rightEye.targetPupilW = 8;
        leftEye.targetPupilH = rightEye.targetPupilH = 8;
      }
      break;
    }
    }
  }

  leftEye.update();
  rightEye.update();
}

void drawEmoPage()
{
  updatePhysicsAndMood();
  unsigned long now = millis();

  // Draw Floating Animated Particles based on Mood
  if (currentMood == MOOD_LOVE)
  {
    // Hearts float up and sway left/right
    int floatY = (now / 40) % 24;
    int swayX = sin(now / 150.0) * 4;
    display.drawBitmap(98 + swayX, 20 - floatY, bmp_heart, 16, 16, SH110X_WHITE);
  }
  else if (currentMood == MOOD_SLEEPY)
  {
    // Zzz floats diagonally up and to the right
    int floatY = (now / 60) % 24;
    int floatX = (now / 80) % 10;
    display.drawBitmap(100 + floatX, 20 - floatY, bmp_zzz, 16, 16, SH110X_WHITE);
  }
  else if (currentMood == MOOD_ANGRY)
  {
    // Anger mark shakes violently in place
    int shakeX = random(-2, 3);
    int shakeY = random(-2, 3);
    display.drawBitmap(56 + shakeX, 0 + shakeY, bmp_anger, 16, 16, SH110X_WHITE);
  }

  drawUltraProEye(leftEye, true);
  drawUltraProEye(rightEye, false);
  drawMouth(currentMood);
}

// --- STANDARD PAGES ---
void drawForecastPage()
{
  WeatherViewData weatherView;
  copyWeatherView(weatherView);

  display.fillRect(0, 0, 128, 16, SH110X_WHITE);
  display.setFont(NULL);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(20, 4);
  display.print("3-DAY FORECAST");

  String tag = weatherStateTag(weatherView);
  if (tag.length() > 0)
  {
    display.setCursor(94, 4);
    display.print(tag);
  }

  display.setTextColor(SH110X_WHITE);
  display.drawLine(42, 16, 42, 64, SH110X_WHITE);
  display.drawLine(85, 16, 85, 64, SH110X_WHITE);
  for (int i = 0; i < 3; i++)
  {
    int xStart = i * 43;
    int centerX = xStart + 21;
    display.setFont(NULL);
    String d = weatherView.forecast[i].dayName;
    if (d == "")
      d = "Wait";
    display.setCursor(centerX - (d.length() * 3), 20);
    display.print(d);
    display.drawBitmap(centerX - 8, 28, getMiniIcon(weatherView.forecast[i].iconType), 16, 16, SH110X_WHITE);
    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    char forecastTemp[8];
    snprintf(forecastTemp, sizeof(forecastTemp), "%d", weatherView.forecast[i].temp);
    display.getTextBounds(forecastTemp, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(centerX - (w / 2) - 2, 60);
    display.print(forecastTemp);
    display.fillCircle(centerX + (w / 2) + 1, 52, 2, SH110X_WHITE);
  }
}
void drawClock()
{
  struct tm t;
  if (!getLocalTime(&t))
  {
    display.setFont(NULL);
    display.setCursor(30, 30);
    display.print("Syncing...");
    return;
  }
  String ampm = (t.tm_hour >= 12) ? "PM" : "AM";
  int h12 = t.tm_hour % 12;
  if (h12 == 0)
    h12 = 12;
  display.setTextColor(SH110X_WHITE);
  display.setFont(NULL);
  display.setTextSize(1);
  display.setCursor(114, 0);
  display.print(ampm);
  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", h12, t.tm_min);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 42);
  display.print(timeStr);
  display.setFont(&FreeSans9pt7b);
  char dateStr[20];
  strftime(dateStr, 20, "%a, %b %d", &t);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 62);
  display.print(dateStr);
}
void drawWeatherCard()
{
  WeatherViewData weatherView;
  copyWeatherView(weatherView);

  // --- 1. SMALLER WEATHER ICON ---
  // We swapped getBigIcon for getMiniIcon, and moved it to X:112 so it sits top-right!
  display.drawBitmap(112, 0, getMiniIcon(weatherView.weatherMain), 16, 16, SH110X_WHITE);

  String tag = weatherStateTag(weatherView);
  if (tag.length() > 0)
  {
    display.setFont(NULL);
    display.setCursor(82, 0);
    display.print(tag);
  }

  // --- 2. DYNAMIC FONT SCALING FOR CITY NAME ---
  char cityLabel[28];
  city.toCharArray(cityLabel, sizeof(cityLabel));
  if (cityLabel[0] == '\0')
    snprintf(cityLabel, sizeof(cityLabel), "CITY");
  for (int i = 0; cityLabel[i] != '\0'; i++)
    cityLabel[i] = toupper(cityLabel[i]);

  size_t cityLen = strlen(cityLabel);
  if (cityLen <= 8)
  {
    // Short names get the big bold font
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(0, 14); // Custom fonts draw from the bottom-up
  }
  else
  {
    // Long names shrink down to the smaller default font
    display.setFont(NULL);
    display.setTextSize(1);
    display.setCursor(0, 4); // Default font draws from the top-down
  }
  display.print(cityLabel);
  // ---------------------------------------------

  // --- 3. THE REST OF THE CARD ---
  display.setFont(&FreeSansBold18pt7b);
  int tempInt = (int)weatherView.temperature;
  char tempText[8];
  snprintf(tempText, sizeof(tempText), "%d", tempInt);
  display.setCursor(0, 48);
  display.print(tempText);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(tempText, 0, 48, &x1, &y1, &w, &h);
  display.fillCircle(x1 + w + 5, 26, 4, SH110X_WHITE); // Degree symbol

  display.setFont(NULL);
  display.drawBitmap(88, 32, bmp_tiny_drop, 8, 8, SH110X_WHITE);
  display.setCursor(100, 32);
  display.print(weatherView.humidity);
  display.print("%");

  display.setCursor(88, 45);
  display.print("~");
  display.print((int)weatherView.feelsLike);

  display.drawLine(0, 52, 128, 52, SH110X_WHITE);
  display.setCursor(0, 55);
  String bottomLine = weatherView.weatherDesc;
  if (weatherView.runtimeState != WEATHER_STATE_OK && weatherView.runtimeMessage.length() > 0)
    bottomLine = weatherView.runtimeMessage;
  char bottomText[22];
  bottomLine.toCharArray(bottomText, sizeof(bottomText));
  display.print(bottomText);
}
void drawWorldClock()
{
  time_t now;
  time(&now);
  time_t indiaEpoch = now + (5 * 3600) + (30 * 60);
  time_t sydneyEpoch = now + (11 * 3600);
  struct tm *indiatm = gmtime(&indiaEpoch);
  int i_h = indiatm->tm_hour;
  int i_m = indiatm->tm_min;
  struct tm *sydneytm = gmtime(&sydneyEpoch);
  int s_h = sydneytm->tm_hour;
  int s_m = sydneytm->tm_min;
  display.fillRect(0, 0, 128, 16, SH110X_WHITE);
  display.setFont(NULL);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(32, 4);
  display.print("WORLD CLOCK");
  display.setTextColor(SH110X_WHITE);
  display.drawLine(64, 18, 64, 54, SH110X_WHITE);
  display.setFont(NULL);
  display.setCursor(16, 22);
  display.print("INDIA");
  display.setFont(&FreeSansBold9pt7b);
  char iStr[10];
  sprintf(iStr, "%02d:%02d", i_h, i_m);
  display.setCursor(5, 46);
  display.print(iStr);
  display.setFont(NULL);
  display.setCursor(78, 22);
  display.print("SYDNEY");
  display.setFont(&FreeSansBold9pt7b);
  char sStr[10];
  sprintf(sStr, "%02d:%02d", s_h, s_m);
  display.setCursor(69, 46);
  display.print(sStr);
  display.setFont(NULL);
  display.setCursor(35, 56);
  display.print("Tap to Exit");
}

void drawCyberdeckPage()
{
  display.setFont(NULL);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  const int barX = 24;
  const int barW = 76;
  const int barInnerW = barW - 2;
  const int percentX = 102;

  int cpuBar = pc_cpu;
  if (cpuBar < 0)
    cpuBar = 0;
  if (cpuBar > 100)
    cpuBar = 100;

  int ramBar = pc_ram;
  if (ramBar < 0)
    ramBar = 0;
  if (ramBar > 100)
    ramBar = 100;

  int cpuFill = (cpuBar * barInnerW) / 100;
  int ramFill = (ramBar * barInnerW) / 100;

  display.setCursor(0, 0);
  display.print("SYS.VITALS");
  display.drawLine(0, 9, 128, 9, SH110X_WHITE);

  if ((millis() / 500) % 2 == 0)
    display.fillRect(120, 0, 8, 8, SH110X_WHITE);

  display.setCursor(0, 15);
  display.print("CPU");
  display.drawRect(barX, 15, barW, 7, SH110X_WHITE);
  display.fillRect(barX + 1, 16, cpuFill, 5, SH110X_WHITE);
  display.setCursor(percentX, 15);
  display.print(cpuBar);
  display.print("%");

  display.setCursor(0, 27);
  display.print("RAM");
  display.drawRect(barX, 27, barW, 7, SH110X_WHITE);
  display.fillRect(barX + 1, 28, ramFill, 5, SH110X_WHITE);
  display.setCursor(percentX, 27);
  display.print(ramBar);
  display.print("%");

  display.setCursor(0, 42);
  display.print("UP: ");
  display.print(pc_up);
  display.print(" KB/s");

  display.setCursor(0, 52);
  display.print("DN: ");
  display.print(pc_down);
  display.print(" KB/s");
}

void drawKoranguRun()
{
  display.setFont(NULL);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  const int playerX = 10;
  const int playerW = 8;
  const int playerH = 8;
  const int obstacleW = 8;
  const int obstacleH = 12;
  const int obstacleY = 44;

  if (isGameOver)
  {
    display.setCursor(38, 15);
    display.print("GAME OVER");
    display.setCursor(40, 30);
    display.print("Score: ");
    display.print(kScore);

    if ((millis() / 400) % 2 == 0)
    {
      display.setCursor(25, 45);
      display.print("Tap to Restart");
    }
    return;
  }

  // 1) Physics
  kVelY += gravity;
  kY += kVelY;

  if (kY >= 48)
  {
    kY = 48;
    kVelY = 0;
    isJumping = false;
  }

  // 2) Obstacle movement
  obsX -= obsSpeed;
  if (obsX < -obstacleW)
  {
    obsX = 128;
    kScore++;
    obsSpeed += 0.1;
  }

  // 3) Collision detection (AABB)
  if (obsX < (playerX + playerW - 1) &&
      (obsX + obstacleW) > (playerX + 1) &&
      (kY + playerH) > (obstacleY + 1))
  {
    isGameOver = true;
    if (kScore > kHighScore)
      kHighScore = kScore;
    currentMood = MOOD_SURPRISED;
  }

  // 4) Rendering
  display.drawLine(0, 56, 128, 56, SH110X_WHITE);

  const unsigned char *runnerSprite = sprite_korangu_run_1;
  if (isJumping)
  {
    runnerSprite = sprite_korangu_jump;
  }
  else if ((millis() / 120) % 2 == 0)
  {
    runnerSprite = sprite_korangu_run_2;
  }

  display.drawBitmap(playerX, (int)kY, runnerSprite, playerW, playerH, SH110X_WHITE);
  display.drawBitmap((int)obsX, obstacleY, sprite_cactus, obstacleW, obstacleH, SH110X_WHITE);

  display.setCursor(0, 0);
  display.print("HI: ");
  display.print(kHighScore);
  display.setCursor(100, 0);
  display.print(kScore);
}

void drawMenuOverlay()
{
  normalizeMenuSelection();
  int totalItems = getMenuItemCount();

  display.setFont(NULL);
  display.setTextSize(1);

  const int boxX = 2;
  const int boxY = 1;
  const int boxW = 124;
  const int boxH = 62;

  display.fillRoundRect(boxX, boxY, boxW, boxH, 5, SH110X_BLACK);
  display.drawRoundRect(boxX, boxY, boxW, boxH, 5, SH110X_WHITE);

  display.setTextColor(SH110X_WHITE);
  display.setCursor(8, 5);
  display.print(wsConnected ? "MENU (ONLINE)" : "MENU (OFFLINE)");
  display.drawFastHLine(6, 13, 116, SH110X_WHITE);

  const int visibleRows = 4;
  const int rowHeight = 10;
  int startIndex = menuSelection - (visibleRows / 2);
  if (startIndex < 0)
    startIndex = 0;

  int maxStart = totalItems - visibleRows;
  if (maxStart < 0)
    maxStart = 0;
  if (startIndex > maxStart)
    startIndex = maxStart;

  for (int row = 0; row < visibleRows; row++)
  {
    int index = startIndex + row;
    if (index >= totalItems)
      break;

    int y = 16 + (row * rowHeight);
    bool selected = (index == menuSelection);

    if (selected)
    {
      display.fillRoundRect(7, y - 1, 111, 9, 2, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    }
    else
    {
      display.setTextColor(SH110X_WHITE);
    }

    display.setCursor(10, y);
    display.print(getMenuLabelAt(index));

    if (selected && getMenuActionAt(index) == MENU_ACTION_BRIGHTNESS)
    {
      display.setCursor(92, y);
      display.print(highBrightness ? "ON" : "DIM");
    }
  }

  if (startIndex > 0)
    display.fillTriangle(116, 15, 120, 15, 118, 12, SH110X_WHITE);
  if ((startIndex + visibleRows) < totalItems)
    display.fillTriangle(116, 53, 120, 53, 118, 56, SH110X_WHITE);

  display.setTextColor(SH110X_WHITE);
  display.setCursor(9, 54);
  display.print("Tap=Scroll Hold=Select");
}

// ==================================================
// 5. BOOT & MAIN
// ==================================================

void playBootAnimation()
{
  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;
  display.setFont(&FreeSansBold9pt7b);
  String bootText = "KORANGU";

  // Calculate text width to center it perfectly
  int16_t x1, y1;
  uint16_t w, h_txt;
  display.getTextBounds(bootText, 0, 0, &x1, &y1, &w, &h_txt);
  int textX = cx - (w / 2);
  int textY = cy + 6; // Adjusted for FreeSans font baseline

  // PHASE 1: Center dot expands into a horizontal laser beam
  for (int w_line = 0; w_line <= SCREEN_WIDTH; w_line += 8)
  {
    display.clearDisplay();
    display.drawLine(cx - w_line / 2, cy, cx + w_line / 2, cy, SH110X_WHITE);
    display.display();
    delay(10);
  }

  // PHASE 2: Laser beam expands vertically into a solid glowing block
  for (int h_box = 0; h_box <= 32; h_box += 4)
  {
    display.clearDisplay();
    display.fillRect(0, cy - h_box / 2, SCREEN_WIDTH, h_box, SH110X_WHITE);
    display.display();
    delay(15);
  }

  // PHASE 3: The text punches through the block (Negative Space)
  display.setTextColor(SH110X_BLACK);
  display.clearDisplay();
  display.fillRect(0, cy - 16, SCREEN_WIDTH, 32, SH110X_WHITE);
  display.setCursor(textX, textY);
  display.print(bootText);
  display.display();
  delay(600); // Hold for impact

  // PHASE 4: Stylized Cyber-Glitch!
  for (int i = 0; i < 5; i++)
  {
    display.clearDisplay();
    display.fillRect(0, cy - 16, SCREEN_WIDTH, 32, SH110X_WHITE);

    // Shift text randomly to simulate a tracking glitch
    display.setCursor(textX + random(-3, 4), textY + random(-2, 3));
    display.print(bootText);

    // Draw random black horizontal slices cutting through the block
    int sliceY = random(cy - 12, cy + 10);
    display.fillRect(0, sliceY, SCREEN_WIDTH, random(2, 6), SH110X_BLACK);

    display.display();
    delay(40); // Fast, jarring frame rate
  }

  // PHASE 5: Sliding Door Reveal
  // The white block splits in the middle and slides off-screen, revealing the clean white text!
  display.setTextColor(SH110X_WHITE);
  for (int sweep = 0; sweep <= cx; sweep += 6)
  {
    display.clearDisplay();

    // 1. Draw the final white text in the background
    display.setCursor(textX, textY);
    display.print(bootText);

    // 2. Draw the retreating white blocks over the text to act as "sliding doors"
    // Left door sliding left
    display.fillRect(0, cy - 16, cx - sweep, 32, SH110X_WHITE);
    // Right door sliding right
    display.fillRect(cx + sweep, cy - 16, cx - sweep + 1, 32, SH110X_WHITE);

    display.display();
    delay(15); // Smooth slide
  }

  // Final hold on the clean logo
  display.clearDisplay();
  display.setCursor(textX, textY);
  display.print(bootText);
  display.display();
  delay(1000); // Give the user a second to admire it

  display.setFont(NULL); // Reset the font size so the "connecting..." text isn't massive
}

// ==================================================
// --- LYRICS HELPER FUNCTIONS ---
// ==================================================

// Calculates exact pixel width of a string at the current text size
int getLyricWidth(String text)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Safely word-wraps text based on pixel width to prevent screen cutoff
int wrapLyricLines(String text, String lines[], int maxLines, int maxWidth)
{
  int lineCount = 0;
  String currentLine = "";
  String remaining = text;

  while (remaining.length() > 0 && lineCount < maxLines)
  {
    int spaceIdx = remaining.indexOf(' ');
    String word = (spaceIdx == -1) ? remaining : remaining.substring(0, spaceIdx);

    String testLine = currentLine;
    if (testLine.length() > 0)
      testLine += " ";
    testLine += word;

    if (getLyricWidth(testLine) <= maxWidth)
    {
      currentLine = testLine;
      if (spaceIdx == -1)
      {
        lines[lineCount++] = currentLine;
        remaining = "";
        break;
      }
      remaining = remaining.substring(spaceIdx + 1);
    }
    else
    {
      if (currentLine.length() == 0)
      {
        // A single word is longer than the screen width; force truncate
        lines[lineCount++] = word.substring(0, max(1, (int)word.length() - 3)) + "...";
        if (spaceIdx == -1)
        {
          remaining = "";
          break;
        }
        remaining = remaining.substring(spaceIdx + 1);
      }
      else
      {
        lines[lineCount++] = currentLine;
        currentLine = "";
      }
    }
  }

  // If not all words fit in the requested line budget, mark truncation clearly.
  if (remaining.length() > 0 && lineCount > 0)
  {
    if (!lines[lineCount - 1].endsWith("..."))
    {
      String clipped = lines[lineCount - 1];
      while (clipped.length() > 0 && getLyricWidth(clipped + "...") > maxWidth)
      {
        clipped.remove(clipped.length() - 1);
        clipped.trim();
      }
      lines[lineCount - 1] = (clipped.length() > 0) ? (clipped + "...") : "...";
    }
  }

  return lineCount;
}

// ==================================================
// --- MAIN LYRICS RENDERER ---
// ==================================================

void drawBeautifulLyrics()
{
  unsigned long elapsed = millis() - lyricStartTime;
  unsigned long beat = millis() / 150; // Syncs monkey animation and accents

  String lyricToRender = currentLyric;
  lyricToRender.trim();
  if (lyricToRender.length() == 0)
    lyricToRender = "[music]";

  display.setFont(NULL);

  // Layout Boundaries
  const int panelW = 24;
  const int textPadLeft = 4;
  const int textAreaW = SCREEN_WIDTH - panelW - textPadLeft - 3;

  String lines[5];
  int textScale = 1;
  int lineCount = 0;
  int lineHeight = 9;
  int blockHeight = 0;

  // --- CASCADING FONT SIZE LOGIC ---

  // 1. Try HUGE Text (Size 3) for punchy 1-2 word drops
  String hugeLines[2];
  display.setTextSize(3);
  int hugeCount = wrapLyricLines(lyricToRender, hugeLines, 2, textAreaW);
  bool hugeTruncated = (hugeCount > 0 && hugeLines[hugeCount - 1].endsWith("..."));

  if (!hugeTruncated && hugeCount > 0 && hugeCount <= 2)
  {
    textScale = 3;
    lineCount = hugeCount;
    for (int i = 0; i < lineCount; i++)
      lines[i] = hugeLines[i];
    lineHeight = 26;
  }
  else
  {
    // 2. Try MEDIUM Text (Size 2) for standard phrases
    String midLines[3];
    display.setTextSize(2);
    int midCount = wrapLyricLines(lyricToRender, midLines, 3, textAreaW);
    bool midTruncated = (midCount > 0 && midLines[midCount - 1].endsWith("..."));

    if (!midTruncated && midCount > 0 && midCount <= 3)
    {
      textScale = 2;
      lineCount = midCount;
      for (int i = 0; i < lineCount; i++)
        lines[i] = midLines[i];
      lineHeight = 18;
    }
    else
    {
      // 3. Fallback to SMALL Text (Size 1) for long, wordy verses
      display.setTextSize(1);
      textScale = 1;
      lineCount = wrapLyricLines(lyricToRender, lines, 5, textAreaW);
      lineHeight = 9;
    }
  }

  // Apply winning text size and calculate block height for vertical centering
  display.setTextSize(textScale);
  blockHeight = lineCount * lineHeight;

  int baseY = (SCREEN_HEIGHT - blockHeight) / 2;
  if (baseY < 0)
    baseY = 0;

  // --- TRANSITIONS & EFFECTS ---
  int globalYOffset = 0;
  bool invertFlash = false;
  bool glitch = false;

  if (lyricTransitionType == 0)
  { // Pop
    if (elapsed < 220)
      globalYOffset = map((long)elapsed, 0L, 220L, -8L, 0L);
  }
  else if (lyricTransitionType == 1)
  { // Slide
    if (elapsed < 320)
      globalYOffset = map((long)elapsed, 0L, 320L, 18L, 0L);
  }
  else if (lyricTransitionType == 2)
  { // Flash
    invertFlash = (elapsed < 140);
  }
  else if (lyricTransitionType == 3)
  { // Glitch
    glitch = (elapsed < 260);
  }

  uint16_t fgColor = SH110X_WHITE;
  if (invertFlash)
  {
    display.fillScreen(SH110X_WHITE);
    fgColor = SH110X_BLACK;
  }
  display.setTextColor(fgColor);

  // --- DRAW DANCING MONKEY ---
  int panelX = SCREEN_WIDTH - panelW;

  // 4-Frame cycle
  int frame = beat % 4;
  const unsigned char *sideSprite = (frame == 0)   ? sprite_lyric_monkey_1
                                    : (frame == 1) ? sprite_lyric_monkey_2
                                    : (frame == 2) ? sprite_lyric_monkey_3
                                                   : sprite_lyric_monkey_4;

  int spriteY = 24 + ((millis() / 260) % 2); // Slight bobbing effect
  display.drawBitmap(panelX + 4, spriteY, sideSprite, 16, 16, fgColor);

  // --- DRAW TEXT ---
  for (int i = 0; i < lineCount; i++)
  {
    int revealOffset = 0;
    if (lyricTransitionType == 1)
    {
      unsigned long lineDelay = i * 70; // Staggered slide reveal per line
      if (elapsed < lineDelay)
      {
        revealOffset = 14;
      }
      else if (elapsed < lineDelay + 260)
      {
        revealOffset = map((long)(elapsed - lineDelay), 0L, 260L, 14L, 0L);
      }
    }

    int w = getLyricWidth(lines[i]);
    int x = textPadLeft + ((textAreaW - w) / 2);
    int y = baseY + (i * lineHeight) + globalYOffset + revealOffset;

    if (glitch)
    {
      x += random(-3, 4);
      if (random(0, 4) == 0)
      {
        display.drawFastHLine(0, random(0, SCREEN_HEIGHT), SCREEN_WIDTH, fgColor);
      }
    }

    display.setCursor(x, y);
    display.print(lines[i]);
  }

  // Reset standard values for next loops
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
}

const char *resetReasonToText(esp_reset_reason_t reason)
{
  switch (reason)
  {
  case ESP_RST_POWERON:
    return "POWERON";
  case ESP_RST_EXT:
    return "EXTERNAL_RESET";
  case ESP_RST_SW:
    return "SOFTWARE_RESTART";
  case ESP_RST_PANIC:
    return "PANIC_CRASH";
  case ESP_RST_INT_WDT:
    return "INT_WATCHDOG";
  case ESP_RST_TASK_WDT:
    return "TASK_WATCHDOG";
  case ESP_RST_WDT:
    return "OTHER_WATCHDOG";
  case ESP_RST_DEEPSLEEP:
    return "WAKE_FROM_SLEEP";
  case ESP_RST_BROWNOUT:
    return "BROWNOUT";
  case ESP_RST_SDIO:
    return "SDIO_RESET";
  default:
    return "UNKNOWN";
  }
}

void setup()
{
  Serial.begin(115200); // Turn on the serial monitor at 115200 speed
  delay(1000);          // Give the USB port 1 second to wake up
  Serial.println("\n\n--- Korangu Boot Sequence Started ---");
  esp_reset_reason_t rr = esp_reset_reason();
  Serial.print("Reset reason: ");
  Serial.print(resetReasonToText(rr));
  Serial.print(" (");
  Serial.print((int)rr);
  Serial.println(")");

  // --- TEMPORARY MEMORY WIPE (Delete after 1st boot!) ---
  // prefs.begin("deskbuddy", false);
  // prefs.clear();
  // prefs.end();
  // Serial.println("MEMORY COMPLETELY WIPED!");

  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(TOUCH_PIN, INPUT);
  display.begin(0x3C, true);
  display.setTextColor(SH110X_WHITE);

  // Hold touch for 3 sec at boot to force config mode
  bool forceConfig = false;
  for (unsigned long t = millis(); millis() - t < CONFIG_HOLD_MS;)
  {
    if (digitalRead(TOUCH_PIN))
    {
      forceConfig = true;
      break;
    }
    delay(80);
  }

  loadConfig();

  if (forceConfig)
  {
    startConfigPortal();
    return;
  }

  // Init Eyes Center
  leftEye.init(22, 10, 28, 28);
  rightEye.init(78, 10, 28, 28);

  playBootAnimation();

  display.clearDisplay();
  display.setFont(NULL);
  display.setCursor(40, 30);
  display.print("connecting");
  display.display();
  Serial.print("Attempting to connect to network: [");
  Serial.print(wifiSsid);
  Serial.println("]");

  // --- THE ULTIMATE BROWNOUT FIX ---
  WiFi.disconnect(true, true); // The second 'true' completely wipes corrupt memory!
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  // Throttle the Wi-Fi power draw to prevent crashes
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  // -----------------------------

  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 15000))
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println(); // Print a new line

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("SUCCESS! Connected to Wi-Fi. IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("CRITICAL FAILURE! Could not connect to Wi-Fi.");
    Serial.print("ESP32 Wi-Fi Error Code: ");
    Serial.println(WiFi.status());
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    startConfigPortal();
    return;
  }

  configTime(0, 0, ntpServer);
  setenv("TZ", tzString.c_str(), 1);
  tzset();
  setWeatherRuntimeState(WEATHER_STATE_LOADING, "Syncing...", WEATHER_UPDATE_RETRY_MS, false);
  startWeatherWorker();
  lastPageSwitch = millis();

  udp.begin(4210);
}

void loop()
{
  if (inConfigMode)
  {
    configServer.handleClient();
    return;
  }
  unsigned long now = millis();
  statLoopCount++;

  runWiFiWatchdog();
  establishTelepathicLink();
  handleTouch();

  if (!wsConnected && (currentPage == 5 || currentPage == 6))
  {
    transitionToPage(0, true);
  }

  // Returns to face after 5 seconds on utility pages.
  // Lyrics (5), Cyberdeck (6), and Game (7) stay sticky until user changes screen.
  if (!inMenuOverlay && currentPage != 0 && currentPage != 5 && currentPage != 6 && currentPage != 7 && (now - lastPageSwitch > 5000))
  {
    transitionToPage(0, true);
  }

  reportCurrentPageToServer();
  flushPendingPageStateResync();
  logRuntimeTelemetry();

  display.clearDisplay();
  switch (currentPage)
  {
  case 0:
    drawEmoPage();
    break;
  case 1:
    drawClock();
    break;
  case 2:
    drawWeatherCard();
    break;
  case 3:
    drawWorldClock();
    break;
  case 4:
    drawForecastPage();
    break;
  case 5:
    drawBeautifulLyrics();
    break; // NEW LYRICS RENDERER!
  case 6:
    drawCyberdeckPage();
    break;
  case 7:
    drawKoranguRun();
    break;
  }

  if (inMenuOverlay)
    drawMenuOverlay();

  display.display();
}
