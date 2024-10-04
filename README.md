> This project allows to manage UTC, local time and timezones conveniently in C, with a correct handling of daylight saving time.
  It offers a complete set of functions for definition and calculation on date and time, based on the usual structure `struct tm` of `time.h`.

# Introduction


Dealing with date and time in C is not easy. The C/POSIX time handling API (`time.h`) is a real mess (see http://www.catb.org/esr/time-programming/) and is far from user-friendly.

It's very low level and difficult to understand and to use.
The object `struct tm` follows unsual and subtle conventions, is counter-intuitive and needs translation to concepts as simple as days, months and years to name a few.
The use of functions such as `mktime`, `localtime` is not straight forward.

Nervertheless, it gets everything needed to handle UTC, localtime (as chosen by the user) and other timezones properly, and to alternate between those referentials
(actually and fortunately, Berkerley in BSD, and later GNU in `glibc`, added an extra (but unfinished and useless), field `tm_zone` to `struct tm` that can be hijacked to handled every timezone.)

One could then build a superset from what's already offered by `time.h`:
  - the object `struct tm` that represents an instant in time,
  - the environment variable `TZ`
  - the functions `tzset`, `mktime`, `time`, `gmtime_r`, `localtime_r`, `strptime` and `strftime`.

No extra logic is required to what is already known by the system (arithmetics, timezones, daylight saving time switches, leap years, leap seconds and display format). This ensures a consistent, robust and flawless design.

Therefore, I have been looking for a simple library of functions in C to manage date and time in a POSIX framework, and at a higher level than 
what POSIX offers, but could not find any that would meet my needs: simplicity and user-friendlyness.

That's the reason why I wrote this minimal set of functions, comparable to what DateTime could offer for .Net (btw, DateTimeOffset in .Net is 
useless as it does not handle daylight saving time.) or what ZonedDateTime offers in Java.

![Wallclocks](time-of-the-world.webp "Wallclocks")

This is what this API `tm` is meant for: let the user handle dates and times easily and naturally.

This API has been intensively tested but is not guaranted flawless.
Let me know if you find any bug.


# Synopsis

```c
#define _DEFAULT_SOURCE         // for additional field const char *tm_zone in struct tm (_BSD_SOURCE has been deprecated)
#define _XOPEN_SOURCE           // for strptime
#define _POSIX_C_SOURCE         // for tzset, gmtime_r, localtime_r, setenv, unsetenv

#include <time.h>
#include "dates.h"
```

# Files

  - Interface is described in `dates.h`.
  - Implementation is in `dates.c`.

# Concepts

Instants in time, as well as positions in space, are absolute and mark a specific moment in the evolution pf the universe: the moment man 
walked on the moon for the first time, or the moment the bakery down the street opened this morning.

Nevertheless, to be figured out with numbers (such as date and time), these instants must be represented in referentials.
Those referentials allow comparisons and arithmetic operations on instants.

A referential is needed to define and represent an instant (by its components such as day of month or hour in the day),
and to set the rules of the Gregorian calendar for arithmetic operations on dates (additions, substractions and comparisons).

Those rules are the same for every referential except for the daylight saving time changes that depend on each country, and defined from year to year.

The two most usual referentials are the Universal Time Coordinates or the local time coordinates.

## Internal design

This library is a superset of lower level C and POSIX concepts and functions of `time.h`.
The functions of this toolbox manipulate instants (points) in time expressed as a date and time of day, in Gregorian calendar.

Instants are internally stored in the structure `struct tm` defined by POSIX, with a resolution of one second.
However, objects `struct tm` should be considered as abstract data types, and should not be initialized by hand.
Anyway, this allows compatible access to low level POSIX functions, such as `strftime` or `strptime` (see man page of `mktime` for usage), even though it's not recommended.

This API does not try to re-implement the logic on dates and time: all arithmetics are left to the POSIX library (i.e. `mktime`, which knows all the necessary rules.)

## Multi-threading

