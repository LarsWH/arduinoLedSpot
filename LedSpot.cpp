#include "LedSpot.h"
#include <EEPROM.h>
#define EEPROM_ADDR_OF_VAL 0
#define PWM_PIN  3


#define USE_FLOAT
#ifdef USE_FLOAT
	#define VAL_MIN 64
	#define VAL_MAX 255
	#define MAX_PWM 255
	#define STEPS (VAL_MAX-VAL_MIN+1)
	float max = 3.9961; //6.3413;
#else
	#define VAL_MIN 0
	#define VAL_MAX 10
	#define STEPS (VAL_MAX + 1)
	int pwm[STEPS] = {
		1, //1.7404
		2,
		3,
		5,
		9,
		15,
		27,
		48,
		84,
		146,
		255 };

#endif

int val = VAL_MIN;



#define PUSH_BUTTON_PIN 5

void writeLed() {
#ifdef USE_FLOAT

	Serial.print("  val:");
	Serial.print(val);

	float scaled = ((float)val * max) / (float)VAL_MAX;

	Serial.print("  scaled:");
	Serial.print(scaled);


	float pwmVal = scaled * scaled * scaled * scaled;

	Serial.print("  pwmVal:");
	Serial.print(pwmVal);

	int pwmInt = (int)pwmVal;
	if (pwmInt > MAX_PWM) {
		pwmInt = MAX_PWM;
	}
#else
	int pwmInt = pwm[val];
#endif

	Serial.print(" PWM:");
	Serial.println(pwmInt);

	analogWrite(PWM_PIN, pwmInt);
}


void setup()
{
	Serial.begin(115200);

	analogWrite(PWM_PIN, 0);
	pinMode(PWM_PIN, OUTPUT);
	analogWrite(PWM_PIN, 0);

	pinMode(PUSH_BUTTON_PIN, INPUT);
	digitalWrite(PUSH_BUTTON_PIN, HIGH);

	Serial.println("Initialized");
	val = EEPROM.read(EEPROM_ADDR_OF_VAL);

	if (val < VAL_MIN) {
		val = VAL_MIN;
	} 
	if (val > VAL_MAX) {
		val = VAL_MAX;
	} 
	writeLed();
}

bool incrementLed() {
	bool incremented = false;
	if (val < VAL_MAX) {
		val++;
		incremented = true;
	}
	writeLed();
	return incremented;
}


bool decrementLed() {
	bool decremented = false;
	if (val > VAL_MIN) {
		val--;
		decremented = true;
	}
	writeLed();
	return decremented;
}



bool isIncrementing = true;



void toggleDirection() {
	isIncrementing = !isIncrementing;
}

#define RAMP_TIME 4000
#define INCREMENT_TIME (RAMP_TIME/STEPS)
#define TICK_TIME 10
#define PRELL_TIME 50
#define PRELL_TICKS (PRELL_TIME/TICK_TIME)
#define INCREMENT_TICKS (INCREMENT_TIME/TICK_TIME)


void regulate(EventType event) {
	static RegulateStateType state = IDLE;
	static int incrementTicks = INCREMENT_TICKS;
	char *pString = "Woops";

	switch (event) 	{
		case EVENT_KEY_RELEASE:
			toggleDirection();
			state = IDLE;
			writeLed();
			EEPROM.update(EEPROM_ADDR_OF_VAL, val);
			break;
	}

	switch (state) {
		case IDLE:
			pString = "IDLE";
			switch (event) {
				case EVENT_KEY_PRESS:
					if (isIncrementing) {
						state = INCREMENT;
					} else {
						state = DECREMENT;
					}
					incrementTicks = INCREMENT_TICKS;
					break;
				default:
					break;
			}
			break;

		case INCREMENT:
			pString = "INCREMENT";
			switch (event) {
				case EVENT_TIMER_TICK:
					if (!incrementTicks--) {
						incrementTicks = INCREMENT_TICKS;
						if (!incrementLed()) {
							isIncrementing = false;
							state = DECREMENT;
						}
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case DECREMENT:
			pString = "DECREMENT";
			switch (event) {
				case EVENT_TIMER_TICK:
					if (!incrementTicks--) {
						incrementTicks = INCREMENT_TICKS;
						if (!decrementLed()) {
							isIncrementing = true;
							state = INCREMENT;
						}
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

	}

/*	static int x = 0;
	Serial.print(x++);
	Serial.print(" State: ");
	Serial.println(pString); */
}

void loop()
{
	EventType event = keyDetect();
	if (event == EVENT_NOTHING) {
		regulate(EVENT_TIMER_TICK);
	} else {
		regulate(event);
	}

	delay(TICK_TIME);
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
			Serial.println("---- EVENT: PRESSED");
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
			Serial.println("---- EVENT: RELEASED");
		}
		break;

	}


	return event;
}
