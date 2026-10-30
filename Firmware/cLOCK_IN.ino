#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include <Preferences.h>

const char* WIFI_SSID = "";            // leave "" to skip WiFi and set the time by hand
const char* WIFI_PASS = "";
const char* TZ_INFO   = "IST-5:30";    // time zone (this is India)
const bool  ACTIVE_BUZZER = false;     // true if the buzzer beeps when simply given 3.3V
const int   BUZZ_FREQ = 2000;
const int   SNOOZE_MINUTES = 5;
const unsigned long RING_TIMEOUT_MS = 5UL * 60UL * 1000UL;

#define TFT_SCLK D9   // labeled SCL on the screen
#define TFT_MOSI D10  // labeled SDA on the screen
#define TFT_RST  D8
#define TFT_DC   D4
#define TFT_CS   D5
#define TFT_BL   D6

#define BTN_UP     D0
#define BTN_DOWN   D1
#define BTN_ENTER  D2
#define BTN_BACK   D3
#define BUZZER     D7

// Needed to expose setOffsets() for the 284x76 panel
class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
GFXcanvas16 canvas(284, 76);
Preferences prefs;

#define GREY 0x7BEF

enum State { HOME, SET_ALARM, SET_TIME, RINGING };
State state = HOME;

int  alarmH = 7, alarmM = 0;
bool alarmOn = false;
int  editH = 0, editM = 0, field = 0;
bool editOn = false;

time_t snoozeUntil = 0;
long   lastTrigKey = -1;
unsigned long ringStart = 0;
bool buzzing = false;
bool dirty = true;
int  lastSec = -1;
bool lastFlash = false;

enum { B_UP, B_DOWN, B_ENTER, B_BACK };
const uint8_t PINS[4] = {BTN_UP, BTN_DOWN, BTN_ENTER, BTN_BACK};
bool rawLast[4] = {false, false, false, false};
bool stable[4]  = {false, false, false, false};
bool evt[4]     = {false, false, false, false};
unsigned long chg[4] = {0, 0, 0, 0};
unsigned long nextRep[4] = {0, 0, 0, 0};

void pollButtons() {
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    bool r = (digitalRead(PINS[i]) == LOW);   // pressed = LOW
    evt[i] = false;
    if (r != rawLast[i]) { rawLast[i] = r; chg[i] = now; }
    if (now - chg[i] >= 30 && r != stable[i]) {
      stable[i] = r;
      if (r) { evt[i] = true; nextRep[i] = now + 500; }
    } else if (stable[i] && (i == B_UP || i == B_DOWN) && now >= nextRep[i]) {
      evt[i] = true;
      nextRep[i] = now + 120;
    }
  }
}

void buzzerInit() {
  pinMode(BUZZER, OUTPUT);
  if (!ACTIVE_BUZZER) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(BUZZER, BUZZ_FREQ, 8);
#else
    ledcSetup(0, BUZZ_FREQ, 8);
    ledcAttachPin(BUZZER, 0);
#endif
  }
}

void buzz(bool on) {
  if (on == buzzing) return;
  buzzing = on;
  if (ACTIVE_BUZZER) { digitalWrite(BUZZER, on ? HIGH : LOW); return; }
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(BUZZER, on ? BUZZ_FREQ : 0);
#else
  ledcWriteTone(0, on ? BUZZ_FREQ : 0);
#endif
}

void getNow(struct tm &t) {
  time_t n = time(nullptr);
  localtime_r(&n, &t);
}

void setClock(int h, int m) {
  struct tm t;
  getNow(t);
  t.tm_hour = h; t.tm_min = m; t.tm_sec = 0;
  struct timeval tv = { mktime(&t), 0 };
  settimeofday(&tv, nullptr);
}

void showMsg(const char* msg) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.print(msg);
}

