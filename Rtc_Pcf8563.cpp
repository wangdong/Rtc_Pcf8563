/*****
 *  NAME
 *    Pcf8563 Real Time Clock support routines
 *  AUTHOR
 *    Joe Robertson, jmr
 *    orbitalair@bellsouth.net
 *    http://orbitalair.wikispaces.com/Arduino
 *  CREATION DATE
 *    9/24/06,  init - built off of usart demo.  using mikroC
 *  NOTES
 *  HISTORY
 *    10/14/06 ported to CCS compiler, jmr
 *    2/21/09  changed all return values to hex val and not bcd, jmr
 *    1/10/10  ported to arduino, jmr
 *    2/14/10  added 3 world date formats, jmr
 *    28/02/2012 A. Pasotti
 *             fixed a bug in RTCC_ALARM_AF,
 *             added a few (not really useful) methods

 *  TODO
 *    x Add Euro date format
 *    Add short time (hh:mm) format
 *    Add 24h/12h format
 ******
 *  Robodoc embedded documentation.
 *  http://www.xs4all.nl/~rfsber/Robo/robodoc.html
 */

#include "Arduino.h"
#include "Wire.h"
#include "Rtc_Pcf8563.h"

Rtc_Pcf8563::Rtc_Pcf8563(void)
{
    Wire.begin();
    Rtcc_Addr = RTCC_R>>1;
}

void Rtc_Pcf8563::initClock()
{
  Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
  Wire.write((byte)0x0);        // start address

  Wire.write((byte)0x0);     //control/status1
  Wire.write((byte)0x0);     //control/status2
  Wire.write((byte)0x01);     //set seconds
  Wire.write((byte)0x01);    //set minutes
  Wire.write((byte)0x01);    //set hour
  Wire.write((byte)0x01);    //set day
  Wire.write((byte)0x01);    //set weekday
  Wire.write((byte)0x01);     //set month, century to 1
  Wire.write((byte)0x01);    //set year to 99
  Wire.write((byte)0x80);    //minute alarm value reset to 00
  Wire.write((byte)0x80);    //hour alarm value reset to 00
  Wire.write((byte)0x80);    //day alarm value reset to 00
  Wire.write((byte)0x80);    //weekday alarm value reset to 00
  Wire.write((byte)0x0);     //set SQW, see: setSquareWave
  Wire.write((byte)0x0);     //timer off
  Wire.endTransmission();

}

/* Private internal functions, but useful to look at if you need a similar func. */
byte Rtc_Pcf8563::decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

byte Rtc_Pcf8563::bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}


void Rtc_Pcf8563::clearStatus()
{
  Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
  Wire.write((byte)0x0);
  Wire.write((byte)0x0);                 //control/status1
  Wire.write((byte)0x0);                 //control/status2
  Wire.endTransmission();
}

void Rtc_Pcf8563::setTime(byte hour, byte minute, byte sec)
{
  Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
  Wire.write((byte)RTCC_SEC_ADDR);       // send addr low byte, req'd

  Wire.write((byte)decToBcd(sec));         //set seconds
  Wire.write((byte)decToBcd(minute));    //set minutes
  Wire.write((byte)decToBcd(hour));        //set hour
  Wire.endTransmission();

}

void Rtc_Pcf8563::setDate(byte day, byte weekday, byte mon, byte century, byte year)
{
    /* year val is 00 to 99, xx
        with the highest bit of month = century
        0=20xx
        1=19xx
        */
    month = decToBcd(mon);
    if (century == 1){
        month |= RTCC_CENTURY_MASK;
    }
    else {
        month &= ~RTCC_CENTURY_MASK;
    }
    Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    Wire.write((byte)RTCC_DAY_ADDR);
    Wire.write((byte)decToBcd(day));            //set day
    Wire.write((byte)decToBcd(weekday));    //set weekday
    Wire.write((byte)month);                         //set month, century to 1
    Wire.write((byte)decToBcd(year));        //set year to 99
    Wire.endTransmission();
}

/* enable alarm interrupt
 * whenever the clock matches these values an int will
 * be sent out pin 3 of the Pcf8563 chip
 */
void Rtc_Pcf8563::enableAlarm()
{

    //set status2 AF val to zero
    status2 &= ~RTCC_ALARM_AF;
    //enable the interrupt
    status2 |= RTCC_ALARM_AIE;

    //enable the interrupt
    Wire.beginTransmission(Rtcc_Addr);  // Issue I2C start signal
    Wire.write((byte)RTCC_STAT2_ADDR);
    Wire.write((byte)status2);
    Wire.endTransmission();
}


/*
* Read status byte
*/
byte Rtc_Pcf8563::readStatus2()
{
    /* set the start byte of the status 2 data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_STAT2_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 1); //request 1 bytes
    return Wire.read();
}

/*
* Returns true if AIE is on
*
*/
boolean Rtc_Pcf8563::alarmEnabled()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AIE;
}

/*
* Returns true if AF is on
*
*/
boolean Rtc_Pcf8563::alarmActive()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AF;
}


/* set the alarm values
 * whenever the clock matches these values an int will
 * be sent out pin 3 of the Pcf8563 chip
 */
