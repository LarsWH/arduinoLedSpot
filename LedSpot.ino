#include <EEPROM.h>
#define ADDR_OF_VAL 0

#define CTRL_PIN  3//LED_BUILTIN
#define TIME_HIGH 1
#define TIME_LOW 40

#define FIRST_KEY 5 //10
#define LAST_KEY  5 //13

#define PRELL_DETECTS 4
#define PRELL_WAIT	10

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

int val = VAL_MIN;
#define WRAPIT


void setup()
{
	Serial.begin(115200);


	analogWrite(CTRL_PIN, 0);
	pinMode(CTRL_PIN, OUTPUT);
	analogWrite(CTRL_PIN, 0);

	for (int i = FIRST_KEY; i <= LAST_KEY; i++) {
		pinMode(i, INPUT);
		digitalWrite(i, HIGH);
	}

	Serial.println("Initialized");
	val = EEPROM.read(ADDR_OF_VAL);
}

int getKeyDown() {
	for (int i = FIRST_KEY; i <= LAST_KEY; i++) {
		if (!digitalRead(i)) {
			for (int prell = 0; prell < PRELL_DETECTS; prell++) {
				if (digitalRead(i)) {
					return 0; // not pressed or prell detected
				}
				delay(PRELL_WAIT);
			}
			return i;
		}
	}
	return 0;
}

int detectKey() {
	static int keyDown = 0;
	static int lastKeyDown = 0;
	int newKeyPress = 0;

	keyDown = getKeyDown();

	if (keyDown) {
		if (keyDown != lastKeyDown) {
			// New key press
			newKeyPress = keyDown;
			Serial.print("Key:");
			Serial.print(newKeyPress);
		}
	}
	lastKeyDown = keyDown;
	return newKeyPress;
}


void incrementLed() {
	if (val < VAL_MAX) {
		val++;
	} 
#ifdef WRAPIT
	else {
		val = 0;
	}
#endif

	Serial.print("  val:");
	Serial.print(val);
	Serial.print(" PWM:");
	Serial.println(pwm[val]);

	EEPROM.update(ADDR_OF_VAL, val);

}

void decrementLed() {
	if (val > VAL_MIN) {
		val--;
	} 
#ifdef WRAPIT
	else {
		val = VAL_MAX;
	}
#endif
	Serial.print("  val:");
	Serial.print(val);
	Serial.print(" PWM:");
	Serial.println(pwm[val]);

	EEPROM.update(ADDR_OF_VAL, val);

}

void loop()
{
	int key = detectKey();

	if (key == FIRST_KEY) {
		incrementLed();
	} else if (key == (FIRST_KEY + 1)) {
		decrementLed();
	}
	analogWrite(CTRL_PIN, pwm[val]);

	/*
	digitalWrite(CTRL_PIN, HIGH);
	delayMicroseconds(TIME_HIGH);
	digitalWrite(CTRL_PIN, LOW);
	delayMicroseconds(TIME_LOW);
	*/


}