bool syncNTP() {
  if (strlen(WIFI_SSID) == 0) return false;
  showMsg("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long s = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - s < 12000) delay(250);
  if (WiFi.status() != WL_CONNECTED) { WiFi.mode(WIFI_OFF); return false; }
  showMsg("Getting time...");
  configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
  struct tm t;
  bool ok = getLocalTime(&t, 10000);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  return ok;
}

void startRing() {
  state = RINGING;
  ringStart = millis();
  lastFlash = false;
  dirty = true;
}

void stopRing() {
  buzz(false);
  state = HOME;
  dirty = true;
}

void checkAlarm() {
  if (state == RINGING) return;
  time_t n = time(nullptr);
  struct tm t;
  localtime_r(&n, &t);
  if (snoozeUntil && n >= snoozeUntil) {
    snoozeUntil = 0;
    startRing();
    return;
  }
  if (alarmOn && t.tm_hour == alarmH && t.tm_min == alarmM) {
    long key = t.tm_yday * 1440L + t.tm_hour * 60 + t.tm_min;
    if (key != lastTrigKey) { lastTrigKey = key; startRing(); }
  }
}

void updateRingSound() {
  if (state != RINGING) return;
  unsigned long el = millis() - ringStart;
  if (el > RING_TIMEOUT_MS) { stopRing(); return; }
  unsigned long p = el % 1000;
  bool on = (p < 150) || (p >= 250 && p < 400) || (p >= 500 && p < 650);
  buzz(on);
}

void saveAlarm() {
  prefs.putInt("h", alarmH);
  prefs.putInt("m", alarmM);
  prefs.putBool("on", alarmOn);
}

void openSetAlarm() {
  editH = alarmH; editM = alarmM; editOn = alarmOn; field = 0;
  state = SET_ALARM;
}

void openSetTime() {
  struct tm t; getNow(t);
  editH = t.tm_hour; editM = t.tm_min; field = 0;
  state = SET_TIME;
}

void adjust(int dir, bool withOnOff) {
  if (field == 0)      editH = (editH + dir + 24) % 24;
  else if (field == 1) editM = (editM + dir + 60) % 60;
  else if (withOnOff)  editOn = !editOn;
}

void handleInput() {
  bool any = evt[0] || evt[1] || evt[2] || evt[3];
  if (!any) return;
  dirty = true;

  switch (state) {
    case HOME:
      if (evt[B_UP]) {
        alarmOn = !alarmOn;
        if (!alarmOn) snoozeUntil = 0;
        saveAlarm();
      }
      if (evt[B_ENTER]) openSetAlarm();
      if (evt[B_DOWN])  openSetTime();
      break;

    case SET_ALARM:
      if (evt[B_UP])   adjust(+1, true);
      if (evt[B_DOWN]) adjust(-1, true);
      if (evt[B_ENTER]) {
        if (++field > 2) {
          alarmH = editH; alarmM = editM; alarmOn = editOn;
          snoozeUntil = 0;
          lastTrigKey = -1;
          saveAlarm();
          state = HOME;
        }
      }
      if (evt[B_BACK]) state = HOME;
      break;

    case SET_TIME:
      if (evt[B_UP])   adjust(+1, false);
      if (evt[B_DOWN]) adjust(-1, false);
      if (evt[B_ENTER]) {
        if (++field > 1) { setClock(editH, editM); state = HOME; }
      }
      if (evt[B_BACK]) state = HOME;
      break;

    case RINGING:
      if (evt[B_BACK]) {
        snoozeUntil = time(nullptr) + SNOOZE_MINUTES * 60;
        stopRing();
      } else if (evt[B_ENTER]) {
        snoozeUntil = 0;
        stopRing();
      }
      break;
  }
}

void flush() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());
}

void drawHome(const struct tm &t) {
  char b[24];
  canvas.fillScreen(ST77XX_BLACK);

  canvas.setTextSize(6);
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(8, 4);
  snprintf(b, sizeof b, "%02d:%02d", t.tm_hour, t.tm_min);
  canvas.print(b);

  canvas.setTextSize(3);
  canvas.setTextColor(ST77XX_CYAN);
  canvas.setCursor(210, 29);
  snprintf(b, sizeof b, "%02d", t.tm_sec);
  canvas.print(b);

  canvas.setTextSize(2);
  canvas.setCursor(8, 58);
  if (snoozeUntil) {
    struct tm s; localtime_r(&snoozeUntil, &s);
    canvas.setTextColor(ST77XX_YELLOW);
    snprintf(b, sizeof b, "SNOOZE %02d:%02d", s.tm_hour, s.tm_min);
  } else if (alarmOn) {
    canvas.setTextColor(ST77XX_GREEN);
    snprintf(b, sizeof b, "ALARM %02d:%02d", alarmH, alarmM);
  } else {
    canvas.setTextColor(GREY);
    snprintf(b, sizeof b, "ALARM OFF");
  }
  canvas.print(b);
}

