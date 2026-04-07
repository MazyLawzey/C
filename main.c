#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "hardware/gpio.h"

// Ваши пины
#define BTN_PIN 24
#define LED_PIN 25

// Переменные для моргания от TinyUSB
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);

// Функция перевода ASCII-символа в HID-код клавиши
// (поддерживает строчные буквы, цифры, пробел и слэш)
uint8_t char_to_hid_key(char c) {
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    if (c == '0') return HID_KEY_0;
    if (c == ' ') return HID_KEY_SPACE;
    if (c == '/') return HID_KEY_SLASH;
    return 0; // Неизвестный символ
}

// Вспомогательная функция для нажатия одной клавиши с модификаторами
void press_key(uint8_t modifier, uint8_t key) {
    uint8_t keycode[6] = {0};
    keycode[0] = key;
    
    // Ждем, пока USB будет готов к отправке
    while(!tud_hid_ready()) {
        tud_task();
    }
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
    board_delay(30);

    // Отпускаем клавишу
    while(!tud_hid_ready()) {
        tud_task();
    }
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    board_delay(30);
}

// Функция для набора целой строки текста
void type_string(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        uint8_t key = char_to_hid_key(str[i]);
        if (key != 0) {
            press_key(0, key);
        }
    }
}

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // Инициализация ваших GPIO
  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_pull_up(BTN_PIN);

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0); // Выключаем при старте

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

void tud_mount_cb(void) { blink_interval_ms = BLINK_MOUNTED; }
void tud_umount_cb(void) { blink_interval_ms = BLINK_NOT_MOUNTED; }
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}
void tud_resume_cb(void) { blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED; }

//--------------------------------------------------------------------+
// USB HID Task
//--------------------------------------------------------------------+

// ... весь код сверху остается прежним ...

void hid_task(void)
{
  // Опрашиваем кнопку каждые 10мс (антидребезг)
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;
  static bool action_done = false;

  if ( board_millis() - start_ms < interval_ms) return; 
  start_ms += interval_ms;

  // Читаем кнопку (0 - нажата, из-за pull-up)
  bool btn_pressed = (gpio_get(BTN_PIN) == 0);

  if (btn_pressed && !action_done && tud_hid_ready())
  {
    // 1. Включаем наш светодиод
    gpio_put(LED_PIN, 1);

    // 2. Нажимаем Win + R (Left GUI + R) для вызова окна "Выполнить" в Windows
    press_key(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);
    
    // Ждем полсекунды, пока Windows гарантированно откроет окно
    board_delay(500); 

    // 3. Пишем команду выключения. 
    // (/s - выключить, /t 0 - таймер 0 секунд)
    type_string("shutdown /s /t 0");
    board_delay(100);

    // 4. Нажимаем Enter для выполнения команды
    press_key(0, HID_KEY_ENTER);

    action_done = true; // Блокируем повторные отправки
    
    // Выключаем светодиод после завершения
    gpio_put(LED_PIN, 0);
  }
  else if (!btn_pressed && action_done)
  {
    // Кнопка отпущена, можно сбросить флаг
    action_done = false;
  }
}

// ... остальной код внизу без изменений ...


// Invoked when sent REPORT successfully to host
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance; (void) len; (void) report;
  // Оставляем пустым, так как мы управляем отправкой вручную через press_key()
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // Оставлено для светодиодов самой клавиатуры (Caps Lock)
}

void led_blinking_task(void)
{
  // Это встроенное моргание от TinyUSB, можно отключить, если мешает
  static uint32_t start_ms = 0;
  static bool led_state = false;

  if (!blink_interval_ms) return;
  if ( board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;
}
