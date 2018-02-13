/*
 * DisplayPage.cpp
 *
 *  Created on: Jul 19, 2017
 *      Author: bram
 */

#include "Display_Page.hpp"

#include "CharacterDisplay.hpp"
#include "GPIO.hpp"
#include "Adafruit_GFX.h"
#include "BBB_OLED.h"

#include "BBB_OLED.h"


#ifdef DISPLAY_PAGE_ENABLED
extern SPIBus_Device::OLEDCharacterDisplay 	*OLEDdisplay;
extern BBB_OLED 							*adafruitdisplay;

DisplayLine::DisplayLine()
{ inverted	= false;
  line_nr	= -1;
  type		= DISPLAY_UNDEFINED;
  for (int i=0;i<22;i++) character[i]=0;
  nr_characters=21;
}

DisplayLine::DisplayLine(DISPLAY_TYPE t,unsigned int nr)
{ inverted=false;
  type=t;
  int unsigned i;
  nr_characters=(type==DISPLAY_NEWHAVENOLED)?20:21;
  for (i=0;i<nr_characters;i++)
	  character[i]=' ';
  character[i]=0;
  line_nr=nr;
}

void DisplayLine::EditLine (string str)
{  unsigned int i;
   for (i=0; i< str.length() &&  i<nr_characters;i++)
	character[i]=str[i];    //edits a line,
   for (;i<nr_characters;i++)
	character[i]=' ';
}

void DisplayLine::Display()
{  if (type==DISPLAY_NEWHAVENOLED){
	 OLEDdisplay->setCursorRC(line_nr,0);
	 OLEDdisplay->print(character);
   }
   if (type==DISPLAY_ADAFRUITOLED){
     adafruitdisplay->setCursorRC(line_nr,0);
     adafruitdisplay->invertDisplay(inverted);
	 adafruitdisplay->print(character);
   }
}

DisplayPage::DisplayPage()
{  type=DISPLAY_UNDEFINED;
   inverted_line=-1;
   maxline=0;
   firstline=0;
}

DisplayPage::DisplayPage(DISPLAY_TYPE tp)
{  type=tp;
   inverted_line=-1;
   firstline=0;
   if (tp==DISPLAY_ADAFRUITOLED){
      printf("Adafruitbegin!\n");
	  adafruitdisplay->clearDisplay();
	  adafruitdisplay->setTextSize(1);
	  adafruitdisplay->setTextColor(WHITE);
	  adafruitdisplay->begin();
	  maxline=8;
   }
   maxline=DISPLAY_NEWHAVENOLED?4:8;
   for (int i=0;i<maxline;i++)
	   line.Add(new DisplayLine(type,i));

}

void DisplayPage::EditLine(unsigned int nr,string str){
	if (nr==line.size())
	{	DisplayLine *newline;
	    newline= new DisplayLine(type,nr);
	    line.Add(newline);
	}
	if (nr<=line.size())
		line[nr]->EditLine(str);

}

void DisplayPage::Display(){  //display the page
	if (type==DISPLAY_ADAFRUITOLED)
	{	adafruitdisplay->clearDisplay();
	    adafruitdisplay->begin();
	}

	for (int nr=firstline%maxline;nr<firstline%maxline+maxline;nr++)
		Display(nr);
	if (type==DISPLAY_ADAFRUITOLED)
		adafruitdisplay->close();
}

void DisplayPage::Display(int nr){  //display line
	if (line[nr]->inverted){
		if (inverted_line!=nr){
		    line[inverted_line]->inverted =false  ;               //deinvert inverted line
			line[inverted_line]->Display();
		}
		inverted_line=nr;
	}
	line[nr]->Display();
}
#endif