void drawEdit(const char* title, bool withOnOff) {
  char b[8];
  canvas.fillScreen(ST77XX_BLACK);

  canvas.setTextSize(2);
  canvas.setTextColor(ST77XX_CYAN);
  canvas.setCursor(0, 0);
  canvas.print(title);

  canvas.setTextSize(5);
  snprintf(b, sizeof b, "%02d", editH);
  canvas.setTextColor(field == 0 ? ST77XX_YELLOW : ST77XX_WHITE);
  canvas.setCursor(8, 22);
  canvas.print(b);

  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(68, 22);
  canvas.print(":");

  snprintf(b, sizeof b, "%02d", editM);
  canvas.setTextColor(field == 1 ? ST77XX_YELLOW : ST77XX_WHITE);
  canvas.setCursor(98, 22);
  canvas.print(b);

  if (withOnOff) {
    canvas.setTextSize(4);
    canvas.setTextColor(field == 2 ? ST77XX_YELLOW : ST77XX_WHITE);
    canvas.setCursor(190, 27);
    canvas.print(editOn ? "ON" : "OFF");
  }

  int ux = (field == 0) ? 8 : (field == 1 ? 98 : 190);
  int uw = (field == 2) ? (editOn ? 48 : 72) : 60;
  canvas.fillRect(ux, 63, uw, 3, ST77XX_YELLOW);

  canvas.setTextSize(1);
  canvas.setTextColor(GREY);
  canvas.setCursor(0, 68);
  canvas.print("UP/DN:change  ENTER:next  BACK:cancel");
}

void drawRing(const struct tm &t, bool flash) {
  char b[8];
  canvas.fillScreen(flash ? ST77XX_RED : ST77XX_BLACK);
  canvas.setTextColor(ST77XX_WHITE);

  canvas.setTextSize(5);
  canvas.setCursor(22, 4);
  canvas.print("WAKE UP!");

  canvas.setTextSize(2);
  canvas.setCursor(8, 48);
  snprintf(b, sizeof b, "%02d:%02d", t.tm_hour, t.tm_min);
  canvas.print(b);

  canvas.setTextSize(1);
  canvas.setCursor(110, 52);
  canvas.print("BACK:snooze  ENTER:stop");
}

void render() {
  struct tm t; getNow(t);

  if (t.tm_sec != lastSec) {
    lastSec = t.tm_sec;
    if (state == HOME || state == RINGING) dirty = true;
  }
  bool flash = false;
  if (state == RINGING) {
    flash = ((millis() - ringStart) / 500) % 2;
    if (flash != lastFlash) { lastFlash = flash; dirty = true; }
  }
  if (!dirty) return;
  dirty = false;

  switch (state) {
    case HOME:      drawHome(t); break;
    case SET_ALARM: drawEdit("SET ALARM", true); break;
    case SET_TIME:  drawEdit("SET TIME", false); break;
    case RINGING:   drawRing(t, flash); break;
  }
  flush();
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) pinMode(PINS[i], INPUT_PULLUP);
  buzzerInit();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);   // this panel's backlight is active LOW

  tft.init(76, 284);
  tft.setOffsets(82, 18);
  tft.invertDisplay(false);
  tft.setRotation(1);          // use 3 if the screen is upside down
  tft.fillScreen(ST77XX_BLACK);
  canvas.setTextWrap(false);
  Serial.println("TFT Initialized!");

  prefs.begin("alarm", false);
  alarmH  = prefs.getInt("h", 7);
  alarmM  = prefs.getInt("m", 0);
  alarmOn = prefs.getBool("on", false);

  setenv("TZ", TZ_INFO, 1);
  tzset();
  setClock(12, 0);

  bool synced = syncNTP();
  if (!synced) openSetTime();

  buzz(true); delay(80); buzz(false);
  dirty = true;
}

void loop() {
  pollButtons();
  checkAlarm();
  handleInput();
  updateRingSound();
  render();
}
