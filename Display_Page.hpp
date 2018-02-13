/*
 * DisplayPage.hpp
 *
 *  Created on: Jul 19, 2017
 *      Author: bram
 */

#ifdef DISPLAY_PAGE_ENABLED


#ifndef SRC_DISPLAY_PAGE_HPP_
#define SRC_DISPLAY_PAGE_HPP_
#include "GlobalIncludes.hpp"
//#include "Vectorclasses.hpp"
#include <cstring>
#include <string>

enum DISPLAY_TYPE{
	DISPLAY_UNDEFINED=0,
	DISPLAY_NEWHAVENOLED=1,
	DISPLAY_ADAFRUITOLED=2,

};

class DisplayLine
{ public:
	DisplayLine(); //creates a displayline of max 21 characters
	DisplayLine(DISPLAY_TYPE t,unsigned int line_nr); // type of display
	void EditLine (string str);    //edits a line,

   	DISPLAY_TYPE type;
    bool inverted;
    void Display();  //displays the line
  private:
	char character [22];  //max 21 char + traling zero
	unsigned int line_nr;
	unsigned int nr_characters;
};

class DisplayLines  : public virtual ivector<DisplayLine>
{ public:
	DisplayLines(){;};
	~DisplayLines(){};
};

class DisplayPage
{  public:
	DisplayPage(); //creates a page of 4 rows of 20 characters
    DisplayPage(DISPLAY_TYPE tp);//creates a page of nr_rows rows of nr_characters characters
    void EditLine(unsigned int nr,string str);
	void 		Display();  //display the page
	void 		Display(int nr);  //displays the line
	DISPLAY_TYPE type;
	DisplayLines line;
	int			firstline;   // the uppermost line shown (more lines than maxline
private:
	short  	inverted_line; //number of the inverted line: -1 = no inverted line
	unsigned short maxline;  //number of lines shown on the display

};


#endif /* SRC_DISPLAY_PAGE_HPP_ */
#endif
