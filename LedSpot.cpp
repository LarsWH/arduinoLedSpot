#include <EEPROM.h>
#define EEPROM_ADDR_OF_VAL 0
#define PWM_PIN  3
#define PUSH_BUTTON_PIN 5

#define VAL_MIN 64
#define VAL_MAX 255
#define MAX_PWM 255
#define STEPS (VAL_MAX-VAL_MIN+1)
#define RAMP_TIME 4000
#define INCREMENT_TIME (RAMP_TIME/STEPS)
#define TICK_TIME 10
#define PRELL_TIME 50
#define PRELL_TICKS (PRELL_TIME/TICK_TIME)
#define INCREMENT_TICKS (INCREMENT_TIME/TICK_TIME)

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
float max = 3.9961; //  max*max*max*max = 255
int val = VAL_MIN;
bool isIncrementing = true;


//---------- Prototypes ----------
EventType keyDetect();
void rampUp(int value);
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
	rampUp(value);
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


void writeLed() {
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
	Serial.print(" PWM:");
	Serial.println(pwmInt);

	analogWrite(PWM_PIN, pwmInt);
}


void rampUp(int value) {
	int valuesToRamp = value - VAL_MIN;
	for (int i = VAL_MIN; i <= value; i++) {
		val = i;
		writeLed();
		delay(INCREMENT_TIME);
	}
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


void toggleDirection() {
	isIncrementing = !isIncrementing;
}


void regulate(EventType event) {
	static RegulateStateType state = IDLE;
	static int incrementTicks = INCREMENT_TICKS;

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
