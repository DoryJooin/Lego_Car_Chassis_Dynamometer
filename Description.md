# Lego_Car_Chassis_Dynamometer
This is a description of Lego Car Chassis Dynamometer

[Components of dyno]
1. Arduino Uno
2. Lego MOC L motor enhanced(red)
   no-load voltage DC 7.4V
   no-load current 0.14A
   blocking current 2.1A
   no-load speed 520rpm
3. Cytron MDD3A motor driver
   operating voltage 4~16V
   continuous current 3A, peak 5A
   2 channels for brushed DC motor
4. I2C LCD 2004
   100uF ceramic condenser between vcc and gnd is needed to get rid of noises from DC motor
5. cooling fan
   DC 12V 2 wire
   rated 5400rpm
   rated current 0.3A
   maximum air flow 20.76CFM
6. TCRT5000 infrared reflective sensor
   DC 3~5V
   detection distance 0.2~15mm
   adjustable sensitivity by potentiometer
7. tact switch
   used in INPUT_PULLUP mode for toggling
8. DC power supply
   9V, 800mA


[Functions]
1. There are three modes.
   RUN : runs motor and cooling fan, calculates runtime.
   STBY : stops motor and cooling fan, saves total revolution to EEPROM.
   OVRSPD TRIP : if rpm goes higher than limit for more than 3 seconds, stops motor and cooling fan and freezes runtime.
   LOWSPD TRIP : if rpm goes lower than limit for more than 3 seconds, stops motor and cooling fan and freezes runtime.
   * trip state can be reset by pushing button. variables for reset and runtime will be clear and switches into STBY mode.

2. LCD displayed variables.
   LCD updates every 500ms.
   1st line) STATE : current operating mode.
   2nd line) RPM : RPM calculated by infrared sensor.
                   infrared sensor counts revolution by detecting reciprocating motion of the piston.
                   RPM is basically calculated by the time gap between each revolution
                   and corrected by averaging recent 10 revolutions' rpm(actually recent 10 revolutions' time gap to get rid of noise)
                   there's a noise filtering which ignores repeating signals in a short time(100ms) so rpm counter is available under 600rpm.
                   if there's no signal from infrared sensor for more than 3 seconds, the rpm is reset to 0.
   3rd line) RUNTIME : runtime is the total operating time since power is supplied to arduino.
                       runtime only counts when it's RUN mode and motor runs. it doesn't count in STBY and TRIP mode.
                       runtime will be clear to 0:0:0:0 if you reset the trip state.
   4th line) REV : there are two revolutions. one is current revolution and the other is total revolution.
                   current revolution is the revolution since power is supplied to arduino. it will be reset to 0 if you cut off the power.
                   total revolution is literally the total revolution since this code was uploaded onto arduino.
                   total revolution is saved to EEPROM so it does not reset even if there's no power supply.
                   guaranteed number of EEPROM writing is approximately 100,000. but total revolution is only saved in STBY and TRIP.
                   so I think endurance of EEPROM is enough through the lifetime of this machine.
                   if you have exceeded the limit of EEPROM writing, just copy the total revolution to another address
                   and change the 'address' variable(0~1023). initial address is 0.

3. Some serial prints for code maintenance are commented out.
