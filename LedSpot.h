#pragma once


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

typedef enum RegulateDirection {
	INCREMENTING,
	DECREMENTING
} RegulateDirectionType;

extern EventType keyDetect();