The API makes an extensive use of the environment variable `TZ` because `TZ` serves as the unique input parameter for `tzset` (this looks like a design flaw)
and must be used to define and switch time representations in different timezones.

Nevertheless, `TZ` is an environment variable shared between all threads : is is not thread-safe by essence.
To work around this and ensure multi-thread safety, the API uses a non-recursive (read-write) lock associated with `TZ`, which guards all calls to `setenv` and `getenv ` on `TZ`.

Therefore, multi-thread safety is provided as long as the environment variable `TZ` is not set by hand in any thread (this is referred to as "MT Safe env" in POSIX safety concepts, see man page `attributes(7)`):

  - Except for `tm_setlocalwallclock` (see below), all other functions are multi-thread safe and can be used in several threads without interfering.
  - the global variable `tm_is_TZ_owner` can optionnally  be set to 1 to make use of `TZ` constness and to increase performance.

In case the condition on `TZ` cannot be ensured, letting or resetting `tm_is_TZ_owner` to its default value (0) increases multi-thread safetyness as much as possible.

Setting `TZ` by hand is neither necessary nor recommended though, as it is a very low level feature of `time.h` which is difficult to use (see man page `tzset(3)`).
If the local time zone should be defined globally (for all threads), `tm_setlocalwallclock` should be used.

# Toolbox overview

The toolbox is made of several sets of functions:

   - Initializers:
      - from scalar values (`tm_set`)
      - from strings (`tm_settimefromstring`, `tm_setdatefromstring` and `tm_setfromiso8601`).
   - Date and time properties:
      - `tm_getyear`, `tm_getmonth`, `tm_getday`, `tm_gethour`, `tm_getminute`, `tm_getsecond`
      - `tm_getdayofyear`, `tm_getdayofweek`
      - `tm_gethoursofday`, `tm_getminutesofday`, `tm_getsecondsofday`
      - `tm_getisoyear`, `tm_getisoweek`
      - `tm_getutcoffset`,
      - `tm_isdaylightsavingtimeineffect`, `tm_isinsidedaylightsavingtimeoverlap`
   - World clock handlers:
      - `tm_getwallclock`
      - `tm_changetoutc`, `tm_changetolocaltime`, `tm_changetosystemtime`, `tm_changetowallclock`
      - `tm_isdefinedinutc`, `tm_isdefinedinlocaltime`, `tm_isdefinedinsystemtime`, `tm_isdefinedinwallclock`
      - `tm_setlocalwallclock`, `tm_getlocalwallclock`, `tm_islocalwallclock`
   - Formatters:
      - `tm_datetostring`, `tm_timetostring`, `tm_tostring`, `tm_toiso8601`
   - Comparators:
       - `tm_compare`, `tm_equals`
   - Arihmetic operators:
      - `tm_addyears`, `tm_addmonths`, `tm_adddays`
      - `tm_addhours`, `tm_addminutes`, `tm_addseconds`
      - `tm_trimtime`
   - Duration operators:
      - `tm_diffyears`, `tm_diffmonths`, `tm_diffweeks`, `tm_diffdays`
      - `tm_diffhours`, `tm_diffminutes`, `tm_diffseconds`
   - Calendar intervals
      - `tm_diffcalendaryears`, `tm_diffisoyears`, `tm_diffcalendarmonths`, `tm_diffcalendardays`
   - Serializer/deserializer:
      - `tm_tobinary`, `tm_frombinary`
   - Calendar properties:
      - `tm_getdaysinyear`, `tm_isleapyear`, `tm_getweeksinisoyear`
      - `tm_getdaysinmonth`
      - `tm_gethoursinday`, `tm_getminutesinday`, `tm_getsecondsinday`
      - `tm_getfirstweekdayinmonth`, `tm_getlastweekdayinmonth`, `tm_getfirstweekdayinisoyear`

