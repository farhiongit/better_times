
/** @file dates.h
 * All functions are Multi Thread safe (MT-Safe env, see attributes(7)), safe to call in the presence of other threads), unless specified.
 */
/*******
 * Copyright 2019 Laurent Farhi
 *
 *  This file is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this file.  If not, see <http://www.gnu.org/licenses/>.
 *****/
#ifndef TM_DATES_H
#  define TM_DATES_H
#  pragma once

#  include "vfunc.h"

typedef enum
{
  TM_OK,                        // Success
  TM_ERROR,                     // Error
} tm_status;

typedef enum
{
  TM_MONDAY = 1,                // Monday (1)
  TM_TUESDAY,                   // Tuesday (2)
  TM_WEDNESDAY,                 // Wednesday (3)
  TM_THURSDAY,                  // Thursday (4)
  TM_FRIDAY,                    // Friday (5)
  TM_SATURDAY,                  // Saturday (6)
  TM_SUNDAY,                    // Sunday (7)
} tm_dayofweek;

typedef enum
{
  TM_JANUARY = 1,               // January (1)
  TM_FEBRUARY,                  // February (2)
  TM_MARCH,                     // March (3)
  TM_APRIL,                     // April (4)
  TM_MAY,                       // May (5)
  TM_JUNE,                      // June (6)
  TM_JULY,                      // July (7)
  TM_AUGUST,                    // August (8)
  TM_SEPTEMBER,                 // September (9)
  TM_OCTOBER,                   // October (10)
  TM_NOVEMBER,                  // November (11)
  TM_DECEMBER,                  // December (12)
} tm_month;

// Daylight saving time automatic management modes
typedef enum
{
  TM_ST_OVER_DST,               // Favor standard time during summer to winter change
  TM_DST_OVER_ST,               // Favor daylight saving time during summer to winter change
} tm_time_precedence;

typedef enum
{
  TM_NOW,
  TM_TODAY,                     // Today at midnight (for a specified referntial)
} tm_predefined_instant;

// Set to optimize performance in case TZ is not used anywhere else.
extern int tm_is_TZ_owner;

// Date and time referentials
extern const char *const TM_REF_SYSTEMTIME;     // Local time representation as defined by the system
extern const char *const TM_REF_LOCALTIME;      // Local time representation as defined by the user with tm_setlocaltimezone (TM_REF_SYSTEMTIME by default)
extern const char *const TM_REF_UTC;    // Coordinated Universal Time representation
extern const char *const TM_REF_UNCHANGED;
extern const char *const TM_REF_UNDEFINED;

tm_status tm_setlocalwallclock (const char *);  // Thread safety : MT-Unsafe const:env (see attributes(7))
const char *tm_getlocalwallclock (void);
int tm_islocalwallclock (const char *);

tm_status tm_make_dtrc (struct tm *dt, int year, tm_month month, int day, int hour, int min, int sec, const char *, tm_time_precedence);
#  define tm_make9(dt, YYYY, MM, DD, hh, mm, ss, rep, precedence) tm_make_dtrc(dt, YYYY, MM, DD, hh, mm, ss, rep, precedence)
#  define tm_make8(dt, YYYY, MM, DD, hh, mm, ss, rep) tm_make9(dt, YYYY, MM, DD, hh, mm, ss, rep, TM_ST_OVER_DST)
#  define tm_make7(dt, YYYY, MM, DD, hh, mm, ss) tm_make8(dt, YYYY, MM, DD, hh, mm, ss, TM_REF_LOCALTIME)
tm_status tm_make_ir (struct tm *, tm_predefined_instant, const char *);
#  define tm_make3(dt, instant, rep) tm_make_ir (dt, instant, rep)
#  define tm_make2(dt, instant) tm_make3 (dt, instant, TM_REF_LOCALTIME)
#  define tm_make1(dt) tm_make2 (dt, TM_NOW)
#  define tm_set(...) VFUNC (tm_make, __VA_ARGS__)

tm_status tm_settimefromstring (struct tm *dt, const char *str, const char *wc, tm_time_precedence);
#  define tm_settimefromstring4(dt, text, rep, precedence) tm_settimefromstring(dt, text, rep, precedence)
#  define tm_settimefromstring3(dt, text, rep) tm_settimefromstring4(dt, text, rep, TM_ST_OVER_DST)
#  define tm_settimefromstring2(dt, text) tm_settimefromstring3(dt, text, TM_REF_UNCHANGED)
#  define tm_settimefromstring(...) VFUNC(tm_settimefromstring, __VA_ARGS__)

tm_status tm_setdatefromstring (struct tm *dt, const char *str, const char *wc, tm_time_precedence);
#  define tm_setdatefromstring4(dt, text, rep, precedence) tm_setdatefromstring(dt, text, rep, precedence)
#  define tm_setdatefromstring3(dt, text, rep) tm_setdatefromstring4(dt, text, rep, TM_ST_OVER_DST)
#  define tm_setdatefromstring2(dt, text) tm_setdatefromstring3(dt, text, TM_REF_UNCHANGED)
#  define tm_setdatefromstring(...) VFUNC(tm_setdatefromstring, __VA_ARGS__)

