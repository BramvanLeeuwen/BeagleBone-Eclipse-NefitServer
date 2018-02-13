/*
 * Sensors.hpp
 *
 *  Created on: Jul 5, 2017
 *      Author: bram
 */

#ifndef SRC_SENSORS_HPP_
#define SRC_SENSORS_HPP_

#include "json.hpp"



enum OLEDDISPLAY{
	CHARACTER_OLED=0,
	GRAPHICAL_OLED=1,
};

struct sensor_display{
	bool 		 enabled;
	unsigned int row;
	unsigned int col;
	unsigned int decimals;
	bool         minus_sign;
	std::string	 displaystring;
};

class SenseAndDisplay
{  public:
	  SenseAndDisplay();
	  void SetDisplay(enum OLEDDISPLAY d,unsigned int row, unsigned int column);
	  float 	value;
	  struct 	sensor_display display[2];

   protected:

      void		Show();
      void 		ShowUnit();
   private:
      void 		Display();
   friend void  OLEDInfo();
   friend void  RefreshNewhavenOLED();
};

class TemperatureSensorEasy : public virtual SenseAndDisplay
{  public:
	TemperatureSensorEasy();
   ~TemperatureSensorEasy(){};
   bool Read();  //reads the temperature value true if changed
   void	Show();

};

class TemperatureSensorIntern : public virtual SenseAndDisplay
{  public:
	TemperatureSensorIntern();
   ~TemperatureSensorIntern(){};
   bool Read();  //reads the temperature and displays if changed
   void	Show();
};

class TemperatureSensorOutside : public virtual SenseAndDisplay
{  public:
	TemperatureSensorOutside();
   ~TemperatureSensorOutside(){};
   bool Read();  //reads the outside temperature and displays if changed
   void	Show();
};

class WaterPressureSensor : public virtual SenseAndDisplay
{  public:
	WaterPressureSensor();
   ~WaterPressureSensor(){};
   bool Read();  //reads the pressure and displays if changed
   void	Show();


};

class AirPressureSensor : public virtual SenseAndDisplay
{ public:
		AirPressureSensor();
		~AirPressureSensor(){};
   bool Read();  //reads the pressure and displays if changed
   void	Show();
   void SetJSon(json* weather);
  private:
   json* weather;


};
class HumiditySensor : public virtual SenseAndDisplay
{  public:
	HumiditySensor();
   ~HumiditySensor(){};
   bool Read();  //reads the relative humidity and displays if changed
   void	Show();
   void SetJSon(json* weather);
private:
   json* weather;

};


#endif /* SRC_SENSORS_HPP_ */