Functions return either a `tm_status` (which can be tested against `TM_OK` or `TM_ERROR`), or an integer value.
Values `TM_JANUARY` (1) to `TM_DECEMBER` (12) can be used to represent months, `TM_MONDAY` (1) to `TM_SUNDAY` (7) to represent days of week.

## Initializers

Instants of time should be initialized with `tm_set()`.
Once a `struct tm` variable `dt` has been allocated statically or dynamically, its address can be passed to `tm_set`.
The use of this function `tm_set` is compulsory, as well as easier than dealing directly with `struct tm`.

`tm_set` accepts two signatures:
 ```c
tm_status tm_set (struct tm *dt, [tm_predefined_instant instant = TM_NOW], [const char* wallclock = TM_REF_LOCALTIME]);
tm_status tm_set (struct tm *dt, int year, tm_month month, int day, int hour, int min, int sec, [const char* wallclock = TM_REF_LOCALTIME], [tm_wallclock clock = TM_ST_OVER_DST]);
```
Arguments between brackets are *optional*. The default value is used if they are ommitted.

Instants `dt` will be represented in local time, UTC or any other timezone, as defined by the parameter `wallclock`.
The parameter `wallclock` also indicates in which time referential the other argument parameters are defined and can be either:
  - `TM_REF_LOCALTIME` for the local time referential ;
  - `TM_REF_UTC` for the Coordinated Universal Time (a.k.a GMT or UTC) ;
  - `TM_REF_UNCHANGED` for the current referential of `dt` before the call (defined by a previius call to `tm_set` of `tm_changetowallclock`) ;
  - a wallclock (`"Europe/Paris"` for instance) conforming to a format accepted by `tz_set` (see man page).

`instant` can be either `TM_NOW` (default value) or `TM_TODAY`:
  - Use `TM_NOW` to initialize `dt` to the current date and time. The referential `ref` *has no effect* on the instant initialized. It only 
affects the way the instant will be displayed and the way operators will act on that instant.
  - Use `TM_TODAY` to initialize `dt` to the beginning of the current day, either local or UTC.

`year`, `month`, `day`, `hour`, `min`, `sec` respectively hold the year (4 digits), month (of type tm_month or integer starting from 1), day of month (starting from 1), 
hour of day (starting from 0), minutes in hour (starting from 0) and seconds in minute (starting from 0), expressed in the referential `ref` as explained above.

Daylight savinf time is handled automatically.
Therefore, `clock` is usually not required and is only helpful to tweak the behavior of `tm_set` *during the very hour* of transition from summer time (DST) to winter time (ST)
(when clocks usually shift backward by one hour) as a clock therefore reads times twice (before and after the shift),
leading to confusion (e.g in Paris, on Sun October the 27th, 2019 at 3am, clocks are shifted back to 2am, therefore e.g. 2.30am occurs twice).

In this specific case, if the *local* time specified by `year`, `month`, `day`, `hour`, `min`, `sec` is unclear
as it could refer either to winter time or to summer time), `clock` lets the user choose between those two possibilities
(if ever necessary):
  - `TM_ST_OVER_DST` gives preference to standard time (winter time) in case of ambiguity. This is the default value.
  - `TM_DST_OVER_ST` gives preference to daylight saving time (summer time) in case of ambiguity.

**In any case, whatever the value of `clock`, daylight saving time is handled automatically and does not depend on this parameter.**

`clock` is ignored if `ref` equals to `TM_REF_UTC`.

`tm_set` returns `TM_ERROR` if the date and time specified by `year`, `month`, `day`, `hour`, `min`, `sec` does not exist or is out of range.
It returns `TM_OK` otherwise.

Once an instant has been initialized, the referential used to represent it can be tested with `tm_isdefinedinutc`, `tm_isdefinedinsystemtime`, `tm_isdefinedinlocaltime`
or `tm_isdefinedinwallclock` (which takes an extra argument for the wallclock against which to apply the test).
```c
int tm_isdefinedinutc (struct tm *dt);
int tm_isdefinedinsystemtime (struct tm *dt);
int tm_isdefinedinlocaltime (struct tm *dt);
int tm_isdefinedinwallclock (struct tm *dt, const char* wallclock);
```
Those functions return 1 or 0. Note that thoses functions are not exclusive: several can retiurn 1 (if the local time is defined as the system time which is set to UTC for instance).

