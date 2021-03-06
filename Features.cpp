/* 
 * Implementation of various clock features (complications) 
 *
 * This source file is part of the Nixie Clock Arduino firmware
 * found under http://www.github.com/microfarad-de/nixie-clock
 * 
 * Please visit:
 *   http://www.microfarad.de
 *   http://www.github.com/microfarad-de
 *   
 * Copyright (C) 2019 Karim Hraibi (khraibi at gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Features.h"
#include "src/MathMf/MathMf.h"

#define TIMER_ALARM_DURATION      (10 * 60000)
#define TIMER_RESET_TIMEOUT       (10000)
#define ALARM_ALARM_DURATION      (30 * 60000)
#define ALARM_SNOOZE_DURATION     (8 * 60000)



/*#######################################################################################*/

BuzzerClass Buzzer;

void BuzzerClass::initialize (uint8_t buzzerPin) {
  this->buzzerPin = buzzerPin;
  pinMode (buzzerPin, OUTPUT);
  digitalWrite (buzzerPin, LOW);
  active = false;
  index = 0;
  initialized = true;
}

void BuzzerClass::loopHandler (void) {
  uint32_t ts;
  
  if (!initialized || !active ) return;

  ts = millis ();
  
  if (ts - melodyTs > (uint32_t)melody[index]) {
    digitalWrite (buzzerPin, !digitalRead (buzzerPin));
    melodyTs = ts;
    index++;
    if (melody[index] < 0) index = 0;
  }
}

void BuzzerClass::playMelody1 (void) {
  if (!initialized || active) return;
  active = true;
  melody = melody1;
  melodyTs = millis () - 5000;
  index = 0;
}

void BuzzerClass::playMelody2 (void) {
  if (!initialized || active) return;
  active = true;
  melody = melody2;
  melodyTs = millis () - 5000;
  index = 0;
}

void BuzzerClass::stop (void) {
  active = false;
  index = 0;
  digitalWrite (buzzerPin, LOW);
}


/*#######################################################################################*/

void ChronoClass::increment10th (void) {
  tenth++;
  if (tenth > 9) tenth = 0, second++;
  if (second > 59) second = 0, minute++;
  if (minute > 59) minute = 0, hour++;
}

void ChronoClass::increment10sec (void) {
  second += 10;
  if (second > 59) second = 0, minute++;
  if (minute > 59) minute = 0, hour++;
}

bool ChronoClass::decrement10sec (void) {
  second -= 10;
  if (second < 0) second = 59, minute--;
  if (minute < 0) minute = 59, hour--;
  if (hour < 0) hour = 0, minute = 0, second = 0;
  if (hour == 0 && minute == 0 && second == 0) return true;
  return false;
}

void ChronoClass::incrementMin (void) {
  minute++;
  if (minute > 59) minute = 0, hour++;
}

bool ChronoClass::decrementMin (void) {
  minute--;
  if (minute < 0) minute = 59, hour--;
  if (hour < 0) hour = 0, minute = 0, second = 0;
  if (hour == 0 && minute == 0 && second == 0) return true;
  return false;
}

void ChronoClass::incrementSec (void) {
  second++;
  if (second > 59) second = 0, minute++;
  if (minute > 59) minute = 0, hour++;
}

bool ChronoClass::decrementSec (void) {
  second--;
  if (second < 0) second = 59, minute--;
  if (minute < 0) minute = 59, hour--;
  if (hour < 0) hour = 0, minute = 0, second = 0;
  if (hour == 0 && minute == 0 && second == 0) return true;
  return false;
}

void ChronoClass::reset (void) {
  tenth = 0;
  second = 0;
  minute = 0;
  hour = 0;
}

void ChronoClass::copy (ChronoClass *tm) {
  tenth = tm->tenth;
  second = tm->second;
  minute = tm->minute;
  hour = tm->hour;
}

void ChronoClass::roundup (void) {
  tenth = 0;
  if (second !=0) second = 0, minute++;
  if (minute > 59) minute = 0, hour++;
  if (second == 0 && minute == 0) minute = 1;
}

/*#######################################################################################*/

void CdTimerClass::initialize (void (*callback)(bool)) {
  this->callback = callback;
  defaultTm.tenth = 0;
  defaultTm.second = 0;
  defaultTm.minute = 5;
  defaultTm.hour = 0;
  reset ();
}

void CdTimerClass::loopHandler (void) {
  static uint32_t alarmTs = 0;
  static uint32_t resetTs = 0;
  uint32_t ts = millis ();
  
  if (tickFlag) { 
    if (!alarm) {
      alarm = tm.decrementSec (); 
      if (alarm) {
        alarmTs = ts;
        tm.copy (&defaultTm);
        Nixie.resetBlinking ();
        Nixie.blinkAll (true);
        Buzzer.playMelody2 ();
      }
    }
    else {
      tm.incrementSec ();
    }
    displayRefresh ();
    tickFlag = false;
  }
  
  if (alarm && ts - alarmTs > TIMER_ALARM_DURATION) resetAlarm ();
  
  if (running) {
    resetTs = ts;
  }
  else if (active && !running) {
    if (ts - resetTs > TIMER_RESET_TIMEOUT) reset ();
  }
}

