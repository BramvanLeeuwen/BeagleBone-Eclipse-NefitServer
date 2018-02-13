/*
 * Vectorclasses.hpp
 *
 *  Created on: Nov 16, 2013
 *  Author: Bram van Leeuwen
 */

#ifndef VECTORCLASSES_HPP_
#define VECTORCLASSES_HPP_


#include <iostream>
#include <algorithm>	// std::swap()
#include <vector>
#include "GlobalIncludes.hpp"
#include <limits.h>
#include <iterator>

using namespace std;

#define  NOT_FOUND INT_MAX

class Fifo
{
private:
  float 		item[40];
  unsigned 		int maxnr;	//array groeit tot en met dit nummer
  unsigned 		int nr;		//number of array elements; eerst nul
  int     		last;
  unsigned int 	calc_deviation_after;
  float   		sum,mean,deviation;

public:
  Fifo ();       				// maakt array met maximaal 15 elemeneten
  Fifo (unsigned int max_nr);	// maakt array met maximaal max_nr elemeneten
  void 			Init(unsigned int max_nr,int deviation_after);
  void 			Add  (float newitem);
  void 			Flush();
  inline float 	Mean()			{return mean;};
  float 		Deviation();
	  	  	  //  float SmoothedMean();
	  	  	  //  float SmoothedDeviation();

  float 		last_input;		//laatste ingevoerde elememt bij start en leeg array -1
};

class ArrayElement
{  public:

   ArrayElement();
  ~ArrayElement();

   void 	    SetName(char*);
   inline char* Name() {return name;};

   void 		SetOutportNr(int nr);
   inline int   OutportNr(){return outport_nr;}

   //----------------- functions to order in array ---------------------------------//
   inline bool operator<  (const int verg_port){return (outport_nr< verg_port);};
   inline bool operator<  (const ArrayElement& verg_comp){return (outport_nr< verg_comp.outport_nr);};
   inline bool operator<= (const int verg_port){return (outport_nr<= verg_port);};
   inline bool operator<= (const ArrayElement& verg_comp){return (outport_nr<= verg_comp.outport_nr);};


  protected:
  	char           	*name;
	int 			outport_nr;
};


//---------------------------------------------------------------------------------------
//--------------------Direct Array-------------------------------------------------------
//---------------------------------------------------------------------------------------

template <class Object>  class dvector
{
  public:
	explicit dvector( ) : currentSize( 0 ){ objects = new Object[ 0 ]; }
    explicit dvector( int theSize = 0 ) : currentSize( theSize ){ objects = new Object[ currentSize ]; }
    //vector( const vector & rhs ) : objects( 0 ){ operator=( rhs ); }
    ~dvector( ){ delete [ ] objects; }

    int size( ) const
    	{ return currentSize; }
    Object & operator[](unsigned int index )
    	{   return objects[ index ];}
    const Object & operator[](unsigned int index ) const
    	{   return objects[ index ];}
    const dvector & operator = ( const dvector & rhs );
    void	resize(unsigned int newSize );

    void    		Add	(Object& element);
    int 			Delete(unsigned int i);
    int 			Detach(unsigned int i);
    void 			Flush	();
    void			sort	();
  protected:
    unsigned int 	currentSize;
  private:
    Object 			*objects;
    unsigned int 	_capacity;
    unsigned int    growsize;
    bool 			sorted;
};

//---------------------------------------------------------------------------------------
//--------------------Indirect Array-----------------------------------------------------
//---------------------------------------------------------------------------------------

template <class Object>
class ivector         //   :public vector<Object*> //indirect vector template
{
  public:
    	explicit 	ivector( int startCapacity = 10,int grow=5,bool own=true)
    	{  currentSize=0;
    	   owner=own;
    	   objptrs=new vector<Object*>(startCapacity);
    	   capac=startCapacity;
    	   growsize=max(2,grow);
    	};
    	~ivector( )
    	{ 	Object *objptr;
    	    if (owner)
    		for (int i=currentSize-1;i>=0;i--)
    		{  	objptr=(Object*)(*objptrs)[i];
    			delete objptr;
    			objptr=(Object*)NIL; //for safety;
    		}
    	};

	void			resize(int newCapacity);
	inline int		size() 		const	{return currentSize;}
	inline bool		empty() 	const	{return currentSize<=0;}
   	inline Object* 	back() 		{return (Object*)(*objptrs)[currentSize-1];};
   	inline Object* 	front() 	{return (Object*)(*objptrs)[0];};
	unsigned int	capacity() 	const{return capac;}
	bool			OwnsElements() {return owner;};

	Object* 		& operator[](unsigned int index )
					{  if (index<currentSize)
						   return((*objptrs)[index]);
						cout <<"operator [] out of range: index= "<< index << " !"<<endl;
						exit(-1);
						return objptrs->at(0) ;
					};

   	inline void		resize(unsigned int newSize )	{ objptrs->resize(newSize);}

  	void  			Add	(Object* element)
  					{   if (currentSize>=capac)
  							objptrs->resize(capac+=growsize);
  						objptrs->at(currentSize )=element;
  						currentSize++;
  					};

  	void  			Add	(Object& element)
  					{	Add(&element);
  					};

  	void 			Show()
  					{	Object obj,*ptr;
  						unsigned int i;
  						for (i=0;i<currentSize;i++)
  						{   ptr=(*objptrs).at(i);
  							cout <<"Ptr "<<ptr;
  							obj=*ptr;
  							cout<<"  index:"<<i<<" ; "<<obj;
  							cout<<endl;
  						};
  					};

