/*
 * Vectorclasses.cpp
 *
 *  Created on: Nov 16, 2013
 *      Author: Bram van Leeuwen
 */


#include "Vectorclasses.hpp"
#include <math.h>

//_________________________________________________________________//
//                                                                 //
// *********                                          ************ //
// *********             First In First Out           ************ //
// *********                                          ************ //
//_________________________________________________________________//

Fifo::Fifo()
{  maxnr=39;
   calc_deviation_after=15;
   Flush();
}

Fifo::Fifo(unsigned int max_nr)
{  maxnr=max_nr;
   calc_deviation_after=15;
   Flush();
}

void Fifo::Init(unsigned int max_nr,int deviation_after)
{  maxnr=max_nr;
   calc_deviation_after=deviation_after;
}

void Fifo::Flush()
{  nr=0;
   last_input=0;
   sum=0;
   mean=0;
   last=-1;
}

void Fifo::Add(float newitem)
{  last++;        //de plek waar dit item ingevoegd wordt
   if ((unsigned int)last>=maxnr) last=0;
   float* lst=&(item[last]);
   sum+=newitem;
   if (nr<maxnr)
      nr++;
   else
      sum-=*lst;
   *lst= newitem;
   last_input=newitem;
   mean=sum/nr;
}

float Fifo::Deviation()
{ static unsigned int dev_counter=0;
  if (dev_counter==0 || nr<maxnr)
  { float  dev,sumsquare=0;
    for (unsigned int i=0;i<nr;i++)
  	 { dev= item[i]-mean;
    	sumsquare+= dev*dev;
  	 }
    deviation= sqrt(sumsquare/nr);
  }
  if (dev_counter==calc_deviation_after)
    dev_counter=0;
  else
    dev_counter++;
  return deviation;
}