void CdTimerClass::tick (void) {
  if (running) {  
    tickFlag = true;
  }
}

void CdTimerClass::secondIncrease (void) {
  tm.increment10sec ();
  displayRefresh ();
  defaultTm.copy (&tm);
}

void CdTimerClass::secondDecrease (void) {
  tm.decrement10sec ();
  displayRefresh ();
  defaultTm.copy (&tm);
}

void CdTimerClass::minuteIncrease (void) {
  tm.incrementMin ();
  displayRefresh ();
  defaultTm.copy (&tm);
}

void CdTimerClass::minuteDecrease (void) {
  tm.decrementMin ();
  displayRefresh ();
  defaultTm.copy (&tm);
}

void CdTimerClass::displayRefresh (void) {
  digits[0].value = dec2bcdLow  (tm.second);
  digits[1].value = dec2bcdHigh (tm.second);
  digits[2].value = dec2bcdLow  (tm.minute);
  digits[3].value = dec2bcdHigh (tm.minute);
  digits[4].value = dec2bcdLow  (tm.hour);
  digits[5].value = dec2bcdHigh (tm.hour); 
}

void CdTimerClass::start (void) {
  if (!running) {
    active = true;
    running = true;
    callback (true);
  }  
}

void CdTimerClass::stop (void) {
  if (running) {
    running = false;
    callback (false);    
  }
}

void CdTimerClass::resetAlarm (void) {
  if (alarm) {
    alarm = false;
    Nixie.blinkAll (false);
    Buzzer.stop ();
    stop ();
  }
}

void CdTimerClass::reset (void) {
  resetAlarm ();
  active = false;
  running = false;
  Nixie.resetDigits (digits, NIXIE_NUM_TUBES);
  callback (false);
  defaultTm.roundup ();
  tm.copy (&defaultTm);
  displayRefresh ();
}

/*#######################################################################################*/

void StopwatchClass::initialize (void (*callback)(bool reset)) {
  this->callback = callback;
  reset ();
}

void StopwatchClass::loopHandler (void) {
  if (tickFlag) {
    tm.increment10th ();
    if (!paused) displayRefresh ();
    if (tm.hour > 1) {
      tm.hour = 1; tm.minute = 59; tm.second = 59; tm.tenth = 9;
      stop ();
    }
    tickFlag = false;
  }
}

void StopwatchClass::tick (void) {
  if (running) {  
    tickFlag = true;
  }
}

void StopwatchClass::start (void) {
  active = true;
  running = true;
  callback (true);  
}

void StopwatchClass::stop (void) {
  running = false;
  pause (false);
  callback (false);
}

void StopwatchClass::pause (bool enable) {
  uint8_t i;
  if (enable && running) {
    paused = true;
    Nixie.resetBlinking ();
    for (i = 0; i < NIXIE_NUM_TUBES; i++) digits[i].blink = true;  
  }
  else {
    paused = false;
    displayRefresh ();
    for (i = 0; i < NIXIE_NUM_TUBES; i++) digits[i].blink = false;
  }
  
}

void StopwatchClass::displayRefresh (void) {
  digits[0].value = 0;
  digits[1].value = dec2bcdLow  (tm.tenth);
  digits[2].value = dec2bcdLow  (tm.second);
  digits[3].value = dec2bcdHigh (tm.second);
  digits[4].value = dec2bcdLow  (tm.minute);
  digits[5].value = dec2bcdHigh (tm.minute); 
  if (tm.hour > 0) digits[4].comma = true;
}

void StopwatchClass::reset (void) {
  uint8_t i;
  active = false;
  running = false;
  paused = false;
  Nixie.resetDigits (digits, NIXIE_NUM_TUBES);
  for (i = 0; i < NIXIE_NUM_TUBES; i++) digits[i].blink = false;
  tm.reset ();
  displayRefresh ();
  callback (false);
}

/*#######################################################################################*/

void AlarmClass::initialize (AlarmEeprom_s *settings) {
  this->settings = settings;
  alarm = false;
  snoozing = false;
  Nixie.blinkAll (false);
  if (settings->minute < 0 || settings->minute > 59) settings->minute = 0;
  if (settings->hour < 0 || settings->hour > 23) settings->hour = 0;
  if (settings->mode != ALARM_OFF && settings->mode != ALARM_WEEKENDS && 
      settings->mode != ALARM_WEEKDAYS && settings->mode != ALARM_DAILY) settings->mode = ALARM_OFF;
  if (settings->lastMode != ALARM_OFF && settings->lastMode != ALARM_WEEKENDS && 
      settings->lastMode != ALARM_WEEKDAYS && settings->lastMode != ALARM_DAILY) settings->lastMode = ALARM_DAILY;
  displayRefresh ();
}