tm_status tm_setfromiso8601 (struct tm *dt, char *str);

const char *tm_getwallclock (struct tm date);
int tm_isdefinedinwallclock (struct tm, const char *);
#  define tm_isdefinedinutc(tm) tm_isdefinedinwallclock(tm, TM_REF_UTC)
#  define tm_isdefinedinsystemtime(tm) tm_isdefinedinwallclock(tm, TM_REF_SYSTEMTIME)
#  define tm_isdefinedinlocaltime(tm) tm_isdefinedinwallclock(tm, TM_REF_LOCALTIME)

tm_status tm_changetowallclock (struct tm *date, const char *);
#  define tm_changetoutc(date) tm_changetowallclock(date, TM_REF_UTC)
#  define tm_changetosystemtime(date) tm_changetowallclock(date, TM_REF_SYSTEMTIME)
#  define tm_changetolocaltime(date) tm_changetowallclock(date, TM_REF_LOCALTIME)

int tm_getyear (struct tm date);        // On 4 digits
tm_month tm_getmonth (struct tm date);
int tm_getday (struct tm date); // Starting from 1
int tm_gethour (struct tm date);        // Starting from 0
int tm_getminute (struct tm date);      // Starting from 0
int tm_getsecond (struct tm date);      // Starting from 0

int tm_getdayofyear (struct tm date);   // Starting from 1
tm_dayofweek tm_getdayofweek (struct tm date);
int tm_getisoweek (struct tm date);     // Starting from 1
int tm_getisoyear (struct tm date);     // On 4 digits
int tm_getsecondsofday (struct tm date);        // Starting from 0
#  define tm_getminutesofday(date) ((int)(tm_getsecondsofday(date)) / 60)
#  define tm_gethoursofday(date) ((int)(tm_getminutesofday(date)) / 60)

int tm_getutcoffset (struct tm date);   // In seconds
int tm_isdaylightsavingtimeineffect (struct tm date);
int tm_isinsidedaylightsavingtimeoverlap (struct tm date);

int tm_compare (const void *dta, const void *dtb);
int tm_equals (struct tm a, struct tm b);

tm_status tm_datetostring (struct tm dt, size_t max, char *str);
tm_status tm_timetostring (struct tm dt, size_t max, char *str);
tm_status tm_tostring (struct tm dt, size_t max, char *str);
tm_status tm_toiso8601 (struct tm dt, size_t max, char *str, int sep);
#  define tm_toiso86014(date, max, str, sep) tm_toiso8601(date, max, str, sep)
#  define tm_toiso86013(date, max, str) tm_toiso86014 (date, max, str, 0)
#  define tm_toiso8601(...) VFUNC(tm_toiso8601, __VA_ARGS__)

tm_status tm_addseconds (struct tm *date, long int nbSecs);
#  define tm_addminutes(date, nbMins) tm_addseconds (date, 60 * (int)(nbMins))
#  define tm_addhours(date, nbHours) tm_addminutes (date, 60 * (int)(nbHours))
tm_status tm_adddays (struct tm *date, int nbDays);
tm_status tm_addmonths (struct tm *date, int nbMonths);
#  define tm_addyears(date, nbYears) tm_addmonths (date, 12 * (int)(nbYears))
tm_status tm_trimtime (struct tm *date);

long int tm_diffseconds (struct tm start, struct tm stop);
#  define tm_diffminutes(start, stop) ((int)(tm_diffseconds(start, stop)) / 60)
#  define tm_diffhours(start, stop) ((int)(tm_diffminutes(start, stop)) / 60)
int tm_diffdays (struct tm start, struct tm stop, int *seconds);
#  define tm_diffdays3(start, stop, seconds) tm_diffdays (start, stop, seconds)
#  define tm_diffdays2(start, stop) tm_diffdays (start, stop, 0)
#  define tm_diffdays(...) VFUNC (tm_diffdays, __VA_ARGS__)
int tm_diffweeks (struct tm start, struct tm stop, int *days, int *seconds);
#  define tm_diffweeks4(start, stop, days, seconds) tm_diffweeks (start, stop, days, seconds)
#  define tm_diffweeks2(start, stop) tm_diffweeks (start, stop, 0, 0)
#  define tm_diffweeks(...) VFUNC (tm_diffweeks, __VA_ARGS__)
int tm_diffmonths (struct tm start, struct tm stop, int *days, int *seconds);
#  define tm_diffmonths4(start, stop, days, seconds) tm_diffmonths (start, stop, days, seconds)
#  define tm_diffmonths2(start, stop) tm_diffmonths (start, stop, 0, 0)
#  define tm_diffmonths(...) VFUNC (tm_diffmonths, __VA_ARGS__)
int tm_diffyears (struct tm start, struct tm stop, int *months, int *days, int *seconds);
#  define tm_diffyears5(start, stop, months, days, seconds) tm_diffyears (start, stop, months, days, seconds)
#  define tm_diffyears2(start, stop) tm_diffyears (start, stop, 0, 0, 0)
#  define tm_diffyears(...) VFUNC (tm_diffyears, __VA_ARGS__)

