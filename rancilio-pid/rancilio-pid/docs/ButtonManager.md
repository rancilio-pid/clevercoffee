The ButtonManager is heavily based on the OneButton Library (https://github.com/mathertel/OneButton)
For further documentation see the original Libary. 


At the following callback functions are available

getState
reports back the debounced state of the switch/button

getRawState
reports back the raw input signal of the switch/button

getSingleClick
reports back single click event after detection time is passed

getDoubleClick
reports back double click event after detection time is passed

getTrippleClick
reports back tripple click event after detection time is passed

getLongPressActive
reports back that the button is pressed longer than the long press detection time

getLongPress
reports back long press event after button is released

getNumber
reports back number of clicks after detection time or when max number of clicks is reached


Further more there is the option to attach functions directly to button events. This is part of the original library and was left in the code because why not. At the moment the only viable use that comes to my mind would be to attach a emergency function to a long press of one of the buttons to halt all running operations in case of water leakage or similar.

There is also the option to use the ButtonManager constructor with other inputs than gpios, therefore a empty constructor must be created and further defined in setup and loop. Again see original OneButton exampled till this documentation is finished.
The same goes for attaching interrupts to the buttons.