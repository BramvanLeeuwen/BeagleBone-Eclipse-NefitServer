# BeagleBone-Eclipse-NefitServer

Project to control a NefitEasy server
Included are files to readout a rotary encoder , several (debounced) buttons, Adafruit Oled and Newhaven Oled (character) displays, 

Temperature is read from a ADT7420 sensor.

Work is under construction but as far as it works fine. Weather data are read from OpenWeather.

The program uses the Qt-project NefitEast as an intermediate that communicates with the Nefit http server.
Communication between the two programs via ftp.