## Local time zone

```c
tm_status tm_setlocalwallclock (const char *wallclock)
```
The local time zone can be defined globally (for all threads) with `tm_setlocalwallclock`.
This function is therefore not thread safe (MT-Unsafe const:env as defined by POSIX, see attributes(7)))

As such, the local time referential corresponds to the timezone specified by the last call to `tm_setlocalwallclock` or by default to the timezone of the operating system.

The parameter `wallclock` can be either:
  - a timezone name (`"Europe/Paris"` for instance) conforming to the same formats of the environment variable `TZ` accepted by `tzset` (see man page).
  - `TM_REF_UTC` to switch to Coordinated Universal Time (a.k.a GMT or UTC) ;
  - `TM_REF_SYSTEMTIME` to switch back to the system timezone).

It returns TM_ERROR in case of an invalid timezone. TM_OK otherwise.
  
The local timezone can be retreived with:
```c
const char *tm_getlocalwallclock (void)
```
It returns:
  - a timezone (`"Europe/Paris"` for instance) conforming to a format accepted by `tzset` (see man page).
  - `TM_REF_UTC` for the Coordinated Universal Time (a.k.a GMT or UTC) ;
  - `TM_REF_SYSTEMTIME` for the system timezone) ;

The local timezone can be checked by a call to `tm_islocalwallclock` which returns 0 or 1.
```c
int tm_islocalwallclock (const char *wallclock)
```
The parameter of those functions can be either:
  - a timezone (`"Europe/Paris"` for instance) conforming to a format accepted by `tzset` (see man page).
  - `TM_REF_UTC` for the Coordinated Universal Time (a.k.a GMT or UTC) ;
  - `TM_REF_SYSTEMTIME` for the system timezone) ;

Note:

  - Using `tm_setlocalwallclock` is relevant for desktop applications for which the single user location and timezone is significant.
  - Using `tm_setlocalwallclock` is meaningless for client/server or web applications for which users can be seated all around the world.
    In that case, the user timezone (defined elsewhere manually or from her location for instance) should systematically be passed to calls to `tm_set`.

## Referential management

Once an instant has been initialized, it can be represented either in UTC, local time referential or any other timezone.

The user can switch alternately between referentials by a call to `tm_changetowallclock`.
```c
tm_status tm_changetowallclock (struct tm *date, const char* wallclock);
```
The parameter `wallclock` indicates which time referential to use and can be either:
  - `TM_REF_LOCALTIME` for local time referential ;
  - `TM_REF_UTC` for Coordinated Universal Time (a.k.a GMT or UTC) ;
  - a timezone (`"Europe/Paris"` for instance) conforming to a format accepted by `tz_set` (see man page).

This call *does NOT affect* the value of the instant but only the way it is represented using units of time (year, month, day, hour, minutes and seconds).
This call *does affect* arithmetic operations on days, as they depend on daylight saving time rules, which are defined diffrently in each timezone.

Short cuts can be used to switch to TM_REF_UTC, TM_REF_SYSTEMTIME or TM_REF_LOCALTIME.
```c
tm_status tm_changetoutc (struct tm *date);
tm_status tm_changetosystemtime (struct tm *date);
tm_status tm_changetolocaltime (struct tm *date);
```
The current wallclock used to represent a date and time can be checked with (self explanatory) function:
```c
const char *tm_getwallclock (struct tm date);
int tm_isdefinedinwallclock (struct tm, const char *);
int tm_isdefinedinutc (struct tm);
int tm_isdefinedinsystemtime (struct tm);
int tm_isdefinedinlocaltime (struct tm);
```
## Date and time properties