    void  			Delete(unsigned int index)
    				{	if (index<currentSize && owner)
    					{    delete (*objptrs)[index];
    					     (*objptrs)[index]=(Object*)0;  //for safety;
    					}
    					Detach(index);
    				};
   	void 			Detach(unsigned int index)
   					{	if (index<currentSize)
   						{  for (unsigned int i=index;i<currentSize;i++)
   							(*objptrs)[i]=(*objptrs)[i+1];
   							currentSize--;
   						}
   					else
   						cout << "index out_of_range: range " <<index << endl;

   					};
   	void 			Detach(Object* element)
   					{	for  (int i=0;i<currentSize;i++)
   						{  if ((*objptrs)[i]==element)
   							Detach(i);
   						}
   					};
   	void 			Flush	()
   					{	currentSize=0;
   						objptrs->resize(capac);
   					};
   	void  			sort	()  {	quicksort(0,currentSize-1); };
   	void  			quicksort(int left, int right)
   					{  int 		i =left, j =right;
   					   Object 	*tmp;
   					   Object 	*pivot =(Object*)(*objptrs)[(left + right)/2];
   					   // partition //
   					   while (i <= j)
   					   {   while (*((Object*)(*objptrs)[i]) < *pivot)
   						   	   i++;
   					   	   while ( *pivot < *((Object*)(*objptrs)[j]))
   					   		   j--;
   					   	   if (i <= j)
   					   	   {   tmp = (*objptrs)[i];
   					   	   	   (*objptrs)[i]= (*objptrs)[j];
   					   	   	   (*objptrs)[j]= tmp;
   					   	   	   i++;
   					   	   	   j--;
   					   	   }
   					   };   //endwhile
   					   // recursion //
   					   if (left < j)
   						   quicksort(left, j);
   					   if (i < right)
   						   quicksort(i, right);

   					};  //end quicksort
   	int             nr(Object* objptr)   //returns the arraynr of the objectpointer ; -1 if not found
   					{ for (unsigned int i=0; i<currentSize;i++)
   						if (objptr== (*objptrs)[i])
   							return i;
                      return -1;
   					}
   	vector<Object*>	*objptrs;
  protected:
   	unsigned int 	currentSize;
   	unsigned int 	capac;
   	unsigned int	growsize;
   	bool			owner;
  //friend bool operator  < (TaskSchedule& ,const TaskSchedule&);
};


template <class Object>
class sivector : public virtual ivector<Object>           //indirect vector template with sort procedures
{
  public:
    explicit 	sivector( int startCapacity = 10,int grow=5,bool srt=true )
    				:ivector<Object>( startCapacity,grow), sorted(srt)
        		{   ivector<Object>::objptrs=new vector<Object*>(startCapacity);
        			ivector<Object>::currentSize=0;
        			ivector<Object>::capac= startCapacity;
        		};
    			~sivector( )
    			{ 	for (int i=ivector<Object>::currentSize-1;i>=0;i--)
    			      delete (*ivector<Object>::objptrs)[i];
    			};

    inline bool		empty() 	const	{return ivector<Object>::empty();}
    inline Object* 	back() 		{return ivector<Object>::back();}//(*ivector<Object>::objptrs)[sivector<Object>::currentSize-1];};
    inline Object* 	front() 	{return ivector<Object>::front();}//ivector<Object>::objptrs->at(0);};


    int Find(const Object *gezocht)
    {  sivector<Object*> item=ItemIter(gezocht);
          return (item==back()? INT_MAX :item-front());
    }


   	int	NextItemNr(const Object *gezocht)
   	{   int diff;
   		Object *ptr;
   			//  bool test=*back()<*gezocht;
   		if (empty()|| *back()<*gezocht)
   			return ivector<Object>::currentSize;
   		int right=ivector<Object>::currentSize-1,left=0,mid;
   		while ((diff=right- left)>1)
   		{  	mid=left +diff/2;
   			ptr=(*ivector<Object>::objptrs)[mid];
   			if (*ptr< *gezocht)
   			left= mid;
   			else
   			right= mid;
   		}
   		ptr=(*ivector<Object>::objptrs)[right];
   		if (*ptr<= *gezocht)
   			right++;
   			ptr=(*ivector<Object>::objptrs)[left];
   				//  bool test2= (*ptr<=*gezocht);
   		return (*ptr<=*gezocht) ? right: left;
   	};

    Object* NextItemIter(Object *gezocht){
   			   int i= NextItemNr(gezocht);
   			   return (i<0 ) ? NULL: ((Object*)(*(Object::objptrs))[i]);
    		};
	void  	Add	(Object* element)
			{ if (ivector<Object>::currentSize>=ivector<Object>::capac)
				ivector<Object>::objptrs->resize( ivector<Object>::capac=ivector<Object>::currentSize+sivector<Object>::growsize);
			  int newitemnr= sorted?NextItemNr(element):sivector<Object>::currentSize;
			  for (int i=ivector<Object>::currentSize;i>newitemnr; i--)
				  (* ivector<Object>:: objptrs)[i] =(*ivector<Object>::objptrs)[i-1];
			      (*ivector<Object>::objptrs)[newitemnr]=element;
			      ivector<Object>::currentSize++;
			};
   	void  	sort(){if (!sorted)this->ivector<Object>::quicksort(0,sivector<Object>::currentSize-1); sorted=true;};
   	inline void set_unsorted()	{ sorted=false;};
  private:
   	bool 		sorted;
};


#endif /* VECTORCLASSES_HPP_ */
