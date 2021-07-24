/**
 * @file ButtonManager.cpp
 *
 * @brief Debouncing and detecting different button function from different kind of buttons and sources
 *
 * @author DerChristian
 * Heavily based and inspired by the OneButton Library from
 * Matthias Hertel, https://www.mathertel.de
 * @Copyright Copyright (c) by Matthias Hertel, https://www.mathertel.de.
 * More information on: https://www.mathertel.de/Arduino/OneButtonLibrary.aspx
  **/


#include "ButtonManager.h"

/**
 * @brief Empty Curstructor usable for special inputs
 */
 ButtonManager::ButtonManager()
{
    _pin=-1;
}

ButtonManager::ButtonManager(const char pin, const boolean activeHigh, const bool pulldownActive, const bool pullupActive)
{
  // ButtonManager();
  _pin = pin;

  if (activeHigh) {
    // the button connects the input pin to VCC when pressed.
    _buttonPressed = HIGH;

  } else {
    // the button connects the input pin to GND when pressed.
    _buttonPressed = LOW;
  } // if

  if (pulldownActive) {
    // use the given pin as input and activate internal PULLDOWN resistor.
    pinMode(pin, INPUT_PULLDOWN);
  } 
  else if(pullupActive) {
    // use the given pin as input and activate internal PULLUP resistor.
    pinMode(pin, INPUT_PULLUP);
  } // if
  else {
    // use the given pin as input
    pinMode(pin, INPUT);
  } // if
} // ButtonManager

// set debounce time in millisec.
void ButtonManager::setDebounceTicks(const int ticks)
{
  _debounceTicks = ticks;
} // setDebounceTicks


// explicitly set the number of millisec that have to pass by before a click is detected.
void ButtonManager::setClickTicks(const int ticks)
{
  _clickTicks = ticks;
} // setClickTicks


// explicitly set the number of millisec that have to pass by before a long button press is detected.
void ButtonManager::setPressTicks(const int ticks)
{
  _pressTicks = ticks;
} // setPressTicks

// Beginning here are the options attach defined functions to button events, this part is left from the original library just for compatibility, this library focuses on getting states from the button
// save function for click event
void ButtonManager::attachClick(callbackFunction newFunction)
{
  _clickFunc = newFunction;
} // attachClick


// save function for parameterized click event
void ButtonManager::attachClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramClickFunc = newFunction;
  _clickFuncParam = parameter;
} // attachClick


// save function for doubleClick event
void ButtonManager::attachDoubleClick(callbackFunction newFunction)
{
  _doubleClickFunc = newFunction;
  _maxClicks = max(_maxClicks, 2);
} // attachDoubleClick


// save function for parameterized doubleClick event
void ButtonManager::attachDoubleClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramDoubleClickFunc = newFunction;
  _doubleClickFuncParam = parameter;
  _maxClicks = max(_maxClicks, 2);
} // attachDoubleClick

// save function for trippleClick event
void ButtonManager::attachTrippleClick(callbackFunction newFunction)
{
  _trippleClickFunc = newFunction;
  _maxClicks = max(_maxClicks, 3);
} // attachTrippleClick


// save function for parameterized trippleClick event
void ButtonManager::attachTrippleClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramTrippleClickFunc = newFunction;
  _trippleClickFuncParam = parameter;
  _maxClicks = max(_maxClicks, 3);
} // attachTrippleClick


// save function for multiClick event
void ButtonManager::attachMultiClick(callbackFunction newFunction)
{
  _multiClickFunc = newFunction;
  _maxClicks = max(_maxClicks, 100);
} // attachMultiClick


// save function for parameterized MultiClick event
void ButtonManager::attachMultiClick(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramMultiClickFunc = newFunction;
  _multiClickFuncParam = parameter;
  _maxClicks = max(_maxClicks, 100);
} // attachMultiClick


// save function for longPressStart event
void ButtonManager::attachLongPressStart(callbackFunction newFunction)
{
  _longPressStartFunc = newFunction;
} // attachLongPressStart


// save function for parameterized longPressStart event
void ButtonManager::attachLongPressStart(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramLongPressStartFunc = newFunction;
  _longPressStartFuncParam = parameter;
} // attachLongPressStart


// save function for longPressStop event
void ButtonManager::attachLongPressStop(callbackFunction newFunction)
{
  _longPressStopFunc = newFunction;
} // attachLongPressStop


// save function for parameterized longPressStop event
void ButtonManager::attachLongPressStop(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramLongPressStopFunc = newFunction;
  _longPressStopFuncParam = parameter;
} // attachLongPressStop


// save function for during longPress event
void ButtonManager::attachDuringLongPress(callbackFunction newFunction)
{
  _duringLongPressFunc = newFunction;
} // attachDuringLongPress


// save function for parameterized during longPress event
void ButtonManager::attachDuringLongPress(parameterizedCallbackFunction newFunction, void *parameter)
{
  _paramDuringLongPressFunc = newFunction;
  _duringLongPressFuncParam = parameter;
} // attachDuringLongPress


void ButtonManager::reset(void)
{
  _state = ButtonManager::OCS_INIT;
  _lastState = ButtonManager::OCS_INIT;
  _nClicks = 0;
  _startTime = 0;
  _debouncedState = 0;
  _rawState = 0;
  _singleClick = 0;
  _doubleClick = 0;
  _trippleClick = 0;
  _longPressActive = 0;
  _longPress = 0;
}