Date and time properties can be accessed through several functions.
They take one argument of type `struct tm`. 
```c
int tm_getyear (struct tm date);
tm_month tm_getmonth (struct tm date);
int tm_getday (struct tm date);
int tm_gethour (struct tm date);
int tm_getminute (struct tm date);
int tm_getsecond (struct tm date);

int tm_getdayofyear (struct tm date);
tm_dayofweek tm_getdayofweek (struct tm date);
int tm_getisoweek (struct tm date);
int tm_getisoyear (struct tm date);
int tm_getsecondsofday (struct tm date);
int tm_getminutesofday (struct tm date);
int tm_gethoursofday (struct tm date);

int tm_getutcoffset (struct tm date);   // In seconds
int tm_isdaylightsavingtimeineffect (struct tm date);
int tm_isinsidedaylightsavingtimeoverlap (struct tm date);
```
Returned values are:

   - `tm_getyear`: returns an integer holding the year (4 digits)
   - `tm_getmonth`: returns a named integer `tm_month` holding the month (from 1 to 12)
   - `tm_getday`: returns an integer holding the day of month (from 1 to 31)
   - `tm_gethour`: returns an integer holding the hour of day (from 0 to 23)
   - `tm_getminute`: returns an integer holding the minute (from 0 to 59)
   - `tm_getsecond`: returns an integer holding the second (from 0 to 59)
   - `tm_getdayofyear`: returns an integer holding the day in year (from 1 to 366)
   - `tm_getdayofweek`: returns a named integer `tm_dayofweek` holding the day in week (from 1 as monday to 7 as sunday)
   - `tm_gethoursofday`: returns the number of elapsed hours since the beginning of the day (from 0 to 24)
   - `tm_getminutesofday`: returns the number of elapsed minutes since the beginning of the day (from 0 to 1499)
   - `tm_getsecondsofday`: returns the number of elapsed hours since the beginning of the day (from 0 to 89999)
   - `tm_getisoyear`: returns an integer holding the ISO year (4 digits)
   - `tm_getisoweek`: returns an integer holding the ISO week number (from 1 to 53, the first week of the year is the one containing the first thursday ot the year)
   - `tm_getutcoffset`: offset in seconds related to UTC
   - `tm_isdaylightsavingtimeineffect`: indicates either DST is in effect (1) or not (0)
   - `tm_isinsidedaylightsavingtimeoverlap` : indeicates if the time representation is ambiguous and should be disambiguated on display (see below). 

## String converters

### Output to displayers

Dates can be converted into strings (for display purpose).

Dedicated functions format the date and time `dt` according to regional settings and places the result in the character array s of size max:
```c
tm_status tm_dateintostring (struct tm dt, size_t max, char *str);
tm_status tm_timeintostring (struct tm dt, size_t max, char *str);
tm_status tm_tostring (struct tm dt, size_t max, char *str);
tm_status tm_toiso8601 (struct tm dt, size_t max, char *str, [int separator = 0]);
```
`max` is the size of `str`, including the terminating null byte.
`separator`, optional, is either 0 (default value) or 1 whether separators are expected within date and time (*YYYYMMDD*T*hhmmss* or *YYYY-MM-DD*T*hh:mm:ss* respectively).

`TM_ERROR` is returned if the size of `str` is to small, `TM_OK` otherwise.

N.B.: `strftime` can still be used with any date and time for specific needs.

In case a date and time could be interpreted either as winter time or as summer time (daylight saving time in effect), the function `tm_isinsidedaylightsavingtimeoverlap` will return 1. In this case, a call to `tm_isdaylightsavingtimeineffect` will let the user make explicit which date and time is displayed.
For instance:

```c
int dc = tm_isinsidedaylightsavingtimeoverlap (dt);
int dst = tm_isdaylightsavingtimeineffect (dt);
str[strftime (str, sizeof (str) / sizeof (*str), "%X%%", dt) - 1] = dc ? (dst ? "A" : "B") : " ";
```

