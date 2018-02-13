/*
 * TaskManager.h
 *
 *  Created on: May 11, 2014
 *      Author: bram
 */
#include "GlobalIncludes.hpp"
//#include "Vectorclasses.hpp"
#include "GPIO.hpp"
#include "signal.h"


#ifndef TASKMANAGER_H_
#define TASKMANAGER_H_

namespace taskmanager_namespace
{
	enum dep_kind 				{no_dependancy,inport_dependant,variable_dependant};
	enum rept_kind         	    {not_repeating,repeating_weekly,repeating_daily,repeating_yearly,repeating_sectick,repeating_mintick};
//	enum limit_kind 			{top_true,bottom_true,equal_true};
	enum quantity_kind 			{temperature,voltage};
	enum comparatorstate_kind 	{single_treshold,low_treshold,high_treshold};
	enum command_Knd          	{comparator_high_tr, comparator_low_tr,comparator_toggle,comparator_disabld,comparator_notr_adjust,boiler_weather, boiler_room,};
	enum comparator_Inputkind	{adc_input,adc_gauged_input,i2c_input,ADC7420_temperature_input,no_inputdependancy,variable_input,boolean_variable_input,notdefined_input};
	enum buttontype_kind        {pressbutton,toggledswitch};
	typedef enum quantity_kind 				quantity_kind;
//	typedef enum limit_kind				limiter_kind;
	typedef enum comparatorstate_kind 		comp_state_kind;
    typedef enum rept_kind					repeat_kind;
	typedef enum command_Knd				command_kind;
	typedef enum comparator_Inputkind		comparator_inputkind;
	typedef enum buttontype_kind            buttontype;
	#define SECS_PER_DAY  		(24 * 3600)
}


class TaskSchedule;
class TcpServer;
class Comparator;

 //----------------- Manager for all the input and output tasks of the ports    ----------------//
class TaskManager
{ public:
    TaskManager();
    void ManageTasks(void);

    bool  	partner_alive;
    unsigned int 	duty_cycle_time;
    bool 	task_fulfilled;

    float 	room_temp;
    float 	outside_temp;
    void	put_CR_ser1();
    int 	TimeToTcpData_buf();    //returns length
    int 	Comp_Sch_namesToTcpData_buf(); //item  returns length
    void 	ReadAdCommandFromTcpBuffer();
    //void AdValueToTcp(char* answer,unsigned int ad_portnr);
    void 	ReadTaskFromTaskstring(void);
    int		ReadOutportFromTcpBuffer();
    //void    ReadComparatorFromTCPbuf (Comparator* comp);

  private:
    //int 	nr_taskcycles,countdown_windowspartner;
    int 	max_taskstringlength;  //=98 in constructor
    char 	taskstring[100];
    unsigned long ad24[16];       //AdInput
    bool 	send_clockpulses;

    void ReadTaskCycleTime(void);
    inline void TaskFulfilled(){taskstring[0]=0;task_fulfilled=true;};
    bool ReadTimeFromTaskstring(char *timestring,int startintasksting,bool read_weekday=true);  //return true if valid tasksring else false
    void Menu();
    int  DayNightBoilerTargetToTcpData_buf();
    void HeartbeatOnLastPort(void);

};


///******************************************************////
////*******************   NAME AND ORDER    *************////
////******************************************************////

class NameAndOrderBase{
public:
	NameAndOrderBase() {};
	void				SetName(string& namestring);
	inline char*		Name() 		{return name;};

	char       			name[COMPARATOR_NAMELENGTH+1];
};
///******************************************************////
////*******************   COMPARATOR    ******************////
////******************************************************////

/// Class to set an outport by various signals
//  digital or floating point
//  in case of the floating point regulation the class can function as:
//  1.  a simple comparator  or schmidt trigger
//  2.  a double comparator c.q. schmidt trigger (high /low treshold)
//  Comparator can be disabled
//  he comparator is tested by the main taskmanager loop

using namespace taskmanager_namespace;

