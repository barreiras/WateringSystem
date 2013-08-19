/*
Add reset button to clear alarms
*/

#include <EEPROM.h>
#include <Time.h>
#include <Wire.h>
#include <TimeAlarms.h>
#include "EEPROMAnything.h"

#define DS1307_I2C_ADDRESS 0x68

#define CONFIG_VERSION "WC006"

static const int relayCount=4;
static const int resetPin=4;

int powerLed = 13;

struct relayConfig_t
{
    time_t startTime; //activation hour
    int seconds; // how long it will be active in seconds
    int pin;
    boolean active;
    AlarmID_t alarmid;
    
};

relayConfig_t relay[relayCount];

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

/************************************************************************/

void setup() {
  
  pinMode(powerLed, OUTPUT);  
  Serial.begin(9600);
  
  Wire.begin();

  //setDateDs1307(30, 58, 15, 7, 3, 8, 13);
  setclock();

  inputString.reserve(200);
  
  loadConfig();
  
  Alarm.timerOnce(5, relayCheck);
}
/************************************************************************/
void loop() {
  digitalWrite(powerLed, HIGH);
  
  if (stringComplete) {
    //Serial.println(inputString);
    
    processSerial(inputString); 
    inputString = "";
    stringComplete = false;
  }
  
  Alarm.delay(100);
}

/************************************************************************/
void loadConfig() {
  
  char ver[5];
  
  EEPROM_readAnything(0,ver);
  
  String s = ver;
  s.trim();
  char charBuf[100];
  s.toCharArray(charBuf, 100) ;
  Serial.println(charBuf);
  
  if(strcmp(charBuf,CONFIG_VERSION) == 0){
	Serial.println("Reading stored config");
    EEPROM_readAnything(5,relay);
    
    for(int i=0;i<relayCount;i++){
      relay[i].alarmid = 0;
      startalarm(i);
      pinMode(relay[i].pin, OUTPUT); 
      digitalWrite(relay[i].pin, HIGH);
    }
    
  }else{
    Serial.println("writing config to eeprom");
     //initializeconfigs
    for(int i=0;i<relayCount;i++){
      relay[i].startTime=now();
      relay[i].seconds=10;
      relay[i].pin=9+i;
      relay[i].active = false;
      relay[i].alarmid = 0;
    }
    
    EEPROM_writeAnything(0,CONFIG_VERSION);
    EEPROM_writeAnything(5,relay);
  }
}


/************************************************************************/
void serialEvent() {
  char inChar;
  if (Serial){
    while (Serial.available()) {
      inChar = (char)Serial.read(); 
      Serial.print(inChar);
      inputString += inChar;
    }
  
    if (inChar == '\n' || inChar == '\r') {
        stringComplete = true;
        Serial.println();
    } 
  }
}

/************************************************************************/
void processSerial(String cmd){
  char *str;
  int badCommand=0;
  int error=0;

  cmd.trim();
  
  char charBuf[100];
  cmd.toCharArray(charBuf, 100) ;
  
  char *p = charBuf;
  
  str = strtok_r(p, " ", &p);
  
  if(str!= NULL)
    if(strcmp(str,"list") == 0 || strcmp(str,"ls") == 0)
      error = processCommandList(p);  
    else 
      if(strcmp(str,"set") == 0 )
        error = processCommandSet(p);
      else
        if(strcmp(str,"time") == 0 ){
          time_t d = now();
          
		  Serial.print(hour(d));
		  Serial.print(":");
		  Serial.print(minute(d));
		  Serial.print(":");
		  Serial.print(second(d));
                  Serial.print(" ");
                  
                  Serial.print(day(d));
		  Serial.print("/");
		  Serial.print(month(d));
		  Serial.print("/");
		  Serial.print(year(d));
		  Serial.println();
		  
        }else 
          if(strcmp(str,"test") == 0 )
            relayConfigTest();
          else
            if(strcmp(str,"test2") == 0 ){
              Serial.println("---------Starting delay----------");
               Alarm.delay(50000);
               Serial.println("---------Done-------");
            }else
              error=1;
  else
    error=1;
      
  if(error)
	Serial.println("Bad command or filename");
  //else{save to eprom}
  
}

/************************************************************************/
int processCommandList(char *p){
  char *str;
  str = strtok_r(p, " ", &p);
      
  if(str!= NULL)
    if(strcmp(str,"raw") == 0 )
      list(true); 
    else 
      Serial.println("list commands: list, list raw");
  else list(false);
  
  return 0;
}