void AlarmClass::loopHandler (int8_t hour, int8_t minute, int8_t wday, bool active) {
  uint32_t ts = millis();

  if (minute != lastMinute) alarmCondition = false;
  
  if (active && !snoozing && settings->mode != ALARM_OFF && !alarmCondition &&
      minute == settings->minute && hour == settings->hour && 
      (settings->mode != ALARM_WEEKDAYS || (wday >= 1 && wday <= 5)) && 
      (settings->mode != ALARM_WEEKENDS || wday == 0 || wday == 6)) {
    startAlarm (); 
    alarmCondition = true;
  }

  if (snoozing && ts - snoozeTs > ALARM_SNOOZE_DURATION) startAlarm ();

  if (snoozing && ts - blinkTs > 500) {
    Nixie.comma[0] = !Nixie.comma[0];
    blinkTs = ts;
  }

  // Alarm timout
  // IMPORTANT: use millis() not ts to avoid premature alarm reset
  // due to a boundary condition where alarmTs > ts
  // (alarmTs is set within startAlarm() which is called after ts was assigned)
  if (alarm && millis() - alarmTs > ALARM_ALARM_DURATION) resetAlarm ();

  lastMinute = minute;
}

void AlarmClass::startAlarm (void) {
  if (!alarm) {
    alarm = true;
    Nixie.resetBlinking ();
    Nixie.blinkAll (true);
    Buzzer.playMelody1 ();
    alarmTs = millis ();
    snoozing = false;
    displayRefresh ();
  }
}

void AlarmClass::snooze (void) {
  if (alarm && !snoozing) {
    alarm = false;
    Nixie.blinkAll (false);
    Buzzer.stop ();
    snoozing = true;
    snoozeTs = millis ();
    displayRefresh ();
  }
}

void AlarmClass::resetAlarm (void) {
  if (alarm || snoozing) {
    alarm = false;
    Nixie.blinkAll (false);
    Buzzer.stop ();
    snoozing = false;
    displayRefresh ();
  }
}

void AlarmClass::modeIncrease (void) {
  if      (settings->mode == ALARM_OFF)      settings->mode = ALARM_WEEKENDS;
  else if (settings->mode == ALARM_WEEKENDS) settings->mode = ALARM_WEEKDAYS;
  else if (settings->mode == ALARM_WEEKDAYS) settings->mode = ALARM_DAILY;
  else if (settings->mode == ALARM_DAILY)    settings->mode = ALARM_OFF, settings->lastMode = ALARM_DAILY;
  displayRefresh ();
}

void AlarmClass::modeDecrease (void) {
  if      (settings->mode == ALARM_OFF)      settings->mode = ALARM_DAILY;
  else if (settings->mode == ALARM_DAILY)    settings->mode = ALARM_WEEKDAYS;
  else if (settings->mode == ALARM_WEEKDAYS) settings->mode = ALARM_WEEKENDS;
  else if (settings->mode == ALARM_WEEKENDS) settings->mode = ALARM_OFF, settings->lastMode = ALARM_DAILY;
  displayRefresh ();
}

void AlarmClass::modeToggle (void) {
  if (settings->mode == ALARM_OFF) {
    settings->mode = settings->lastMode;
  }
  else {
    settings->lastMode = settings->mode;
    settings->mode = ALARM_OFF;
  }
  displayRefresh ();
}

void AlarmClass::minuteIncrease (void) {
  settings->minute++;
  if (settings->minute > 59) settings->minute = 0;
  displayRefresh ();
}

void AlarmClass::minuteDecrease (void) {
  settings->minute--;
  if (settings->minute < 0) settings->minute = 59;
  displayRefresh ();
}

void AlarmClass::hourIncrease (void) {
  settings->hour++;
  if (settings->hour > 23) settings->hour = 0;
  displayRefresh ();
}

void AlarmClass::hourDecrease (void) {
  settings->hour--;
  if (settings->hour < 0) settings->hour = 23;
  displayRefresh ();
}

void AlarmClass::displayRefresh (void) {
  for (uint8_t i = 0; i < NIXIE_NUM_TUBES; i++) digits[i].blank = false;
  digits[0].value = (uint8_t)settings->mode;
  Nixie.comma[0] = (bool)settings->mode;
  digits[1].blank = true;
  digits[2].value = dec2bcdLow  (settings->minute);
  digits[3].value = dec2bcdHigh (settings->minute);
  digits[4].value = dec2bcdLow  (settings->hour);
  digits[5].value = dec2bcdHigh (settings->hour);   
}