class Comparator  :public NameAndOrderBase
{ public:
	GPIO						*outputport;
	//int 						pin_nr;   		// and pin of outport gpio
	float					    inputvalue,*input_float;  //value to compare with
	ADC							*input_Adc;
	//myI2C						*input_I2C;
	ADT7420_Tempsensor			*input_tempsensor;
	comparator_inputkind  		inputclass;
	float						hysteresis;
	comp_state_kind			    state;
	bool						enabled;
	bool						toggled;
	float						high_treshold_top;
	float						low_treshold_top;


   Comparator();
   Comparator(GPIO *outport,string& namestring);  //no input dependancy
   //Comparator(GPIO *outport,myI2C *input_I2C,  float htrhtp, float ltrhtp, string& namestring);
   Comparator(GPIO *outport,ADC	  *input_Adc,  float htrhtp, float ltrhtp, string& namestring);
   Comparator(GPIO *outport,float *inputvalue, float htrhtp, float ltrhtp, string& namestring);
   Comparator(GPIO *outport,ADT7420_Tempsensor *input, float htrhtp, float ltrhtp, string &namestring);
           //inport to compare //high treshold //low treshold
   ~Comparator();

   void 			Tune(TaskSchedule *schedule); //Tune the comparator with the taskschedule property's
   bool 			TakeAction();
   bool 			SensorConnected() 	{return sensor_connected;};
   void				SetInportNr(unsigned int nr);
   void				SetOutportNr(unsigned int nr);


  private:
   void 			Init(GPIO *outport,float htrhtp, float ltrhtp);
   int 				inport_nr;
   bool				sensor_connected;
};

class Comparators  : public virtual ivector<Comparator>
{ public:
	Comparators(){owner=false;};
	~Comparators(){};

	bool TakeAction();  //returns true if one of the outputs is switched
};
//********************************************************////
////*******************     OUTPORT      *****************////
////******************************************************////
// Every outport is paired with a comparator: the comparator drives the outport
class Outport 	:public GPIO   //*** Output control class ****
{   protected:
	 Comparator *comparator;
	public:
	Outport();
	Outport(Header_name nm, unsigned char nr,PIN_DIRECTION	dirctn,char* namestring);
	Outport(Header_name nm, unsigned char nr,char* namestring);
	void SetComparator (Comparator* comparator);
	inline Comparator* GetComparator(){return comparator;};

	~Outport();
};

class Relais	: public Outport  //*** Output control class for relais in mOsso****
{   public:
	Relais();
	Relais(int relais_number, char* namestring);

	~Relais();
};

//**********************************************************************////
////**************************     INPORT       ************************////
////********************************************************************////
// Inports can be powerd by a simlple high low signal or by a switch; in this case the switch is debounced
// the switch is either a pushbutton switch or a two state toggle
// If a comparator is coupled the switch the switch toggles a the comparator state (high treshold or low treshold

//constants used by the 'Jack G. Ganssle' debouncer
#define MAJORITY_VOTE 5  //MAJORITY_VOTE Number of samples for which the button must remain in pressed state to consider that the button is pressed.
#define STABILITY_BITS 2  //STABILITY_BITS is the number of final samples (last few samples at the end of debounce)after which we consider the input stable


class Inport 	:public GPIO   //*** Input control class ****
{ private:
	 bool         		button;   //buttons are debounced
     buttontype   		keytype;
	 PIN_VALUE    		switch_value;
     Comparator   		*coupled_comparator;
  public:
	 bool         		debouncedKeyPress;
	 unsigned int		count;   //number of times to check the key
	 bool         		action_to_take;
	 bool         		change_send_to_ftp;
	 pthread_mutex_t 	key_mutex;

	 Inport();
	 inline PIN_VALUE   switchstate(){return switch_value;};
	 void               ToggleSwitch();
	 buttontype         Keytype() {return keytype;};
	 void               debounceSwitch(bool *Key_changed, bool *Key_pressed);
	 void               debounceSwitch2(bool *Key_changed, bool *Key_pressed);
	 inline void        CoupleComparator(Comparator* c){coupled_comparator=c;};
     Inport(Header_name nm, unsigned char nr,char* namestring,bool button=false,buttontype tp=pressbutton);
     Inport(unsigned int mOsso_number,char* namestring,bool button=false,buttontype tp=pressbutton);
	 ~Inport(){};
};

// Arrays to bundle the inports and outports