### Input from displayers

`tm_setdatefromstring()`, `tm_settimefromstring` let initialize a date or time (resp.) from a string formatted with regard to the regional setting or the ISO8601 standard.

Signatures are:
```c
tm_status tm_setdatefromstring (struct tm *dt, const char *text, const char *wallclock, [tm_wallclocksetting = TM_ST_OVER_DST]);
tm_status tm_settimefromstring (struct tm *dt, const char *text, const char *wallclock, [tm_wallclocksetting = TM_ST_OVER_DST]);
```
`tm_wallclocksetting` is usually not required but, if ever necessary, is used as for `tm_set`.
```c
tm_status tm_setfromiso8601 (struct tm *dt, const char *str);
```
`tm_setfromiso8601` lets initialize a date and optionnaly time from a string formatted with regard to the ISO8601 standard.
The resullting value of `dt` will be represented:
  - in local time if the UTC offset specified in `str` equals the local time UTC offset,
  - in UTC otherwise. 

## Arithmetics

## Assignment

An instant `dta` (of type `struct tm`) can be copied into another one with the assignment operator `=`:

```c
struct tm dtb = dta;
```

### Adding a duration to an instant
```c
tm_status tm_addseconds (struct tm *date, long int nbSeconds);
tm_status tm_addminutes (struct tm *date, long int nbMinute);
tm_status tm_addhours (struct tm *date, long int nbHours);
tm_status tm_adddays (struct tm *date, int nbDays);
tm_status tm_addmonths (struct tm *date, int nbMonths);
tm_status tm_addyears (struct tm *date, int nbYears);
```
Those function take a pointer to an instant `struct tm` as first argument, and an integer (either positive or negative) as second argument.

The behavior of these operations depend on the referential (local, UTC or any wallclock) in which the instant is represented, as the duration of days depend on the daylight saving time rules dictated by each country (daylight saving time is taken into account).

   - `tm_addyears`: adding years
   - `tm_addmonths`: adding months
   - `tm_adddays`: adding days
   - `tm_addhours`: adding hours
   - `tm_addminutes`: adding minutes
   - `tm_addseconds`: adding seconds

All these functions take a `struct tm *` as the first argument, and an interger (which might be negative) as the second argument.
They return TM_ERROR in case of overflow (date out of range), TM_OK otherwise.

### Substracting two instants
```c
long int tm_diffseconds (struct tm start, struct tm stop);
long int tm_diffminutes (struct tm start, struct tm stop);
long int tm_diffhours (struct tm start, struct tm stop);
int tm_diffdays (struct tm start, struct tm stop, [int *seconds]);
int tm_diffweeks (struct tm start, struct tm stop, [int *days, int *seconds]);
int tm_diffmonths (struct tm start, struct tm stop, [int *days, int *seconds]);
int tm_diffyears (struct tm start, struct tm stop, [int *months, int *days, int *seconds]);
```
Those function take two pointers to an instant `struct tm` as firt arguments, and return an integer (either positive or negative). The last following arguments are optional.

The behavior of these operations depend on the referential (local, UTC or any wallclock) in which the instant is represented, as the duration of days depend on the daylight saving time rules dictated by each country (daylight saving time is taken into account).

   - `tm_diffyears`: number of elapsed full years between two instants, and optionally, remaining months, days and seconds

```c
int tm_diffyears (struct tm start, struct tm stop, [int *months = 0, int *days = 0, int *seconds = 0]);
```
   - `tm_diffmonths`: number of elapsed full months between two instants, and optionally, remaining days and seconds
```c
int tm_diffmonths (struct tm start, struct tm stop, [int *days = 0, int *seconds = 0]);
```
   - `tm_diffweeks`: number of elapsed full weeks between two instants, and optionally, remaining days and seconds
```c
int tm_diffweeks (struct tm start, struct tm stop, [int *days = 0, int *seconds = 0]);
```
   - `tm_diffdays`: number of elapsed full days between two instants, and optionally, remaining seconds