/************************************************************************/
int processCommandSet(char *p){
  char *str;
  int relaynumber,h,m,s;
  String error="Set sintax: set <relay> <activation time hh:mm:ss> <time in seconds> <pin> <active>";  
   
  str = strtok_r(p, " ", &p);
  if(str!= NULL){  
    
    if(strcmp(str,"time") == 0 ){
        str = strtok_r(p, " ", &p);
        if(str!= NULL){
          tmElements_t tmSet;
          char *str_t = strtok_r(str, ":", &str);
          tmSet.Hour=atoi(str_t);
          
          str_t = strtok_r(str, ":", &str);
          tmSet.Minute=atoi(str_t);
          
          str_t = strtok_r(str, ":", &str);
          tmSet.Second=atoi(str_t);
          
          time_t d = now();
          
          setTime(tmSet.Hour,tmSet.Minute,tmSet.Second,day(d),month(d),year(d));
          setDateDs1307(tmSet.Second, tmSet.Minute, tmSet.Hour, weekday(d), day(d), month(d), year(d));
                      
        }
      }else{
        
        
        relaynumber=atoi(str);
    
        str = strtok_r(p, " ", &p);
        if(str!= NULL){
          if(strcmp(str,"starttime") == 0 ){
            str = strtok_r(p, " ", &p);
              if(str!= NULL){
                tmElements_t tmSet;
                char *str_t = strtok_r(str, ":", &str);
                tmSet.Hour=atoi(str_t);
                
                str_t = strtok_r(str, ":", &str);
                tmSet.Minute=atoi(str_t);
                
                str_t = strtok_r(str, ":", &str);
                tmSet.Second=atoi(str_t);
                
                relay[relaynumber].startTime = makeTime(tmSet);
                
                if(relay[relaynumber].active == 1){
                  Alarm.disable(relay[relaynumber].alarmid);
                  startalarm(relaynumber);
                }
                
              }
          }
          if(strcmp(str,"seconds") == 0 ){
            str = strtok_r(p, " ", &p);
              if(str!= NULL)
                  relay[relaynumber].seconds=atoi(str);
          }
          if(strcmp(str,"pin") == 0 ){
            str = strtok_r(p, " ", &p);
              if(str!= NULL){
                  relay[relaynumber].pin=atoi(str);
                  pinMode(relay[relaynumber].pin, OUTPUT);
                  digitalWrite(relay[relaynumber].pin, HIGH); 
                }
          }
          if(strcmp(str,"active") == 0 ){
            str = strtok_r(p, " ", &p);
              if(str!= NULL){
                int active = atoi(str);
                relay[relaynumber].active=active;
                if(active == true){
                  startalarm(relaynumber);
                }else{
                  Alarm.disable(relay[relaynumber].alarmid);
                }
              }
          }
          if(strcmp(str,"open") == 0 ){
            str = strtok_r(p, " ", &p);
              if(str!= NULL){
                int seconds = atoi(str);
                openrelay(relaynumber,seconds);
              }
          }
         
         
        //  }else{Serial.println(error);}
        //}else{Serial.println(error);}
        
        boolean raw = false;
        int i = relaynumber;
        
        showConfig(i,false, true);
        
        EEPROM_writeAnything(5,relay);
      }
    }
  }
  
  return 0;
}


/************************************************************************/
void relayConfigTest(){
  
  for(int i=0;i<relayCount;i++){
    if(relay[i].active == true){
      openrelay(i,relay[i].seconds);
    }
  }
  
}
/************************************************************************/
void relayCheck(){
  openrelay(0,1);
  Alarm.delay(500);
  openrelay(1,1);
  Alarm.delay(500);
  openrelay(2,1);
  Alarm.delay(500);
  openrelay(3,1);
}

void openrelay0(){openrelay(0,relay[0].seconds);}
void openrelay1(){openrelay(1,relay[1].seconds);}
void openrelay2(){openrelay(2,relay[2].seconds);}
void openrelay3(){openrelay(3,relay[3].seconds);}
/************************************************************************/
void openrelay(int relaynumber,int seconds){
  
  Serial.println("");
  Serial.print("starting relay ");  Serial.print(relaynumber);
  Serial.print(" for ");  Serial.print(seconds); Serial.println(" seconds");
  for(int i=0;i<seconds;i++)
    Serial.print("-");
  Serial.println("");
  
  digitalWrite(relay[relaynumber].pin, LOW);
  
  for(int i=0;i<seconds;i++){
    Alarm.delay(1000);
    Serial.print("=");
  }
  Serial.println("");
  digitalWrite(relay[relaynumber].pin, HIGH);
  
}