class Outports 	:public virtual ivector<Outport>    //*** Output control class ****
{ 	public:
    	Outports(bool own=false){owner=own;};        // does not own elements by default
		~Outports();
		void  Add(Outport *port,Comparator *comp);
		void  Add(Outport& port,Comparator *comp);
		int   Nr(Outport* objptr);
//		Outport* &operator[](unsigned int index );
};

class Inports 	:public virtual ivector<Inport>    //*** Input control class ****
{ 	public:
    	Inports(bool own=false){owner=own;};        // does not own elements by default
		~Inports(){};
		void  Add(Inport *port);
};
////********************************************************************////
////************************** TASKSCHEDULE     ************************////
////********************************************************************////

// A scheduler for a scheduled change of a comparator state (high/low or treshold change)
// The schedule can be one time or periodic
using namespace taskmanager_namespace;
class TaskSchedule     :public NameAndOrderBase
{
  public:
     TaskSchedule();
     TaskSchedule(repeat_kind rk, const bool *repeat_od, command_kind com, Comparator* comp,string& name);

     ~TaskSchedule();

     time_t			timedate_to_act;      //first executing time
     bool			repeat_on_day[7];	  //sunday 0 monday 1 saturday 6
     bool  			repeating;
     repeat_kind    repeat_interval;
     char          	name[SCHEDULE_NAMELENGTH+1];
     command_kind	command;
     command_kind	command2;     // H high Low D disabled
    		//In roomcoontroller H daytemp L Nigttenp B Boiler high C Boiler Low
     command_kind	adjust_treshold; // H high Low D No treshold change N
     float			new_treshold;
     bool			enabled;
     int			nr_secminticks;   //repeat interval in secs or minutes in case of repeating_sectick,repeating_mintic
     void 			NextTask(time_t time);
     void  			Execute(void);

  private:
    void           	ExecuteCommand(command_kind comm,Comparator * comp);
    Comparator*     comparator;
    Comparator*     comparator2;

    void 			InitClass();
  public:
 //   inline bool operator<  (const TaskSchedule& compare_schedule){return  strcmp(name,compare_schedule.name)< 0;};
 //   inline bool operator>  (const TaskSchedule& compare_schedule){return  strcmp(name,compare_schedule.name)> 0;};
//    inline bool operator== (const TaskSchedule& compare_schedule){return  strcmp(name,compare_schedule.name)==0;};
 //   inline bool operator<=	(const TaskSchedule& compare_schedule){return  strcmp(name,compare_schedule.name)<=0;};
//    inline bool operator>= (const TaskSchedule& compare_schedule){return  strcmp(name,compare_schedule.name)>=0;};

    //bool   		 ActiveToday(CTime& time);
    bool   		ActiveToday();
    void		NextTask();

    friend class Tcp;
    friend class TcpServer;

};

////********************************************************************////
////************************** NEFITSCHEDULE    ************************////
////********************************************************************////
class NefitTime
{ public :
	NefitTime(){nefit_min_time=0;nefit_week=0;};
	NefitTime(time_t timet);

	NefitTime& operator =(time_t timet);
	NefitTime& operator =(struct tm  timet);
	//inline NefitTime& operator =(unsigned int t){ntime=t;return *this;};
    operator time_t();
    operator struct tm();
    time_t  LastSunday();
    unsigned int   d(){return nefit_min_time/(24*60);};
	unsigned int   t(){return nefit_min_time%(24*60);};
	inline void convert_tm(struct tm tim){nefit_min_time = tim.tm_wday*24*60+ tim.tm_hour*60 + tim.tm_min;};

	inline bool operator<  (const NefitTime& compare_time)	{return nefit_min_time< compare_time.nefit_min_time;};
	inline bool operator>  (const NefitTime& compare_time)	{return nefit_min_time> compare_time.nefit_min_time;};
	inline bool operator== (const NefitTime& compare_time)	{return nefit_min_time==compare_time.nefit_min_time;};
	inline bool operator<= (const NefitTime& compare_time)	{return nefit_min_time<=compare_time.nefit_min_time;};
	inline bool operator>= (const NefitTime& compare_time)	{return nefit_min_time>=compare_time.nefit_min_time;};
	inline bool operator<  (unsigned int compare_time)	{return nefit_min_time< compare_time;};
	inline bool operator>  (unsigned int compare_time)	{return nefit_min_time> compare_time;};
	inline bool operator== (unsigned int compare_time)	{return nefit_min_time==compare_time;};
	inline bool operator<= (unsigned int compare_time)	{return nefit_min_time<=compare_time;};
	inline bool operator>= (unsigned int compare_time)	{return nefit_min_time>=compare_time;};

