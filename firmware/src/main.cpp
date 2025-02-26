#include <lilka.h>
#include <Wire.h>
#include <radio.h>
#include <RDA5807M.h>
#include <Preferences.h>

#define SDA_PIN 13
#define SCL_PIN 12
#define DISPLAY_WIDTH 280
#define DISPLAY_HEIGHT 240
#define MIN_FREQUENCY 8750
#define MAX_FREQUENCY 10800
#define MIN_VOLUME 1
#define MAX_VOLUME 15
#define GET_RADIO_INFO_INTERVAL 2000
#define SAVE_SETTINGS_DELAY 3000

lilka::Canvas canvas;
RDA5807M radio;

Preferences settings;
unsigned long lastSettingsChangeTime = 0;
bool pendingSave = false;

unsigned long nextGetRadioInfoTime = 0;

int frequency = 10000;
int volume = 1;
int signalStrength = 0;
bool isMute = false;

void initRadioModule();
void drawTitle();
void drawFrequency();
void drawSignalStrength();
void drawVolumeLevel();
void drawFrequencyScale();
void drawBatteryLevel();
void drawScreen();
void setupButtons();
void handleLeftButton(lilka::ButtonState button);
void handleRightButton(lilka::ButtonState button);
void handleUpButton(lilka::ButtonState button);
void handleDownButton(lilka::ButtonState button);
void handleSelectButton(lilka::ButtonState button);
void handleAButton(lilka::ButtonState button);
void handleDButton(lilka::ButtonState button);
void handleGetRadioInfo();
void loadSettings();
void saveSettings();
int convertRssiToSignalStrength(int rssi);

void setup()
{
  lilka::begin();
  loadSettings();
  setupButtons();
  initRadioModule();
}

void loop()
{
  lilka::State state = lilka::controller.getState();

  handleGetRadioInfo();
  handleLeftButton(state.left);
  handleRightButton(state.right);
  handleUpButton(state.up);
  handleDownButton(state.down);
  handleSelectButton(state.select);
  handleAButton(state.a);
  handleDButton(state.d);

  if (pendingSave && (millis() - lastSettingsChangeTime > SAVE_SETTINGS_DELAY))
  {
    saveSettings();
    pendingSave = false;
  }

  drawScreen();
}

void initRadioModule()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);
  radio.initWire(Wire);
  radio.setBandFrequency(RADIO_BAND_FM, frequency);
  radio.setVolume(volume);
  radio.setMono(false);
  radio.setMute(isMute);
}

int convertRssiToSignalStrength(int rssi)
{
  // Map RSSI from 0 to 45 into 0-5 strength range
  int range = map(rssi, 0, 45, 0, 5);
  return constrain(range, 0, 5);
}

void drawTitle()
{
  canvas.setCursor(75, 25);
  canvas.setTextColor(lilka::colors::White);
  canvas.setFont(FONT_8x13_MONO);
  canvas.setTextSize(2);
  canvas.print("FM Radio");
}

void drawFrequency()
{
  canvas.setCursor(frequency < 10000 ? 80 : 70, 130);
  canvas.setTextColor(lilka::colors::White);
  canvas.setFont(u8g2_font_UnnamedDOSFontIV_tr);
  canvas.setTextSize(3);
  canvas.print(frequency / 100.0, 1);
}

void drawSignalStrength()
{
  canvas.setCursor(220, 45);
  canvas.setTextColor(lilka::colors::White);
  canvas.setFont(FONT_8x13_MONO);
  canvas.setTextSize(1);
  canvas.print("Signal");

  int initialBarHeight = 3;
  int barWidth = 4;
  int barSpacing = 2;
  int baseX = 230;
  int baseY = 65;

  for (int i = 0; i < 5; i++)
  {
    int barHeight = initialBarHeight * (i + 1);
    int x = baseX + i * (barWidth + barSpacing);
    if (i < signalStrength)
    {
      canvas.fillRect(x, baseY - barHeight, barWidth, barHeight, lilka::colors::Green);
    }
    else
    {
      canvas.drawRect(x, baseY - barHeight, barWidth, barHeight, lilka::colors::Green);
    }
  }
}

void drawVolumeLevel()
{
  canvas.setCursor(220, 80);
  canvas.setTextColor(lilka::colors::White);
  canvas.setFont(FONT_8x13_MONO);
  canvas.setTextSize(1);
  isMute ? canvas.print("Vol: 0") : canvas.print("Vol: " + String(volume));
}

