#include <Arduino.h>

// --- Конфігурація (Завдання 2) ---
struct Config {
    static constexpr uint8_t ledPin = 4;           // Пін світлодіода 
    static constexpr uint8_t buttonPin = 0;        // Пін вбудованої кнопки BOOT
    static constexpr uint32_t blinkInterval = 500; // Інтервал блимання, мс
    static constexpr uint32_t debounceDelay = 50;  // Антидребезг, мс
};

// --- Строгі типи ---
enum class LedState { On, Off };
enum class Mode { Blinking, AlwaysOn, AlwaysOff };

// --- Клас Led (Завдання 1) ---
class Led {
public:
    void init() {
        pinMode(Config::ledPin, OUTPUT);
        set(LedState::Off);
    }

    void set(LedState state) {
        digitalWrite(Config::ledPin, state == LedState::On ? HIGH : LOW);
        currentState = state;
    }

    LedState get() const { 
        return currentState; 
    }

    void toggle() {
        set(currentState == LedState::On ? LedState::Off : LedState::On);
    }

private:
    LedState currentState = LedState::Off;
};

// --- Глобальні змінні ---
Led led;
Mode currentMode = Mode::AlwaysOff;

uint32_t lastToggleTime = 0;
uint32_t lastButtonPressTime = 0;

// Змінні для вимірювання часу виконання (Завдання 3)
uint32_t loopSumTime = 0;
uint32_t loopCount = 0;

// Прапорець переривання (Завдання 4)
volatile bool buttonPressed = false;

// --- ISR (Обробник переривання) ---
// IRAM_ATTR обов'язковий для ESP32, щоб функція знаходилась в оперативній пам'яті
void IRAM_ATTR buttonISR() {
    buttonPressed = true;
}

void setup() {
    Serial.begin(115200);
    
    led.init();
    
    // Налаштування кнопки
    pinMode(Config::buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Config::buttonPin), buttonISR, FALLING);
}

void loop() {
    uint32_t loopStart = micros(); // Початок вимірювання часу
    uint32_t now = millis();

    // 1. Обробка кнопки та перемикання режимів
    if (buttonPressed) {
        buttonPressed = false; // Одразу скидаємо прапорець
        
        // Антидребезг (debouncing)
        if (now - lastButtonPressTime >= Config::debounceDelay) {
            lastButtonPressTime = now;
            
            // Зміна режиму по колу
            switch (currentMode) {
                case Mode::Blinking:  currentMode = Mode::AlwaysOn;  break;
                case Mode::AlwaysOn:  currentMode = Mode::AlwaysOff; break;
                case Mode::AlwaysOff: currentMode = Mode::Blinking;  break;
            }
        }
    }

    // 2. Обробка стану LED згідно з поточним режимом (Superloop)
    switch (currentMode) {
        case Mode::AlwaysOn:
            if (led.get() != LedState::On) led.set(LedState::On);
            break;
            
        case Mode::AlwaysOff:
            if (led.get() != LedState::Off) led.set(LedState::Off);
            break;
            
        case Mode::Blinking:
            if (now - lastToggleTime >= Config::blinkInterval) {
                lastToggleTime = now;
                led.toggle();
            }
            break;
    }

    // 3. Вимірювання часу виконання (Завдання 3)
    uint32_t loopEnd = micros();
    loopSumTime += (loopEnd - loopStart);
    loopCount++;

    if (loopCount >= 1000) {
        Serial.print("Середній час ітерації loop() [мкс]: ");
        Serial.println((float)loopSumTime / 1000.0);
        
        loopCount = 0;
        loopSumTime = 0;
    }
}