```c
int tm_diffdays (struct tm start, struct tm stop, [int *seconds = 0]);
```
   - `tm_diffhours`: number of elapsed full hours between two instants
```c
long int tm_diffhours (struct tm start, struct tm stop);
```
   - `tm_diffminutes`: number of elapsed full minutes between two instants
```c
long int tm_diffminutes (struct tm start, struct tm stop);
```
   - `tm_diffseconds`: number of elapsed seconds between two instants
```c
long int tm_diffseconds (struct tm start, struct tm stop);
```
### Comparing two instants
```c
int tm_compare (const void *dta, const void *dtb)
```
`tm_compare` compares two instants, whatever the referential with which they have been defined:

  - it returns -1 if the instant dta is before dtb,
  - it returns 1 if the instant dta is after dtb,
  - it returns 0 if the instants dta and dta are simultaneous.
  
This function is compatible with a use by `qsort`
```c
int tm_equals (struct tm a, struct tm b)
```
`tm_equals`compares two dates together whith their referential. It should scarcely be used.

### Rounding an instant

A date and time can be rounded to mignight (in the previously chosen referenetial) with `tm_trimtime`.
```c
tm_status tm_trimtime (struct tm *date)
```
### Comparing two calendar dates

The following functions take two dates as arguments and compare their calendar properties.

```c
int tm_diffcalendardays (struct tm start, struct tm stop);
int tm_diffcalendarmonths (struct tm start, struct tm stop);
int tm_diffcalendaryears (struct tm start, struct tm stop);
int tm_diffisoyears (struct tm start, struct tm stop);
```

Returned values:

  - `tm_diffcalendaryears`: number of changes of years between the two dates
  - `tm_diffisoyears`: number of changes of iso years between the two dates
  - `tm_diffcalendarmonths`: number of changes of months between the two dates
  - `tm_diffcalendardays`: number of changes of days between the two dates

All theses functions take two arguments of type `struct tm` and return an integer (which might be negative).
The two dates should use the same referential, otherwise, 0 is returned and `errno` is set.

The behavior of these operations depends on the referential (local or UTC) in which the instants are represented.

## Gregorian calendar properties
```c
int tm_getdaysinyear (int year);
int tm_isleapyear (int year);
int tm_getweeksinisoyear (int isoyear);
int tm_getdaysinmonth (int year, tm_month month);
int tm_getsecondsinday (int year, tm_month month, int day, [const char *wallclock = TM_REF_LOCALTIME]);
int tm_getminutesinday (int year, tm_month month, int day, [const char *wallclock = TM_REF_LOCALTIME]);
int tm_gethoursinday (int year, tm_month month, int day, [const char *wallclock = TM_REF_LOCALTIME]);
int tm_getfirstweekdayinmonth (int year, tm_month month, tm_dayofweek dow);
int tm_getlastweekdayinmonth (int year, tm_month month, tm_dayofweek dow);
int tm_getfirstweekdayinisoyear (int isoyear, tm_dayofweek dow);
```
Those functions retreive properties on the gregorian calendar.

  - On year:

      - `tm_getdaysinyear`: number of days in year
      - `tm_isleapyear`: returns 1 for leap year, 0 otherwise
      - `tm_getweeksinisoyear`: number of weeks in iso-year

  - On month:

      - `tm_getdaysinmonth`: number of days in month

  - On day:

      - `tm_gethoursinday`: number of hours in day (23, 24 or 25)
      - `tm_getminutesinday`: number of minutes in day
      - `tm_getsecondsinday`: number of seconds in day

  - On weekday:

      - `tm_getfirstweekdayinmonth`: cardinal day for the first given weekday in month
      - `tm_getlastweekdayinmonth`: cardinal day for the last given weekday in month
      - `tm_getfirstweekdayinisoyear`: cardinal day for the first given weekday in iso-year

