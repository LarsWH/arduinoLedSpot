#include <Arduino.h>
#include <EEPROM.h>
#define EEPROM_ADDR_OF_VAL 0
#define PWM_PIN  3
#define PUSH_BUTTON_PIN 5

#define VAL_MIN 0
#define VAL_MAX 255
#define STEPS (VAL_MAX-VAL_MIN+1)
#define TICK_TIME 10
#define PRELL_TIME 50
#define PRELL_TICKS (PRELL_TIME/TICK_TIME)

typedef enum Event {
	EVENT_NOTHING,
	EVENT_KEY_PRESS,
	EVENT_KEY_RELEASE,
	EVENT_TIMER_TICK
} EventType;

typedef enum KeyState {
	KEY_STATE_RELEASED,
	KEY_STATE_BEING_PRESSED,
	KEY_STATE_PRESSED,
	KEY_STATE_BEING_RELEASED,
} KeyStateType;

typedef enum RegulateState {
	IDLE,
	INCREMENT,
	DECREMENT
} RegulateStateType;


//---------- Global variables ----------
#define FLOAT_MAX 4            //  max*max*max*max = 256
#define FLOAT_MIN 1            //  min*min*min*min = 1
#define FLOAT_RANGE (FLOAT_MAX - FLOAT_MIN)

int val = VAL_MIN;
bool isIncrementing = true;


//---------- Prototypes ----------
EventType keyDetect();
void rampUpLightAfterPowerOn(int value);
void regulate(EventType event);


void setup()
{
	Serial.begin(115200);

	analogWrite(PWM_PIN, 0);
	pinMode(PWM_PIN, OUTPUT);
	analogWrite(PWM_PIN, 0);

	pinMode(PUSH_BUTTON_PIN, INPUT);
	digitalWrite(PUSH_BUTTON_PIN, HIGH);

	Serial.println("Initialized");
	int value = EEPROM.read(EEPROM_ADDR_OF_VAL);

	if (value < VAL_MIN) {
		value = VAL_MIN;
	} 
	if (value > VAL_MAX) {
		value = VAL_MAX;
	} 
	rampUpLightAfterPowerOn(value);
}

void loop()
{
	static long l = 0;

	EventType event = keyDetect();

	if (event == EVENT_NOTHING) {
		if (l % 4) {
			regulate(EVENT_TIMER_TICK);
		}
	} else {
		regulate(event);
	}
	l++;
	delay(TICK_TIME);
}

/**
The PWM has a dynamic range of 256 values (0 to 255), so regulation in 256 steps makes sense.

These enteger values 'val' from 0 to 255 are mapped to floating point values in the range 1 to 4 and used in this
polynomia: f(x) = x^4 to get a usable regulation of the light
*/
void writeLed() {
	Serial.print("  val:");
	Serial.print(val);

	float scaled = (((float)val * FLOAT_RANGE) / (float)STEPS) + FLOAT_MIN;
	Serial.print("  scaled:");
	Serial.print(scaled);

	float pwmVal = scaled * scaled * scaled * scaled;
	Serial.print("  pwmVal:");
	Serial.print(pwmVal);

	int pwmInt = (int)pwmVal;
	if (pwmInt > VAL_MAX) {
		pwmInt = VAL_MAX;
	}
	Serial.print(" PWM:");
	Serial.println(pwmInt);

	analogWrite(PWM_PIN, pwmInt);
}


void rampUpLightAfterPowerOn(int value) {
	int valuesToRamp = value - VAL_MIN;
	for (int i = VAL_MIN; i <= value; i++) {
		val = i;
		writeLed();
		delay(40);
	}
}

bool incrementLight() {
	bool incremented = false;
	if (val < VAL_MAX) {
		val++;
		incremented = true;
	}
	writeLed();
	return incremented;
}


bool decrementLight() {
	bool decremented = false;
	if (val > VAL_MIN) {
		val--;
		decremented = true;
	}
	writeLed();
	return decremented;
}


void toggleIncrementDecrement() {
	isIncrementing = !isIncrementing;
}


void regulate(EventType event) {
	static RegulateStateType state = IDLE;

	switch (event) 	{
		case EVENT_KEY_RELEASE:
			toggleIncrementDecrement();
			state = IDLE;
			writeLed();
			EEPROM.update(EEPROM_ADDR_OF_VAL, val); // Limit the number of EEPROM writes (=only when the key is released)
			break;
	}

	switch (state) {
		case IDLE:
			switch (event) {
				case EVENT_KEY_PRESS:
					if (isIncrementing) {
						state = INCREMENT;
					} else {
						state = DECREMENT;
					}
					break;
				default:
					break;
			}
			break;

		case INCREMENT:
			switch (event) {
				case EVENT_TIMER_TICK:
					if (!incrementLight()) {
						isIncrementing = false;
						state = DECREMENT;
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case DECREMENT:
			switch (event) {
				case EVENT_TIMER_TICK:
					if (!decrementLight()) {
						isIncrementing = true;
						state = INCREMENT;
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

	}

}

EventType keyDetect() {
	EventType event = EVENT_NOTHING;
	static KeyStateType keyState = KEY_STATE_RELEASED;
	static int prellDetects = PRELL_TICKS;
	int buttonDown = !digitalRead(PUSH_BUTTON_PIN);

	switch (keyState) {
		case KEY_STATE_RELEASED:
			if (buttonDown) {
				keyState = KEY_STATE_BEING_PRESSED;
				prellDetects = PRELL_TICKS;
			}
			break;
		case KEY_STATE_BEING_PRESSED:
			if (buttonDown) {
				prellDetects--;
			}
			else {
				keyState = KEY_STATE_RELEASED;
			}

			if (!prellDetects) {
				keyState = KEY_STATE_PRESSED;
				event = EVENT_KEY_PRESS;
			}
			break;
		case KEY_STATE_PRESSED:
			if (!buttonDown) {
				keyState = KEY_STATE_BEING_RELEASED;
				prellDetects = PRELL_TICKS;
			}
			break;
		case KEY_STATE_BEING_RELEASED:
			if (!buttonDown) {
				prellDetects--;
			}
			else {
				keyState = KEY_STATE_PRESSED;
			}

			if (!prellDetects) {
				keyState = KEY_STATE_RELEASED;
				event = EVENT_KEY_RELEASE;
			}
			break;
	}

	return event;
}