// In the original Library there was only one get function. To adapt the library for use in machines where different events should be triggered by the same button action depending on the machine state, new functions are added below.

// ShaggyDog ---- return number of clicks in any case: single or multiple clicks
int ButtonManager::getNumberClicks(void)
{
  return _nClicks;
}

bool ButtonManager::getDebouncedState(void)
{
  return _debouncedState;
}

bool ButtonManager::getRawState(void)
{
  return _rawState;
}

bool ButtonManager::getSingleClick(void)
{
  return _singleClick;
}

bool ButtonManager::getDoubleClick(void)
{
  return _doubleClick;
}

bool ButtonManager::getTrippleClick(void)
{
  return _trippleClick;
}

bool ButtonManager::getLongPressActive(void)
{
  return _longPressActive;
}

bool ButtonManager::getLongPress(void)
{
  return _longPress;
}

/**
 * @brief Check input of the configured pin and then advance the finite state
 * machine (FSM).
 */
void ButtonManager::tick(void)
{
  if (_pin >= 0) {
    tick(digitalRead(_pin) == _buttonPressed);
    }
}

/**
 *  @brief Advance to a new state and save the last one to come back to if debounce timer isn't reached.
 */
void ButtonManager::_newState(stateMachine_t nextState)
{
  _lastState = _state;
  _state = nextState;
} // _newState()

/**
 * @brief Run the finite state machine (FSM) using the given level.
 */
void ButtonManager::tick(bool activeLevel)
{
  unsigned long now = millis(); // current (relative) time in msecs.
  unsigned long waitTime = (now - _startTime);

  // Implementation of the state machine
  switch (_state) {
  case ButtonManager::OCS_INIT:
    // waiting for level to become active.
    if (activeLevel) {
      _newState(ButtonManager::OCS_DOWN);
      _startTime = now; // remember starting time
      _nClicks = 0;
      _rawState = true;
    } // if
    break;

  case ButtonManager::OCS_DOWN:
    // waiting for level to become inactive.

    if ((!activeLevel) && (waitTime < _debounceTicks)) {
      // button was released to quickly so I assume some bouncing.
      _newState(_lastState);

    } else if (!activeLevel) {
      _newState(ButtonManager::OCS_UP);
      _startTime = now; // remember starting time

    } else if ((activeLevel) && (waitTime > _pressTicks)) {
      if (_longPressStartFunc) _longPressStartFunc();
      if (_paramLongPressStartFunc) _paramLongPressStartFunc(_longPressStartFuncParam);
      _newState(ButtonManager::OCS_PRESS);
      _debouncedState = true;
    } // if
    break;

  case ButtonManager::OCS_UP:
    // level is inactive

    if ((activeLevel) && (waitTime < _debounceTicks)) {
      // button was pressed to quickly so I assume some bouncing.
      _newState(_lastState); // go back

    } else if (waitTime >= _debounceTicks) {
      // count as a short button down
      _nClicks++;
      _newState(ButtonManager::OCS_COUNT);
    } // if
    break;

  case ButtonManager::OCS_COUNT:
    // dobounce time is over, count clicks

    if (activeLevel) {
      // button is down again
      _newState(ButtonManager::OCS_DOWN);
      _startTime = now; // remember starting time

    } else if ((waitTime > _clickTicks) || (_nClicks == _maxClicks)) {
      // now we know how many clicks have been made.

      if (_nClicks == 1) {
        // this was 1 click only.
        if (_clickFunc) _clickFunc();
        if (_paramClickFunc) _paramClickFunc(_clickFuncParam);
        _singleClick = true;

      } else if (_nClicks == 2) {
        // this was a 2 click sequence.
        if (_doubleClickFunc) _doubleClickFunc();
        if (_paramDoubleClickFunc) _paramDoubleClickFunc(_doubleClickFuncParam);
        _doubleClick = true;

      } else if (_nClicks == 3) {
        // this was a 3 click sequence.
        if (_trippleClickFunc) _trippleClickFunc();
        if (_paramTrippleClickFunc) _paramTrippleClickFunc(_trippleClickFuncParam);
        _trippleClick = true;

      } else {
        // this was a multi click sequence.
        if (_multiClickFunc) _multiClickFunc();
        if (_paramMultiClickFunc) _paramMultiClickFunc(_doubleClickFuncParam);
      } // if

      reset();
    } // if
    break;

  case ButtonManager::OCS_PRESS:
    // waiting for menu pin being release after long press.

    if (!activeLevel) {
      _newState(ButtonManager::OCS_PRESSEND);
      _startTime = now;

    } else {
      // still the button is pressed
      if (_duringLongPressFunc) _duringLongPressFunc();
      if (_paramDuringLongPressFunc) _paramDuringLongPressFunc(_duringLongPressFuncParam);
      _longPressActive = true;
    } // if
    break;

  case ButtonManager::OCS_PRESSEND:
    // button was released.

    if ((activeLevel) && (waitTime < _debounceTicks)) {
      // button was released to quickly so I assume some bouncing.
      _newState(_lastState); // go back

    } else if (waitTime >= _debounceTicks) {
      if (_longPressStopFunc) _longPressStopFunc();
      if (_paramLongPressStopFunc) _paramLongPressStopFunc(_longPressStopFuncParam);
      _longPress = true;
      reset();
    }
    break;

  default:
    // unknown state detected -> reset state machine
    _newState(ButtonManager::OCS_INIT);
    break;
  } // if

} // ButtonManager.tick()


// end.