  private:
	unsigned int nefit_min_time;   //time in minutes after sunday
	unsigned int nefit_week ;  //=0 repeating every week; =1 in first day of the week
	struct tm Tm();
	time_t 	  Time_t();


};

// A scheduler for a scheduled change of a comparator state
// The schedule weekly periodic
using namespace taskmanager_namespace;
class NefitSchedule
{
  public:
	 NefitSchedule();
	 NefitSchedule(unsigned int time,float temperature);
     ~NefitSchedule(){};

     NefitTime		time    ;   //time.nefit_min_time = in minutes after sunday 0.00 h
     float			temperature;
     bool			enabled;
     unsigned int   w;          // week 0 = every week: weekly schedule; 1= first week of the year (maybe only with 1 day Saturday)
     bool			to_fulfill; //task not yet fulfilled

     void  			Execute(void);
  private:
     //void         ExecuteCommand(command_kind comm,Comparator * comp);
     //Comparator*  comparator;

  public:
      inline bool operator<  (const NefitSchedule& compare_schedule){return time< compare_schedule.time;};
      inline bool operator>  (const NefitSchedule& compare_schedule){return time> compare_schedule.time;};
      inline bool operator== (const NefitSchedule& compare_schedule){return time==compare_schedule.time;};
      inline bool operator<= (const NefitSchedule& compare_schedule){return time<=compare_schedule.time;};
      inline bool operator>= (const NefitSchedule& compare_schedule){return time>=compare_schedule.time;};
      inline bool operator<  (unsigned int compare_time){return time< compare_time;};
      inline bool operator>  (unsigned int compare_time){return time> compare_time;};
      inline bool operator== (unsigned int compare_time){return time==compare_time;};
      inline bool operator<= (unsigned int compare_time){return time<=compare_time;};
      inline bool operator>= (unsigned int compare_time){return time>=compare_time;};

    friend class Tcp;
    friend class TcpServer;
};



//================Array of NefitSchedules=================//
class NefitSchedules: public sivector<NefitSchedule>
{ public:
					NefitSchedules(){::sivector<NefitSchedule>();time_to_act=LONG_MAX;};
					~NefitSchedules(){};

	time_t			time_to_act;      //first executing time
	NefitSchedule   *nextschedule;
	bool 			Read();
	void  			Execute(void);
private:
	//void 			NextTask(time_t time);
	void 			SetNextTask();
	int				NextItemNr(time_t &time);

};
//================Array of NefitSchedules=================//

//================Array of taskschedules=================//
class TaskSchedules
{ public:
    TaskSchedules();
    ~TaskSchedules();


    void Execute();
    bool Add(TaskSchedule* element);   //Adds only if name unique   if name == any of the old names: delete element and return false;
    void Delete(unsigned int i);       //deletes only if i exists returns false if i out of range
    void Detach(unsigned int i);
    void Update(unsigned int i,TaskSchedule* updated_element);
    void Interchange(unsigned int i,unsigned int j);
//  void Detach(Object* element);

    void Flush();
    //void NextTask(int i);					//Set schedule number i on next moment
    int  first_task;                    //Sequence number of first task to execute
    void SetFirstTask();               //Finds first task (in time) to execute

    //simplification of vector
    void			resize(int newCapacity);
	inline int		size() 		const	{return currentSize;}
	inline bool		empty() 	const	{return currentSize<=0;}
    unsigned int	capacity() 	const	{return capac;}

//  void  sort	()  {	quicksort(0,currentSize); };
    inline TaskSchedule* & operator[](unsigned int index )  {return objptrs[index];};
    //const TaskSchedule* & operator[](unsigned int index ) const;
    TaskSchedule  	**objptrs;
  private:
    unsigned int 		currentSize;
    unsigned int 		capac;

};



//================Array of taskschedules=================//


#endif /* TASKMANAGER_H_ */