/************************************************************************/
void list(boolean raw){
    for(int i=0;i<relayCount;i++){
      showConfig(i,raw,i==0);
    }
}

/************************************************************************/
int startalarm(int relaynumber){
  if( relay[relaynumber].active == true ){
    relay[relaynumber].alarmid =Alarm.alarmRepeat(hour(relay[relaynumber].startTime),
                                                 minute(relay[relaynumber].startTime),
                                                 second(relay[relaynumber].startTime), 
                                                 (relaynumber == 0)?openrelay0:
                                                 (relaynumber == 1)?openrelay1:
                                                 (relaynumber == 2)?openrelay2:openrelay3);
  }
}



/************************************************************************/
void showConfig(int i, boolean raw, boolean showheader){
  
   if(showheader)
     raw?:Serial.println("| relay | starttime | seconds | pin | active | alarmid |");
  
   raw?:Serial.print("|   "); Serial.print(i);
      raw?Serial.print(","):Serial.print("   | "); 
      
          if(hour(relay[i].startTime)<10)Serial.print("0");
          Serial.print(hour(relay[i].startTime));
          Serial.print(":");
          if(minute(relay[i].startTime)<10)Serial.print("0");
          Serial.print(minute(relay[i].startTime));
          Serial.print(":");
          if(second(relay[i].startTime)<10)Serial.print("0");
          Serial.print(second(relay[i].startTime));
      
      raw?Serial.print(","):Serial.print("  |  "); Serial.print(relay[i].seconds);
      
      if(!raw){
        if(relay[i].seconds<10)Serial.print(" ");
        if(relay[i].seconds<100)Serial.print(" ");
        if(relay[i].seconds<1000)Serial.print(" ");
        if(relay[i].seconds<10000)Serial.print(" ");
      }
      
      raw?Serial.print(","):Serial.print("  | "); Serial.print(relay[i].pin);
      if(!raw)if(relay[i].pin<10)Serial.print(" ");
      
      raw?Serial.print(","):Serial.print("  |   "); Serial.print(relay[i].active); 
      
      raw?Serial.print(","):Serial.print("    |  "); Serial.print(relay[i].alarmid);
      
      if(!raw){
        if(relay[i].alarmid<10)Serial.print(" ");
        if(relay[i].alarmid<100)Serial.print(" ");
        if(relay[i].alarmid<1000)Serial.print(" ");
        if(relay[i].alarmid<10000)Serial.print(" ");
      }
      raw?Serial.println():Serial.println("  |");
      
}


/************************************************************************/
void setclock(){
  //read from rtc, set arduino time
  
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print("  Day_of_week:");
  Serial.println(dayOfWeek, DEC);
  
  setTime(hour,minute,second,dayOfMonth,month,year);

}
/************************************************************************/
/************************************************************************/
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}


/************************************************************************/
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

// Stops the DS1307, but it has the side effect of setting seconds to 0
// Probably only want to use this for testing
/*void stopDs1307()
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(0x80);
  Wire.endTransmission();
}*/


/************************************************************************/
// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers
void setDateDs1307(byte second,        // 0-59
                   byte minute,        // 0-59
                   byte hour,          // 1-23
                   byte dayOfWeek,     // 1-7
                   byte dayOfMonth,    // 1-28/29/30/31
                   byte month,         // 1-12
                   byte year)          // 0-99
{
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write(0);
   Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
   Wire.write(decToBcd(minute));
   Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
                                   // bit 6 (also need to change readDateDs1307)
   Wire.write(decToBcd(dayOfWeek));
   Wire.write(decToBcd(dayOfMonth));
   Wire.write(decToBcd(month));
   Wire.write(decToBcd(year));
   Wire.endTransmission();
}


/************************************************************************/
// Gets the date and time from the ds1307
void getDateDs1307(byte *second,
          byte *minute,
          byte *hour,
          byte *dayOfWeek,
          byte *dayOfMonth,
          byte *month,
          byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}