## Serializer/deserializer
```c
time_t tm_tobinary (struct tm);
tm_status tm_frombinary (struct tm *dt, time_t binary, , [const char *wallclock = TM_REF_LOCALTIME]);
```
Instants can be persisted in databases using those two functions for storing (`tm_tobinary`) and retrieving (`tm_frombinary`) in a format that is independent of timezone and therefore suitable for internationalization.

  - `tm_tobinary`: transforms into a value for database storage or data transfer (the returned value is the number of seconds elapsed since the Epoch, 1970-01-01 00:00:00, UTC)
  - `tm_frombinary`: retrieves from a value stored in database storage received from data transfer. The retrieved value can then be represented in a chosen wallclock referential.

## Dates only

Additional functions are available to manage calendar dates (without time of day).
Those calendar dates are absolute dates (New year's Eve 2018, 4th of July 2011), without any reference to timezone.

Note: if a period, one day long, localized in a given country and timezone, is needed, `tm_set` and `tm_trimtime` should be used to set the beginning of that whole day ;
the end (excluded) of that day can then be fetched using `tm_adddays (&dt, 1)`.

```c
tm_status dt_set (struct tm *dt, [const char *wallclock = TM_REF_LOCALTIME]);   // Initializes dt with the current date for the given timezone.
tm_status dt_set (struct tm *dt, int year, tm_month month, int day);

tm_status dt_tostring (struct tm dt, size_t max, char *str);
tm_status dt_toiso8601 (struct tm dt, size_t max, char *str, int sep);          // Separator '-' is used if sep is set
tm_status dt_setfromstring (struct tm *dt, const char *text);
tm_status dt_setfromiso8601 (struct tm *dt, char *str);

int dt_getyear (struct tm date); // On 4 digits
tm_month dt_getmonth (struct tm date);
int dt_getday (struct tm date);
int dt_getdayofyear (struct tm date);  // Starting from 1
tm_dayofweek dt_getdayofweek (struct tm date);
int dt_getisoweek (struct tm date);
int dt_getisoyear (struct tm date);

tm_status dt_adddays (struct tm *date, int nbDays);
tm_status dt_addmonths (struct tm *date, int nbMonths);
tm_status dt_addyears (struct tm *date, int nbYears);

int dt_equals (struct tm a, struct tm b);
int dt_compare (const void *dta, const void *dtb);

int dt_diffdays (struct tm start, struct tm stop);
int dt_diffweeks (struct tm start, struct tm stop, [int *days]);
int dt_diffmonths (struct tm start, struct tm stop, [int *days]);
int dt_diffyears (struct tm start, struct tm stop, [int *months, int *days]);

int dt_diffcalendardays (struct tm start, struct tm stop);
int dt_diffcalendarmonths (struct tm start, struct tm stop);
int dt_diffcalendaryears (struct tm start, struct tm stop);
int dt_diffisoyears (struct tm start, struct tm stop);

int dt_tobinary (struct tm date);
tm_status dt_frombinary(struct tm *date, int binary);
```

# Unit testing

The API has been extensively tested with unit tests implemented in `dates_tu_check.c`.

Check is used as the Unit Testing Framework for C (see [https://libcheck.github.io/check/](https://libcheck.github.io/check/)).

[gcov](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html) is used to ensure full coverage (97 %) of the code by the unit tests.

# Examples

Look at `dates_tu_check.c` for examples of usage.

Use `Makefile` as an example for compilation.

# Afterword

Handling dates and times is easy as long as the user let the system work it out, and does not try to code it by his own.

Some principles have to be followed:

  - a date and time does not hold any format (not more than numeric values). A format is only needed for user input and display.
    It should never be used to store or transport dates and times, except international standards such as ISO 8601.
  - a date and time does not hold any timezone (no more than a numeric value holds a unit). A timezone is only needed for arithmetic, user input and display.
    A timezone should not be used when storing or transporting dates and times: dates and times should only be stored or exchanged between systems in UTC.

Have fun !