void Rtc_Pcf8563::setAlarm(byte min, byte hour, byte day, byte weekday)
{
    if (min <99) {
        min = constrain(min, 0, 59);
        min = decToBcd(min);
        min &= ~RTCC_ALARM;
    } else {
        min = 0x0; min |= RTCC_ALARM;
    }

    if (hour <99) {
        hour = constrain(hour, 0, 23);
        hour = decToBcd(hour);
        hour &= ~RTCC_ALARM;
    } else {
        hour = 0x0; hour |= RTCC_ALARM;
    }

    if (day <99) {
        day = constrain(day, 1, 31);
        day = decToBcd(day); day &= ~RTCC_ALARM;
    } else {
        day = 0x0; day |= RTCC_ALARM;
    }

    if (weekday <99) {
        weekday = constrain(weekday, 0, 6);
        weekday = decToBcd(weekday);
        weekday &= ~RTCC_ALARM;
    } else {
        weekday = 0x0; weekday |= RTCC_ALARM;
    }

    Rtc_Pcf8563::enableAlarm();

    //TODO bounds check  the inputs first
    Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    Wire.write((byte)RTCC_ALRM_MIN_ADDR);
    Wire.write((byte)min);                //minute alarm value reset to 00
    Wire.write((byte)hour);                //hour alarm value reset to 00
    Wire.write((byte)day);                //day alarm value reset to 00
    Wire.write((byte)weekday);            //weekday alarm value reset to 00
    Wire.endTransmission();
}

/**
* Get alarm, set values to RTCC_NO_ALARM (99) if alarm flag is not set
*/
void Rtc_Pcf8563::getAlarm()
{
    /* set the start byte of the alarm data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_ALRM_MIN_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 4); //request 4 bytes
    alarm_minute = Wire.read();
    if(B10000000 & alarm_minute){
        alarm_minute = RTCC_NO_ALARM;
    } else {
        alarm_minute = bcdToDec(alarm_minute & B01111111);
    }
    alarm_hour = Wire.read();
    if(B10000000 & alarm_hour){
        alarm_hour = RTCC_NO_ALARM;
    } else {
        alarm_hour = bcdToDec(alarm_hour & B00111111);
    }
    alarm_day = Wire.read();
    if(B10000000 & alarm_day){
        alarm_day = RTCC_NO_ALARM;
    } else {
        alarm_day = bcdToDec(alarm_day  & B00111111);
    }
    alarm_weekday = Wire.read();
    if(B10000000 & alarm_weekday){
        alarm_weekday = RTCC_NO_ALARM;
    } else {
        alarm_weekday = bcdToDec(alarm_weekday  & B00000111);
    }
}

/**
* Set the square wave pin output
*/
void Rtc_Pcf8563::setSquareWave(byte frequency)
{
    Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    Wire.write((byte)RTCC_SQW_ADDR);
    Wire.write((byte)frequency);
    Wire.endTransmission();
}

void Rtc_Pcf8563::clearSquareWave()
{
    Rtc_Pcf8563::setSquareWave(SQW_DISABLE);
}

/**
* Reset the alarm leaving interrupt unchanged
*/
void Rtc_Pcf8563::resetAlarm()
{
    //set status2 AF val to zero to reset alarm
    status2 &= ~RTCC_ALARM_AF;
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_STAT2_ADDR);
    Wire.write((byte)status2);
    Wire.endTransmission();
}


void Rtc_Pcf8563::clearAlarm()
{
    //set status2 AF val to zero to reset alarm
    status2 &= ~RTCC_ALARM_AF;
    //turn off the interrupt
    status2 &= ~RTCC_ALARM_AIE;

    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_STAT2_ADDR);
    Wire.write((byte)status2);
    Wire.endTransmission();
}


/**
* Setup the timer
*/
byte Rtc_Pcf8563::getTimerFrequency() 
{
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_TIMER_CONTROL_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 1);
    return Wire.read();
}

byte Rtc_Pcf8563::getTimer() 
{
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_TIMER_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 1);
    return Wire.read();
}

void Rtc_Pcf8563::setTimer(byte frequency, byte count, bool enableInterruption)
{
    status2 &= ~(RTCC_ALARM_TF | RTCC_ALARM_TIE);
    if (enableInterruption)
        status2 |= RTCC_ALARM_TIE;

    Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    Wire.write((byte)RTCC_STAT2_ADDR);    // Clear TF and/or enable the timer interruption
    Wire.write((byte)status2);
    Wire.endTransmission();

    Wire.beginTransmission(Rtcc_Addr);    // Issue I2C start signal
    Wire.write((byte)RTCC_TIMER_CONTROL_ADDR);
    Wire.write((byte)frequency);
    Wire.write((byte)count);
    Wire.endTransmission();
}

void Rtc_Pcf8563::clearTimer()
{
    setTimer(TIMER_DISABLE, 0, false);
}


boolean Rtc_Pcf8563::timerActive()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_TF;
}

boolean Rtc_Pcf8563::timerEnabled()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_TIE;
}

