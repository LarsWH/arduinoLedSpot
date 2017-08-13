#include "LedSpot.h"
#include <EEPROM.h>
#define EEPROM_ADDR_OF_VAL 0

#define PWM_PIN  3

//#define PRELL_WAIT	10

//#define USE_FLOAT
#ifdef USE_FLOAT
	float max = 6.3413;
	int steps = 255;
	float increment = max / (float)steps;
#else
	#define VAL_MAX 10
	#define VAL_MIN 0
	int pwm[VAL_MAX + 1] = {
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
}

void writeLed() {
	analogWrite(PWM_PIN, pwm[val]);
}

bool incrementLed() {
	bool incremented = false;

#ifdef USE_FLOAT
#else
	if (val < VAL_MAX) {
		val++;
		incremented = true;
	}

	Serial.print("  val:");
	Serial.print(val);
	Serial.print(" PWM:");
	Serial.println(pwm[val]);
	writeLed();
#endif

	return incremented;
}


bool decrementLed() {
	bool decremented = false;
#ifdef USE_FLOAT
#else
	if (val > VAL_MIN) {
		val--;
		decremented = true;
	}

	Serial.print("  val:");
	Serial.print(val);
	Serial.print(" PWM:");
	Serial.println(pwm[val]);
	writeLed();
#endif

	return decremented;
}



bool isIncrementing = true;



void toggleDirection() {
	isIncrementing = !isIncrementing;
}

#define TICK_TIME 100
#define PRELL_TIME 300
#define SIGNAL_TIME 1000
#define INCREMENT_TIME 1000
#define SIGNAL_TICKS (SIGNAL_TIME/TICK_TIME)
#define PRELL_TICKS (PRELL_TIME/TICK_TIME)
#define INCREMENT_TICKS (INCREMENT_TIME/TICK_TIME)


void regulate(EventType event) {
	static RegulateStateType state = IDLE;
	static int signalTicks = SIGNAL_TICKS;
	static int incrementTicks = INCREMENT_TICKS;
	char *pString = "Woops";

	switch (event) 	{
		case EVENT_KEY_RELEASE:
			toggleDirection();
			state = IDLE;
			writeLed();
			//EEPROM.update(EEPROM_ADDR_OF_VAL, val);
			break;
	}

	switch (state) {
		case IDLE:
			pString = "IDLE";
			switch (event) {
				case EVENT_KEY_PRESS:
					if (isIncrementing) {
						state = INCREMENT_START;
					} else {
						state = DECREMENT_START;
					}
					signalTicks = SIGNAL_TICKS;
					break;
				default:
					break;
			}
			break;

		case INCREMENT_START:
			pString = "INCREMENT_START";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = INCREMENT;
						incrementTicks = INCREMENT_TICKS;
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case INCREMENT:
			pString = "INCREMENT";
			switch (event) {
				case EVENT_TIMER_TICK:
					incrementTicks--;
					if (!incrementTicks) {
						incrementTicks = INCREMENT_TICKS;
						if (!incrementLed()) {
							isIncrementing = false;
							state = INCREMENT_MAX_REACHED;
							signalTicks = SIGNAL_TICKS;
						}
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case INCREMENT_MAX_REACHED:
			pString = "INCREMENT_MAX_REACHED";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = INCREMENT_SIGNAL_MAX;
						signalTicks = SIGNAL_TICKS;
						analogWrite(PWM_PIN, 0);
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case INCREMENT_SIGNAL_MAX:
			pString = "INCREMENT_SIGNAL_MAX";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = DECREMENT_START;
						signalTicks = SIGNAL_TICKS;
						writeLed();
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case DECREMENT_START:
			pString = "DECREMENT_START";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = DECREMENT;
						incrementTicks = INCREMENT_TICKS;
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
					incrementTicks--;
					if (!incrementTicks) {
						incrementTicks = INCREMENT_TICKS;
						if (!decrementLed()) {
							isIncrementing = true;
							state = DECREMENT_MIN_REACHED;
							signalTicks = SIGNAL_TICKS;
						}
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case DECREMENT_MIN_REACHED:
			pString = "DECREMENT_MIN_REACHED";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = DECREMENT_SIGNAL_MIN;
						signalTicks = SIGNAL_TICKS;
						analogWrite(PWM_PIN, 0);
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;

		case DECREMENT_SIGNAL_MIN:
			pString = "DECREMENT_SIGNAL_MIN";
			switch (event) {
				case EVENT_TIMER_TICK:
					signalTicks--;
					if (!signalTicks) {
						state = INCREMENT_START;
						signalTicks = SIGNAL_TICKS;
						writeLed();
					}
					break;
				default:
					state = IDLE;
					break;
			}
			break;
	}

	static int x = 0;
	Serial.print(x++);
	Serial.print(" State: ");
	Serial.println(pString);
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