int tm_diffcalendardays (struct tm start, struct tm stop);
int tm_diffcalendarmonths (struct tm start, struct tm stop);
int tm_diffcalendaryears (struct tm start, struct tm stop);
int tm_diffisoyears (struct tm start, struct tm stop);

int tm_getdaysinyear (int year);
int tm_isleapyear (int year);
int tm_getweeksinisoyear (int isoyear);
int tm_getdaysinmonth (int year, tm_month month);
int tm_getsecondsinday (int year, tm_month month, int day, const char *);
#  define tm_getsecondsinday4(year, month, day, rep) tm_getsecondsinday(year, month, day, rep)
#  define tm_getsecondsinday3(year, month, day) tm_getsecondsinday4(year, month, day, TM_REF_LOCALTIME)
#  define tm_getsecondsinday(...) VFUNC (tm_getsecondsinday, __VA_ARGS__)
#  define tm_getminutesinday(...) ((int)(tm_getsecondsinday(__VA_ARGS__)) / 60)
#  define tm_gethoursinday(...) ((int)(tm_getminutesinday(__VA_ARGS__)) / 60)
int tm_getfirstweekdayinmonth (int year, tm_month month, tm_dayofweek dow);
int tm_getlastweekdayinmonth (int year, tm_month month, tm_dayofweek dow);
int tm_getfirstweekdayinisoyear (int isoyear, tm_dayofweek dow);

time_t tm_tobinary (struct tm);
tm_status tm_frombinary (struct tm *, time_t binary, const char *);
#  define tm_frombinary3(date, instant, rep) tm_frombinary(date, instant, rep)
#  define tm_frombinary2(date, instant) tm_frombinary3(date, instant, TM_REF_LOCALTIME)
#  define tm_frombinary(...) VFUNC (tm_frombinary, __VA_ARGS__)

tm_status dt_tostring (struct tm dt, size_t max, char *str);
tm_status dt_toiso8601 (struct tm dt, size_t max, char *str, int sep);
#  define dt_set4(date, YYYY, MM, DD)             tm_make8(date, YYYY, MM, DD, 0, 0, 0, TM_REF_UTC)
tm_status dt_make_ir (struct tm *date, const char *wc);
#  define dt_set2(date, rep)                      dt_make_ir (date, rep)
#  define dt_set1(date)                           dt_set2 (date, TM_REF_LOCALTIME)
#  define dt_set(...)                             VFUNC (dt_set, __VA_ARGS__)
#  define dt_setfromstring(date, text)            tm_setdatefromstring(date, text, TM_REF_UTC)
tm_status dt_setfromiso8601 (struct tm *dt, char *str);
#  define dt_equals                               tm_equals
#  define dt_getyear                              tm_getyear    // On 4 digits
#  define dt_getmonth                             tm_getmonth
#  define dt_getday                               tm_getday     // Starting from 1
#  define dt_getdayofyear                         tm_getdayofyear
                                                                // Starting from 1
#  define dt_getdayofweek                         tm_getdayofweek
#  define dt_getisoweek                           tm_getisoweek // Starting from 1
#  define dt_getisoyear                           tm_getisoyear // On 4 digits
#  define dt_adddays                              tm_adddays
#  define dt_addmonths                            tm_addmonths
#  define dt_addyears                             tm_addyears
#  define dt_compare                              tm_compare
#  define dt_diffdays(start, stop)                tm_diffdays (start, stop, 0)
#  define dt_diffweeks(start, stop, days)         tm_diffweeks (start, stop, days, 0)
#  define dt_diffmonths(start, stop, days)        tm_diffmonths (start, stop, days, 0)
#  define dt_diffyears(start, stop, months, days) tm_diffyears (start, stop, months, days, 0)
#  define dt_diffcalendardays                     tm_diffcalendardays
#  define dt_diffcalendarmonths                   tm_diffcalendarmonths
#  define dt_diffcalendaryears                    tm_diffcalendaryears
#  define dt_diffisoyears                         tm_diffisoyears
#  define dt_tobinary(date)                       (366 * dt_getyear(date) + dt_getdayofyear(date) - 1)
#  define dt_frombinary(date, binary)             ((dt_set ((date), (binary) / 366, 1, 1) == TM_OK && \
                                                    dt_adddays ((date), (binary) % 366) == TM_OK) \
                                                   ? TM_OK : TM_ERROR)

#endif