void Rtc_Pcf8563::getDate()
{
    /* set the start byte of the date data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_DAY_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 4); //request 4 bytes
    //0x3f = 0b00111111
    day = bcdToDec(Wire.read() & 0x3f);
    //0x07 = 0b00000111
    weekday = bcdToDec(Wire.read() & 0x07);
    //get raw month data byte and set month and century with it.
    month = Wire.read();
    if (month & RTCC_CENTURY_MASK) {
        century = 1;
    }
    else {
        century = 0;
    }
    //0x1f = 0b00011111
    month = month & 0x1f;
    month = bcdToDec(month);
year = bcdToDec(Wire.read());
}

void Rtc_Pcf8563::getTime()
{
    /* set the start byte , get the 2 status bytes */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.write((byte)RTCC_STAT1_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 5); //request 5 bytes
    status1 = Wire.read();
    status2 = Wire.read();
    //0x7f = 0b01111111
    sec = bcdToDec(Wire.read() & 0x7f);
    minute = bcdToDec(Wire.read() & 0x7f);
    //0x3f = 0b00111111
    hour = bcdToDec(Wire.read() & 0x3f);
}

char *Rtc_Pcf8563::formatTime(byte style)
{
    getTime();
    switch (style) {
        case RTCC_TIME_HM:
            strOut[0] = '0' + (hour / 10);
            strOut[1] = '0' + (hour % 10);
            strOut[2] = ':';
            strOut[3] = '0' + (minute / 10);
            strOut[4] = '0' + (minute % 10);
            strOut[5] = '\0';
            break;
        case RTCC_TIME_HMS:
        default:
            strOut[0] = '0' + (hour / 10);
            strOut[1] = '0' + (hour % 10);
            strOut[2] = ':';
            strOut[3] = '0' + (minute / 10);
            strOut[4] = '0' + (minute % 10);
            strOut[5] = ':';
            strOut[6] = '0' + (sec / 10);
            strOut[7] = '0' + (sec % 10);
            strOut[8] = '\0';
            break;
        }
    return strOut;
}


char *Rtc_Pcf8563::formatDate(byte style)
{
    getDate();

        switch (style) {

        case RTCC_DATE_ASIA:
            //do the asian style, yyyy-mm-dd
            if ( century == 1 ){
                strDate[0] = '1';
                strDate[1] = '9';
            }
            else {
                strDate[0] = '2';
                strDate[1] = '0';
            }
            strDate[2] = '0' + (year / 10 );
            strDate[3] = '0' + (year % 10);
            strDate[4] = '-';
            strDate[5] = '0' + (month / 10);
            strDate[6] = '0' + (month % 10);
            strDate[7] = '-';
            strDate[8] = '0' + (day / 10);
            strDate[9] = '0' + (day % 10);
            strDate[10] = '\0';
            break;
        case RTCC_DATE_US:
        //the pitiful US style, mm/dd/yyyy
            strDate[0] = '0' + (month / 10);
            strDate[1] = '0' + (month % 10);
            strDate[2] = '/';
            strDate[3] = '0' + (day / 10);
            strDate[4] = '0' + (day % 10);
            strDate[5] = '/';
            if ( century == 1 ){
                strDate[6] = '1';
                strDate[7] = '9';
            }
            else {
                strDate[6] = '2';
                strDate[7] = '0';
            }
            strDate[8] = '0' + (year / 10 );
            strDate[9] = '0' + (year % 10);
            strDate[10] = '\0';
            break;
        case RTCC_DATE_WORLD:
        default:
            //do the world style, dd-mm-yyyy
            strDate[0] = '0' + (day / 10);
            strDate[1] = '0' + (day % 10);
            strDate[2] = '-';
            strDate[3] = '0' + (month / 10);
            strDate[4] = '0' + (month % 10);
            strDate[5] = '-';

            if ( century == 1 ){
                strDate[6] = '1';
                strDate[7] = '9';
            }
            else {
                strDate[6] = '2';
                strDate[7] = '0';
            }
            strDate[8] = '0' + (year / 10 );
            strDate[9] = '0' + (year % 10);
            strDate[10] = '\0';
            break;

    }
    return strDate;
}

byte Rtc_Pcf8563::getSecond() {
    return sec;
}

byte Rtc_Pcf8563::getMinute() {
    return minute;
}

byte Rtc_Pcf8563::getHour() {
    return hour;
}

byte Rtc_Pcf8563::getAlarmMinute() {
    return alarm_minute;
}

byte Rtc_Pcf8563::getAlarmHour() {
    return alarm_hour;
}

byte Rtc_Pcf8563::getAlarmDay() {
    return alarm_day;
}

byte Rtc_Pcf8563::getAlarmWeekday() {
    return alarm_weekday;
}

byte Rtc_Pcf8563::getDay() {
    return day;
}

byte Rtc_Pcf8563::getMonth() {
    return month;
}

byte Rtc_Pcf8563::getYear() {
    return year;
}

byte Rtc_Pcf8563::getWeekday() {
    return weekday;
}

byte Rtc_Pcf8563::getStatus1() {
    return status1;
}

byte Rtc_Pcf8563::getStatus2() {
    return status2;
}


//
// END OF FILE
//