void drawFrequencyScale()
{
  int tempFrequency = (frequency / 10) - 20;
  for (int i = 0; i < 40; i++)
  {
    if ((tempFrequency % 10) == 0)
    {
      canvas.drawLine(i * 7, 210, i * 7, DISPLAY_HEIGHT, lilka::colors::Yellow);
      canvas.drawLine((i * 7) + 1, 210, (i * 7) + 1, DISPLAY_HEIGHT, lilka::colors::Yellow);
      canvas.setFont(FONT_8x13_MONO);
      canvas.setCursor(i * 7 - 15, 205);
      canvas.setTextColor(lilka::colors::White);
      canvas.setTextWrap(false);
      canvas.print(tempFrequency / 10.0, 1);
    }
    else if ((tempFrequency % 5) == 0 && (tempFrequency % 10) != 0)
    {
      canvas.drawLine(i * 7, 220, i * 7, DISPLAY_HEIGHT, lilka::colors::Yellow);
    }
    else
    {
      canvas.drawLine(i * 7, 230, i * 7, DISPLAY_HEIGHT, lilka::colors::Yellow);
    }
    tempFrequency++;
  }

  canvas.fillTriangle(135, 180, 145, 180, 140, 185, lilka::colors::Red);
  canvas.drawLine(140, 185, 140, DISPLAY_HEIGHT, lilka::colors::Red);
}

void drawBatteryLevel()
{
  int batteryLevel = lilka::battery.readLevel();

  int batteryWidth = 25;
  int batteryHeight = 10;
  int paddingRight = 25;
  int paddingTop = 14;

  int x = DISPLAY_WIDTH - batteryWidth - paddingRight;
  int y = paddingTop;

  canvas.drawRect(x, y, batteryWidth, batteryHeight, lilka::colors::White);

  canvas.fillRect(x + batteryWidth, y + batteryHeight / 5, 3, batteryHeight / 2, lilka::colors::White);

  int fillWidth = map(batteryLevel, 0, 100, 0, batteryWidth - 2);

  canvas.fillRect(x + 1, y + 1, fillWidth, batteryHeight - 2, lilka::colors::Green);
}

void drawScreen()
{
  canvas.fillScreen(lilka::colors::Black);

  drawTitle();
  drawFrequency();
  drawSignalStrength();
  drawVolumeLevel();
  drawFrequencyScale();
  drawBatteryLevel();

  lilka::display.drawCanvas(&canvas);
}

void setupButtons()
{
  for (lilka::Button button : {lilka::Button::UP, lilka::Button::DOWN, lilka::Button::LEFT, lilka::Button::RIGHT})
  {
    lilka::controller.setAutoRepeat(button, 10, 300);
  }
}

void handleLeftButton(lilka::ButtonState button)
{
  if (button.justPressed)
  {
    frequency -= 10;
    if (frequency < MIN_FREQUENCY)
    {
      frequency = MIN_FREQUENCY;
    }
    radio.setFrequency(frequency);

    if (isMute)
    {
      radio.setMute(isMute);
    }

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleRightButton(lilka::ButtonState button)
{
  if (button.justPressed)
  {
    frequency += 10;
    if (frequency > MAX_FREQUENCY)
    {
      frequency = MAX_FREQUENCY;
    }
    radio.setFrequency(frequency);

    if (isMute)
    {
      radio.setMute(isMute);
    }

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleUpButton(lilka::ButtonState button)
{
  if (button.justPressed && !isMute)
  {
    volume += 1;
    if (volume > MAX_VOLUME)
    {
      volume = MAX_VOLUME;
    }
    radio.setVolume(volume);

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleDownButton(lilka::ButtonState button)
{
  if (button.justPressed && !isMute)
  {
    volume -= 1;
    if (volume < MIN_VOLUME)
    {
      volume = MIN_VOLUME;
    }
    radio.setVolume(volume);

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleSelectButton(lilka::ButtonState button)
{
  if (button.justPressed)
  {
    isMute = !isMute;
    radio.setMute(isMute);
  }
}

void handleAButton(lilka::ButtonState button)
{
  if (button.justPressed)
  {
    radio.seekUp();
    delay(250);
    frequency = radio.getFrequency();

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleDButton(lilka::ButtonState button)
{
  if (button.justPressed)
  {
    radio.seekDown();
    delay(250);
    frequency = radio.getFrequency();

    lastSettingsChangeTime = millis();
    pendingSave = true;
  }
}

void handleGetRadioInfo()
{
  unsigned long now = millis();
  if (now > nextGetRadioInfoTime)
  {
    RADIO_INFO info;
    radio.getRadioInfo(&info);
    signalStrength = convertRssiToSignalStrength(info.rssi);
    nextGetRadioInfoTime = now + GET_RADIO_INFO_INTERVAL;
  }
}

void loadSettings()
{
  settings.begin("lilradio", true);
  frequency = settings.getInt("frequency", frequency);
  volume = settings.getInt("volume", volume);
  settings.end();
}

void saveSettings()
{
  settings.begin("lilradio", false);
  settings.putInt("frequency", frequency);
  settings.putInt("volume", volume);
  settings.end();
}
