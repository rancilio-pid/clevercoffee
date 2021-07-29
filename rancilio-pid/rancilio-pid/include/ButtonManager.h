#ifndef ButtonManager_h
#define ButtonManager_h

#include "Arduino.h"

extern "C" {
typedef void (*callbackFunction)(void);
typedef void (*parameterizedCallbackFunction)(void *);
}

class ButtonManager
{
public:
  // ----- Constructor -----
  ButtonManager();

  /**
   * Initialize the ButtonManager
   * @param pin The pin to be used for input from a momentary button.
   * @param activeHigh Set to true when the input level is HIGH when the button is pressed, Default is true.
   * @param pulldownActive Activate the internal pulldown when available. Default is true.
   * @param pullupActive Activate the internal pullup when available. Default is false.
   */
  ButtonManager(const char pin, const boolean activeHigh = true, const bool pulldownActive = true, const bool pullupActive = false);

  // ----- Set runtime parameters -----

  /**
   * set # millisec after safe click is assumed.
   */
  void setDebounceTicks(const int ticks);

  /**
   * set # millisec after single click is assumed.
   */
  void setClickTicks(const int ticks);

  /**
   * set # millisec after press is assumed.
   */
  void setPressTicks(const int ticks);

  /**
   * Attach an event to be called when a single click is detected.
   * @param newFunction This function will be called when the event has been detected.
   */
  void attachClick(callbackFunction newFunction);
  void attachClick(parameterizedCallbackFunction newFunction, void *parameter);

  /**
   * Attach an event to be called after a double click is detected.
   * @param newFunction This function will be called when the event has been detected.
   */
  void attachDoubleClick(callbackFunction newFunction);
  void attachDoubleClick(parameterizedCallbackFunction newFunction, void *parameter);

   /**
   * Attach an event to be called after a tripple click is detected.
   * @param newFunction This function will be called when the event has been detected.
   */
  void attachTrippleClick(callbackFunction newFunction);
  void attachTrippleClick(parameterizedCallbackFunction newFunction, void *parameter);

  /**
   * Attach an event to be called after a multi click is detected.
   * @param newFunction This function will be called when the event has been detected.
   */
  void attachMultiClick(callbackFunction newFunction);
  void attachMultiClick(parameterizedCallbackFunction newFunction, void *parameter);

  /**
   * Attach an event to fire when the button is pressed and held down.
   * @param newFunction
   */
  void attachLongPressStart(callbackFunction newFunction);
  void attachLongPressStart(parameterizedCallbackFunction newFunction, void *parameter);

  /**
   * Attach an event to fire as soon as the button is released after a long press.
   * @param newFunction
   */
  void attachLongPressStop(callbackFunction newFunction);
  void attachLongPressStop(parameterizedCallbackFunction newFunction, void *parameter);

  /**
   * Attach an event to fire periodically while the button is held down.
   * @param newFunction
   */
  void attachDuringLongPress(callbackFunction newFunction);
  void attachDuringLongPress(parameterizedCallbackFunction newFunction, void *parameter);

  // ----- State machine functions -----

  /**
   * @brief Call this function every some milliseconds for checking the input
   * level at the initialized digital pin.
   */
  void tick(void);


  /**
   * @brief Call this function every time the input level has changed.
   * Using this function no digital input pin is checked because the current
   * level is given by the parameter.
   */
  void tick(bool level);


  /**
   * Reset the button state machine.
   */
  void reset(void);


  /*
   * return number of clicks in any case: single or multiple clicks
   */
  int getNumberClicks(void);

  /*
   * return debounced state
   */
  bool getState(void);

  /*
   * return raw input signal
   */
  bool getRawState(void);

  /*
   * return single click
   */
  bool getSingleClick(void);

  /*
   * return double click
   */
  bool getDoubleClick(void);

  /*
   * return tripple click
   */
  bool getTrippleClick(void);

  /*
   * returns true while button is pressed down for a longer period
   */
  bool getLongPressActive(void);

/*
   * returns true right after a long press has happend
   */
  bool getLongPress(void);



  /**
   * @return true if we are currently handling button press flow
   * (This allows power sensitive applications to know when it is safe to power down the main CPU)
   */
  bool isIdle() const { return _state == OCS_INIT; }

  /**
   * @return true when a long press is detected
   */
  bool isLongPressed() const { return _state == OCS_PRESS; };

private:
  int _pin;                         // hardware pin number.
  unsigned int _debounceTicks = 50; // number of ticks for debounce times.
  unsigned int _clickTicks = 200;   // number of msecs before a click is detected.
  unsigned int _pressTicks = 800;   // number of msecs before a long button press is detected

  int _buttonPressed;

  // These variables will hold functions acting as event source.
  callbackFunction _clickFunc = NULL;
  parameterizedCallbackFunction _paramClickFunc = NULL;
  void *_clickFuncParam = NULL;

  callbackFunction _doubleClickFunc = NULL;
  parameterizedCallbackFunction _paramDoubleClickFunc = NULL;
  void *_doubleClickFuncParam = NULL;

  callbackFunction _trippleClickFunc = NULL;
  parameterizedCallbackFunction _paramTrippleClickFunc = NULL;
  void *_trippleClickFuncParam = NULL;

  callbackFunction _multiClickFunc = NULL;
  parameterizedCallbackFunction _paramMultiClickFunc = NULL;
  void *_multiClickFuncParam = NULL;

  callbackFunction _longPressStartFunc = NULL;
  parameterizedCallbackFunction _paramLongPressStartFunc = NULL;
  void *_longPressStartFuncParam = NULL;

  callbackFunction _longPressStopFunc = NULL;
  parameterizedCallbackFunction _paramLongPressStopFunc = NULL;
  void *_longPressStopFuncParam;

  callbackFunction _duringLongPressFunc = NULL;
  parameterizedCallbackFunction _paramDuringLongPressFunc = NULL;
  void *_duringLongPressFuncParam = NULL;

  // These variables that hold information across the upcoming tick calls.
  // They are initialized once on program start and are updated every time the
  // tick function is called.

  // define FiniteStateMachine
  enum stateMachine_t : int {
    OCS_INIT = 0,
    OCS_DOWN = 1,
    OCS_UP = 2,
    OCS_COUNT = 3,
    OCS_PRESS = 6,
    OCS_PRESSEND = 7,
    UNKNOWN = 99
  };

  /**
   *  Advance to a new state and save the last one to come back in cas of bouncing detection.
   */
  void _newState(stateMachine_t nextState);

  stateMachine_t _state = OCS_INIT;
  stateMachine_t _lastState = OCS_INIT; // used for debouncing

  unsigned long _startTime; // start of current input change to checking debouncing
  int _nClicks;             // count the number of clicks with this variable
  int _maxClicks = 4;       // max number (1, 2, 3, 4 = multiclick) of clicks of interest by registration of event functions.
  bool _debouncedState;
  bool _rawState;
  bool _singleClick;
  bool _doubleClick;
  bool _trippleClick;
  bool _longPressActive;
  bool _longPress;

};

#endif