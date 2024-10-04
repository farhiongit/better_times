
/** @file dates.c
 * All functions are Multi Thread safe (MT-Safe), unless specified.
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
//#define _GNU_SOURCE
//#define _POSIX_SOURCE
#define _DEFAULT_SOURCE         // for additional field const char *tm_zone in struct tm (_BSD_SOURCE has been deprecated)
#define _XOPEN_SOURCE           // for strptime
#define _POSIX_C_SOURCE         // for tzset, gmtime_r, localtime_r, setenv, unsetenv

#include <time.h>               // Comment to catch syscalls to time.h
//#include <bits/types/struct_tm.h>       // Comment out to catch syscalls to time.h
#include "dates.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

int tm_is_TZ_owner = 0;         // Not optimized by default

static const char tm_ref_localtime = 0;
const char *const TM_REF_LOCALTIME = &tm_ref_localtime; // Local time representation
const char *const TM_REF_SYSTEMTIME = 0;        // Local time representation as defined by the system
static const char tm_ref_utc = 0;
const char *const TM_REF_UTC = &tm_ref_utc;     // Coordinated Universal Time representation
static const char tm_ref_unchanged = 0;
const char *const TM_REF_UNCHANGED = &tm_ref_unchanged;
static const char tm_ref_undefined = 0;
const char *const TM_REF_UNDEFINED = &tm_ref_undefined;

#ifndef WALLCLOCK_MAX_NB
#  define WALLCLOCK_MAX_NB 2000 // there are currently 1829 entries in TZ database /usrshare/zoneinfo
#endif
#define WALLCLOCK_MAX_LENGTH 200
static size_t nb_registered_wallclocks = 0;
static char registered_wallclock[WALLCLOCK_MAX_LENGTH][WALLCLOCK_MAX_NB] = { 0 };       // Set everything to 0 (even though already the default behavior for static variables)

static const char *TM_LOCALTIMEZONE_NAME = 0;   // 0 means system timezone
// Make the API Thread safe (MT-Safe env locale to be precise, see see man attributes(7)):
static pthread_mutex_t tzset_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t wallclock_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t localtimename_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *
tm_utctimezone (void)
{
  return "UTC";
}

static int
tm_isutctimezone (const char *tm_zone)
{
  return (tm_zone == TM_REF_UTC || tm_zone == tm_utctimezone () || (tm_zone && tm_utctimezone () && !strcmp (tm_zone, tm_utctimezone ())))? 1 : 0;
}

static const char *
tm_systemtimezone (void)
{
  return TM_REF_SYSTEMTIME;
}

static const char *
tm_localtimezone (void)
{
  const char *ret = 0;
  pthread_mutex_lock (&localtimename_mutex);
  ret = TM_LOCALTIMEZONE_NAME;
  pthread_mutex_unlock (&localtimename_mutex);
  if (ret)
    return ret;
  else
    return tm_systemtimezone ();
}

static void
tm_unregisterwallclock (const char *wc)
{
  pthread_mutex_lock (&wallclock_mutex);
  if (wc && *wc)
    for (size_t i = 0; i < nb_registered_wallclocks; i++)
      if (!strcmp (registered_wallclock[i], wc))
      {
        *registered_wallclock[i] = 0;
        break;
      }
  pthread_mutex_unlock (&wallclock_mutex);
}

static const char *
tm_getregisteredwallclock (const char *wc, int add)
{
  if (wc == TM_REF_SYSTEMTIME)
    return tm_systemtimezone ();
  else if (tm_isutctimezone (wc))
    return tm_utctimezone ();
  else if (wc == TM_REF_LOCALTIME)
    return tm_localtimezone ();
  else if (wc == 0 || *wc == 0)
    return (errno = EINVAL), TM_REF_UNDEFINED;

  pthread_mutex_lock (&wallclock_mutex);
  size_t islot = nb_registered_wallclocks;
  const char *ret = 0;
  for (size_t i = 0; i < nb_registered_wallclocks; i++)
    if (!strcmp (registered_wallclock[i], wc))
    {
      ret = registered_wallclock[i];
      pthread_mutex_unlock (&wallclock_mutex);
      return ret;
    }
    else if (islot == nb_registered_wallclocks && *registered_wallclock[i] == 0)
      islot = i;

  if (!add)
  {
    pthread_mutex_unlock (&wallclock_mutex);
    return TM_REF_UNDEFINED;
  }

  if (islot < nb_registered_wallclocks)
  {
    ret = strncpy (registered_wallclock[islot], wc, WALLCLOCK_MAX_LENGTH - 1);
    pthread_mutex_unlock (&wallclock_mutex);
    return ret;
  }

  if (nb_registered_wallclocks == WALLCLOCK_MAX_NB || strlen (wc) >= WALLCLOCK_MAX_LENGTH)
  {
    errno = ENOMEM;
    perror ("Wallclock registration");
    pthread_mutex_unlock (&wallclock_mutex);
    return tm_systemtimezone ();
  }

  ret = strncpy (registered_wallclock[nb_registered_wallclocks++], wc, WALLCLOCK_MAX_LENGTH - 1);
  pthread_mutex_unlock (&wallclock_mutex);
  return ret;
}

int
tm_isdefinedinwallclock (struct tm dt, const char *wc)
{
  wc = tm_getregisteredwallclock (wc, 0);
  if (wc == TM_REF_UNDEFINED)
    return 0;
  else if (dt.tm_zone == wc)
    return 1;
  else if (dt.tm_zone && wc)
    return !strcmp (dt.tm_zone, wc);
  else
    return 0;
}

static void
tm_tzunset (const char *old_tz)
{
  if (tm_is_TZ_owner)
    return;
  if (!old_tz)
    unsetenv ("TZ");
  else
    setenv ("TZ", old_tz, 1);
  tzset ();
}

static tm_status
tm_tzset (const char *wc, const char * *p_old_tz)
{
  char *old_tz = getenv ("TZ");
  if (p_old_tz)
    *p_old_tz = old_tz;
  if (wc && !*wc)
    return TM_ERROR;
  // if the TZ variable does appear in the environment, but its value is empty,
  // then Coordinated Universal Time (UTC) is used as default.
  if (wc && !strcmp (tm_utctimezone (), wc))    // Specific code for UTC
    wc = "";
  if (tm_is_TZ_owner && !old_tz && !wc)
    return TM_OK;
  else if (!wc)
    unsetenv ("TZ");
  else if (tm_is_TZ_owner && old_tz && !strcmp (old_tz, wc))
    return TM_OK;
  else if (setenv ("TZ", wc, 1) != 0)
    return TM_ERROR;

  tm_status ret = TM_OK;
  int saveerrno = errno;
  errno = 0;
  // N.B.: tzset is automatically called by the other time conversion functions that depend on the timezone.
  tzset ();                     // time syscall, seems to set errno if TZ is empty or cannot be interpreted.
  // if the TZ variable does appear in the environment, but its value cannot be interpreted,
  // then Coordinated Universal Time (UTC) is used as default.
  if (errno && wc && strlen (wc))       // wc = "" is not an error
  {
    tm_unregisterwallclock (wc);
    if (!old_tz)
      unsetenv ("TZ");
    else
      setenv ("TZ", old_tz, 1);
    tzset ();
    ret = TM_ERROR;
  }
  else
    errno = saveerrno;

  return ret;
}

// wc should conform to format accepted by tzset (see man tzset)
// On Linux, possible values can be listed with command 'find /usr/share/zoneinfo \! -type d | sort'
// Thread safety : MT-Unsafe const:env (see attributes(7))
tm_status
tm_setlocalwallclock (const char *wc)
{
  if (wc == TM_REF_LOCALTIME || wc == TM_REF_UNCHANGED)
    return TM_OK;
  pthread_mutex_lock (&tzset_mutex);
  const char *old_tz;
  if (tm_tzset (wc = tm_getregisteredwallclock (wc, 1), &old_tz) == TM_ERROR)   // tm_tzset simply checks if wc is a valid timezone here.
  {
    pthread_mutex_unlock (&tzset_mutex);
    return TM_ERROR;
  }
  pthread_mutex_lock (&localtimename_mutex);
  TM_LOCALTIMEZONE_NAME = wc;
  pthread_mutex_unlock (&localtimename_mutex);
  tm_tzunset (old_tz);
  pthread_mutex_unlock (&tzset_mutex);
  return TM_OK;
}

// Thread safety : MT-Safe env (see attributes(7))
const char *
tm_getlocalwallclock (void)
{
  const char *wc = 0;
  pthread_mutex_lock (&localtimename_mutex);
  wc = TM_LOCALTIMEZONE_NAME;
  pthread_mutex_unlock (&localtimename_mutex);
  if (tm_isutctimezone (wc))
    return TM_REF_UTC;
  else
    return tm_localtimezone ();
}

// Thread safety : MT-Safe env (see attributes(7))
int
tm_islocalwallclock (const char *wc)
{
  return tm_getregisteredwallclock (tm_localtimezone (), 0) == tm_getregisteredwallclock (wc, 0);
}

/// Normalizes instant in time.
/// @param [in,out] date Pointer to broken-down time structure
/// @returns Absolute calendar time
/// @remark Calls mktime. The tm_normalizetolocal() function is equivalent to the POSIX standard function mktime()
static tm_status                /* might set errno to EINVAL */
tm_normalize (struct tm *tm, time_t *t)
{
  /* (mktime man page)
     The mktime() function converts a broken-down time structure, expressed as local time, to calendar time representation.

     The function ignores the values supplied by the caller in the tm_wday, tm_yday and tm_gmtoff fields.
     The value specified in the tm_isdst field informs mktime() whether or not daylight saving time (DST) is in effect for the time supplied in
     the tm structure:
     - a positive value means DST is in effect;
     - zero means that DST is not in effect;
     - and a negative value means that mktime() should (use timezone information and system databases to) attempt to determine whether DST is in
     effect at the specified time.

     The mktime() function modifies the fields of the tm structure as follows:
     - tm_wday and tm_yday are set to values determined from the contents of the other fields;
     - tm_gmtoff is set according to the current timezone of the system (environment variable TZ).
     - If structure members are outside their valid interval, they will be normalized (so that, for example, 40 October is changed into 9 November).
     - tm_isdst  is set (regardless of its initial value) to a positive value or to 0, respectively, to indicate whether DST is or is not in effect
     at the specified time.

     Calling mktime() also sets the external variable tzname with information about the current timezone.
   */

  struct tm oldtm = *tm;
  const char *wc = tm->tm_zone;
  pthread_mutex_lock (&tzset_mutex);
  const char *old_tz;
  if (tm_tzset (wc, &old_tz) == TM_ERROR)
  {
    pthread_mutex_unlock (&tzset_mutex);
    return TM_ERROR;
  }

  int saveerrno = errno;
  errno = 0;
  // May apply daylight saving if tm_isdst is not negative before function call:
  time_t ret = mktime (tm);     // time syscall, calls tzset ; if structure members are outside their valid interval, they will be normalized.
  if (errno)
  {
    tm_tzunset (old_tz);
    pthread_mutex_unlock (&tzset_mutex);
    *tm = oldtm;
    return TM_ERROR;
  }

  tm_tzunset (old_tz);
  pthread_mutex_unlock (&tzset_mutex);
  tm->tm_zone = wc;
  if (tm->tm_year + 1900 + 1 < tm->tm_year)
    return (*tm = oldtm), (errno = EINVAL), TM_ERROR;
  if (t)
    *t = ret;
  errno = saveerrno;
  return TM_OK;
}

const char *
tm_getwallclock (struct tm date)
{
  return date.tm_zone;
}

static int
tm_isdaylightsavingextrasummertime (struct tm date)
{
  if (tm_isdefinedinutc (date) || !date.tm_isdst)
    return 0;

  date.tm_isdst = 0;
  tm_normalize (&date, 0);      // date was already normalized at creation, it should not set errno

  // Returns 1 during the hour before DST loses effect (from summer to winter time switch), 0 otherwise.
  return (date.tm_isdst ? 0 : 1);
}

static int
tm_isdaylightsavingextrawintertime (struct tm date)
{
  if (tm_isdefinedinutc (date) || date.tm_isdst)
    return 0;

  date.tm_isdst = 1;
  tm_normalize (&date, 0);      // date was already normalized at creation, it should not set errno

  // Returns 1 during the hour after DST loses effect (from summer to winter time switch), 0 otherwise.
  return (date.tm_isdst ? 1 : 0);
}

static tm_status
tm_todaylightsavingextrawintertime (struct tm *date)
{
  int saveerrno = errno;
  errno = 0;
  if (!tm_isdaylightsavingextrasummertime (*date) || errno)
    return (errno = saveerrno), TM_ERROR;

  date->tm_isdst = 0;
  return tm_normalize (date, 0) == TM_ERROR ? TM_ERROR : date->tm_isdst ? TM_ERROR : TM_OK;
}

static tm_status
tm_todaylightsavingextrasummertime (struct tm *date)
{
  int saveerrno = errno;
  errno = 0;
  if (!tm_isdaylightsavingextrawintertime (*date) || errno)
    return (errno = saveerrno), TM_ERROR;

  date->tm_isdst = 1;
  return tm_normalize (date, 0) == TM_ERROR ? TM_ERROR : date->tm_isdst ? TM_OK : TM_ERROR;
}

/*****************************************************
*   CONSTRUCTORS                                     *
*****************************************************/
tm_status
tm_make_ir (struct tm *tm, tm_predefined_instant instant, const char *rep)
{
  time_t now = time (0);        // time syscall

  if (rep == TM_REF_UNCHANGED)
    rep = tm_getwallclock (*tm);

  if (tm_isutctimezone (rep))
  {
    rep = tm_getregisteredwallclock (rep, 0);
    gmtime_r (&now, tm);        // time syscall
  }
  else
  {
    rep = tm_getregisteredwallclock (rep, 1);
    pthread_mutex_lock (&tzset_mutex);
    const char *old_tz;
    if (tm_tzset (rep, &old_tz) == TM_ERROR)
    {
      pthread_mutex_unlock (&tzset_mutex);
      return (errno = EINVAL), TM_ERROR;
    }
    localtime_r (&now, tm);     // time syscall. For portable code, tzset(3) should be called before localtime_r().
    tm_tzunset (old_tz);
    pthread_mutex_unlock (&tzset_mutex);
  }
  tm->tm_zone = rep;

  if (instant == TM_TODAY)
  {
    tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
    tm->tm_isdst = -1;          // Let timezone information and system databases define DST flag.
    return tm_normalize (tm, 0);
  }
  else if (instant == TM_NOW)
    return TM_OK;
  else
    return (errno = EINVAL), TM_ERROR;
}

tm_status
tm_make_dtrc (struct tm *tm, int year, tm_month month, int day, int hour, int min, int sec, const char *rep, tm_time_precedence clock)
{
  if (rep == TM_REF_UNCHANGED)
    rep = tm_getwallclock (*tm);
  rep = tm_getregisteredwallclock (rep, 1);

  tm->tm_year = year - 1900;
  if (tm->tm_year > year)
    return (errno = EINVAL), TM_ERROR;
  tm->tm_mon = (typeof (tm->tm_mon)) (month - 1);
  tm->tm_mday = day;
  tm->tm_hour = hour;
  tm->tm_min = min;
  tm->tm_sec = sec;
  tm->tm_zone = rep;
  tm->tm_isdst = -1;            // Let timezone information and system databases define DST flag.

  time_t ret;
  if (tm_normalize (tm, &ret) == TM_ERROR)
    return (errno = EINVAL), TM_ERROR;
  if (!tm_isutctimezone (rep))
  {
    if (clock == TM_DST_OVER_ST)
      tm_todaylightsavingextrasummertime (tm);
    else if (clock == TM_ST_OVER_DST)
      tm_todaylightsavingextrawintertime (tm);
    else
      return (errno = EINVAL), TM_ERROR;
  }

  if (tm->tm_year == year - 1900 && tm->tm_mon == (typeof (tm->tm_mon)) (month - 1) && tm->tm_mday == day && tm->tm_hour == hour && tm->tm_min == min && tm->tm_sec == sec)
    return TM_OK;
  else
    return (errno = EINVAL), TM_ERROR;
}

// tm should have been initialized with tm_set first.
tm_status
tm_setdatefromstring (struct tm *tm, const char *buf, const char *rep, tm_time_precedence clock)
{
  char *ret;

  ret = strptime (buf, "%x", tm);       // time syscall
  if (!ret || *ret)
    ret = strptime (buf, "%Ex", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%Y-%m-%d", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%Y−%m−%d", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%Y %m %d", tm);
  if (!ret || *ret)
    return (errno = EINVAL), TM_ERROR;

  int year = tm->tm_year + 1900;        /* tm_year is the number of years since 1900. */

  if (year >= 0 && year < 100)
  {
    time_t now = time (0);      // time syscall
    struct tm tm_now;
    tzset ();
    localtime_r (&now, &tm_now);        // time syscall
    int current_year = tm_now.tm_year + 1900;   /* tm_year is the number of years since 1900. */

    tm->tm_year += (typeof (tm->tm_year)) lround ((current_year - year) / 100.) * 100;
  }
  if (rep == TM_REF_UNCHANGED)
    rep = tm_getwallclock (*tm);
  rep = tm_getregisteredwallclock (rep, 1);
  tm->tm_zone = rep;
  tm->tm_isdst = -1;            // Let timezone information and system databases define DST flag.

  struct tm copy = *tm;

  time_t retval;
  if (tm_normalize (tm, &retval) == TM_ERROR)
    return (errno = EINVAL), TM_ERROR;
  if (!tm_isutctimezone (rep))
  {
    if (clock == TM_DST_OVER_ST)
      tm_todaylightsavingextrasummertime (tm);
    else if (clock == TM_ST_OVER_DST)
      tm_todaylightsavingextrawintertime (tm);
    else
      return (errno = EINVAL), TM_ERROR;
  }

  if (tm->tm_year == copy.tm_year && tm->tm_mon == copy.tm_mon && tm->tm_mday == copy.tm_mday
      && tm->tm_hour == copy.tm_hour && tm->tm_min == copy.tm_min && tm->tm_sec == copy.tm_sec)
    return TM_OK;
  else
    return (errno = EINVAL), TM_ERROR;
}

// tm should have been initialized with tm_set first.
tm_status
tm_settimefromstring (struct tm *tm, const char *buf, const char *rep, tm_time_precedence clock)
{
  char *ret;

  tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
  ret = strptime (buf, "%X", tm);       // time syscall
  if (!ret || *ret)
    ret = strptime (buf, "%EX", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%H:%M:%S", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%H %M %S", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%H:%M", tm);
  if (!ret || *ret)
    ret = strptime (buf, "%H %M", tm);
  if (!ret || *ret)
    return (errno = EINVAL), TM_ERROR;

  if (rep == TM_REF_UNCHANGED)
    rep = tm_getwallclock (*tm);
  rep = tm_getregisteredwallclock (rep, 1);
  tm->tm_zone = rep;
  tm->tm_isdst = -1;            // Let timezone information and system databases define DST flag.

  struct tm copy = *tm;

  time_t retval;
  if (tm_normalize (tm, &retval) == TM_ERROR)
    return (errno = EINVAL), TM_ERROR;
  if (!tm_isutctimezone (rep))
  {
    if (clock == TM_DST_OVER_ST)
      tm_todaylightsavingextrasummertime (tm);
    else if (clock == TM_ST_OVER_DST)
      tm_todaylightsavingextrawintertime (tm);
    else
      return (errno = EINVAL), TM_ERROR;
  }

  if (tm->tm_year == copy.tm_year && tm->tm_mon == copy.tm_mon && tm->tm_mday == copy.tm_mday
      && tm->tm_hour == copy.tm_hour && tm->tm_min == copy.tm_min && tm->tm_sec == copy.tm_sec)
    return TM_OK;
  else
    return (errno = EINVAL), TM_ERROR;
}

tm_status
tm_setfromiso8601 (struct tm *dt, char *str)
{
  // <date>T<time><tz>
  // <date> is YYYY-MM-DD or YYYYMMDD,
  //   YYYY indicates a four-digit year, 0000 through 9999.
  //   MM indicates a two-digit month of the year, 01 through 12.
  //   DD indicates a two-digit day of that month, 01 through 31.
  // <time> is hh:mm:ss.sss or hhmmss.sss or hh:mm:ss or hhmmss or hh:mm or hhmm or hh
  //   hh refers to a zero-padded hour between 00 and 24 (where 24 is only used to denote midnight at the end of a calendar day).
  //  mm refers to a zero-padded minute between 00 and 59.
  //  ss refers to a zero-padded second between 00 and 60.
  // <tz> is Z or ±hh:mm or ±hhmm or ±hh
  // - is '-' or '−'
  const char *regex = "^" "(([0-9][0-9][0-9][0-9])[-−]?([0-9][0-9])[-−]?([0-9][0-9]))"      // AAAAMMDD or AAAA-MM-DD
    "(T([0-9][0-9])([:]?([0-9][0-9])([:]?([0-9][0-9])([.,](([0-9])+))?)?)?)?"   // hhmmss or hh:mm:ss
    "((([-−+])([0-9][0-9]))([:]?([0-9][0-9]))?|Z)?"   // +/-hhmm or +/-hh:mm
    "$";

  regex_t preg;
  if (regcomp (&preg, regex, REG_EXTENDED | REG_NEWLINE))
    abort ();

  regmatch_t pmatch[19 + 1];    // an item for each open parenthesis of regex + 1
  if (regexec (&preg, str, sizeof (pmatch) / sizeof (*pmatch), pmatch, 0) == REG_NOMATCH)
  {
    regfree (&preg);
    return (errno = EINVAL), TM_ERROR;
  }
  regfree (&preg);

  int annee, mois, jour, heure, min, sec, gsign, gheure, gmin;
  int fsec;
  const char *wc;
  heure = min = sec = fsec = gheure = gmin = 0;
  const size_t UTCOFFSET = 14;  // This hard-coded value depend on the variable regex.
  const size_t UTCOFFSETSIGN = 16;      // This hard-coded value depend on the variable regex.
  if (pmatch[UTCOFFSET].rm_so < 0 || pmatch[UTCOFFSET].rm_so >= pmatch[UTCOFFSET].rm_eo)        // no UTC offset information
    wc = TM_REF_LOCALTIME;      // If no UTC relation information is given with a time representation, the time is assumed to be in local time.
  else
  {
    wc = TM_REF_UTC;
    if (strncmp ("Z", str + pmatch[UTCOFFSET].rm_so, (size_t) (pmatch[UTCOFFSET].rm_eo - pmatch[UTCOFFSET].rm_so)) == 0)        // UTC time zone
      gheure = gmin = 0;        // If the time is in UTC, add a Z directly after the time without a space. Z is the zone designator for the zero UTC offset. An offset of zero, in addition to having the special representation "Z", can also be stated numerically as "+00:00", "+0000", or "+00"
    if (pmatch[UTCOFFSETSIGN].rm_so >= 0 && pmatch[UTCOFFSETSIGN].rm_so < pmatch[UTCOFFSETSIGN].rm_eo &&        // if UTC offset does not start with '+'
        strncmp ("+", str + pmatch[UTCOFFSETSIGN].rm_so, (size_t) (pmatch[UTCOFFSETSIGN].rm_eo - pmatch[UTCOFFSETSIGN].rm_so)) != 0)
      gsign = -1;               // '-' or '−'
    else
      gsign = 1;                // '+'
  }

  // relevant subexpression indexes
  /* *INDENT-OFF* */
  struct
  {
    size_t index;
    int *pvar;
    size_t len;
  } parts[] =
  {
    [0] = {2, &annee}, [1] = {3, &mois}, [2] = {4, &jour},
    [3] = {6, &heure}, [4] = {8, &min}, [5] = {10, &sec}, [6] = {12, &fsec},
    [7] = {17, &gheure},[8] = {19, &gmin},
  };
  /* *INDENT-ON* */

  for (size_t part = 0; part < sizeof (parts) / sizeof (*parts); part++)
  {
#define MAXLEN ((size_t)6)
    char substr[MAXLEN + 1];
    substr[MAXLEN] = 0;
    parts[part].len = 0;
    if (pmatch[parts[part].index].rm_so < 0 || pmatch[parts[part].index].rm_so >= pmatch[parts[part].index].rm_eo)
      continue;
    strncpy (substr, str + pmatch[parts[part].index].rm_so, MAXLEN);    // at most MAXLEN bytes are copied, including the terminating null byte ('\0').
    if ((size_t) (pmatch[parts[part].index].rm_eo - pmatch[parts[part].index].rm_so) > MAXLEN)
      substr[parts[part].len = MAXLEN] = 0;
    else
      substr[parts[part].len = (size_t) (pmatch[parts[part].index].rm_eo - pmatch[parts[part].index].rm_so)] = 0;
    *(parts[part].pvar) = atoi (substr);
  }

  float msec = (float) fsec;
  for (size_t i = parts[6].len; i > 0; i--)
    msec /= 10;

  int deltad = 0;
  // Midnight is a special case and may be referred to as either "00:00" or "24:00". "2007-04-05T24:00" is the same instant as "2007-04-06T00:00"
  if (heure == 24)
  {
    deltad = 1;
    heure = 0;
  }
  // 60 is only used to denote an added leap second managed by the system, and is ignored here
  if (sec == 60)
    sec = 59;

  if (tm_set (dt, annee, mois, jour, heure, min, sec, wc) == TM_ERROR)
    return (errno = EINVAL), TM_ERROR;
  if (tm_adddays (dt, deltad) == TM_ERROR)
    return (errno = EINVAL), TM_ERROR;
  if (wc == TM_REF_UTC && tm_addseconds (dt, -1 * gsign * (3600 * gheure + 60 * gmin)) == TM_ERROR)     // To calculate UTC time one has to subtract the offset from the local time
    return (errno = EINVAL), TM_ERROR;
  if (tm_isdefinedinutc (*dt))
  {
    if (tm_changetowallclock (dt, TM_REF_LOCALTIME) == TM_ERROR)
      return (errno = EINVAL), TM_ERROR;
    if (tm_getutcoffset (*dt) != gsign * (3600 * gheure + 60 * gmin) && // Does UTC offset match with local time ?
        tm_changetowallclock (dt, TM_REF_UTC) == TM_ERROR)
      return (errno = EINVAL), TM_ERROR;
  }

  return TM_OK;
}

static tm_status
tm_tostring_fmt (struct tm dt, size_t max, char *str, const char *fmt)
{
  assert (fmt && *fmt);
  if (!str || !max)
    return (errno = EINVAL), TM_ERROR;

  // Provided that the result string, including the terminating null byte, does not exceed max bytes, strftime() returns the number of bytes (excluding the terminating null byte) placed in the array str.
  // If the length of the result string (including the terminating null byte) would exceed max bytes, then strftime() returns 0, and the contents of the array are undefined.
  // The return value 0 does not necessarily indicate an error. But POSIX.1-2001 does not specify any errno settings.
  // This makes it impossible to distinguish this error case from cases where the format string legitimately produces a zero-length output string. For example, in many locales %p yields an empty string.
  return strftime (str, max, fmt, &dt) ? TM_OK : TM_ERROR;      // time syscall.
}

tm_status
tm_toiso8601 (struct tm dt, size_t max, char *str, int sep)
{
  if (sep)
    return tm_tostring_fmt (dt, max, str, "%Y-%m-%dT%H:%M:%S%z");
  else
    return tm_tostring_fmt (dt, max, str, "%Y%m%dT%H%M%S%z");
}

tm_status
tm_timetostring (struct tm dt, size_t max, char *str)
{
  const char *fmt[] = { "%X", "%X (%Z)", "%X (UTC%z)", "%X (%Z,UTC%z)" };
  size_t index = 2U * (tm_isinsidedaylightsavingtimeoverlap (dt) ? 1 : 0) + (tm_isdefinedinlocaltime (dt) ? 0 : 1);
  return tm_tostring_fmt (dt, max, str, fmt[index]);
}

tm_status
tm_datetostring (struct tm dt, size_t max, char *str)
{
  const char *fmt = "%x";
  if (!tm_isdefinedinlocaltime (dt))
    fmt = "%x (%Z)";
  return tm_tostring_fmt (dt, max, str, fmt);
}

tm_status
tm_tostring (struct tm dt, size_t max, char *str)
{
  const char *fmt[] = { "%x %X", "%x %X (%Z)", "%x %X (UTC%z)", "%x %X (%Z,UTC%z)" };
  size_t index = 2U * (tm_isinsidedaylightsavingtimeoverlap (dt) ? 1 : 0) + (tm_isdefinedinlocaltime (dt) ? 0 : 1);
  return tm_tostring_fmt (dt, max, str, fmt[index]);
}

tm_status
dt_tostring (struct tm dt, size_t max, char *str)
{
  return tm_tostring_fmt (dt, max, str, "%x");
}

tm_status
dt_toiso8601 (struct tm dt, size_t max, char *str, int sep)
{
  if (sep)
    return tm_tostring_fmt (dt, max, str, "%Y-%m-%d");
  else
    return tm_tostring_fmt (dt, max, str, "%Y%m%d");
}

tm_status
dt_setfromiso8601 (struct tm *dt, char *str)
{
  if (tm_setfromiso8601 (dt, str) == TM_ERROR)
    return TM_ERROR;

  dt->tm_zone = tm_getregisteredwallclock (TM_REF_UTC, 0);
  return TM_OK;
}

tm_status
dt_make_ir (struct tm *date, const char *wc)
{
  if (tm_make_ir (date, TM_TODAY, wc) == TM_ERROR)
    return TM_ERROR;

  date->tm_zone = tm_getregisteredwallclock (TM_REF_UTC, 0);
  return TM_OK;
}

/*****************************************************
*   CONVERTERS                                       *
**************************************************** */

static tm_status
tm_toutcrepresentation (struct tm *date)
{
  if (!tm_isdefinedinutc (*date))
  {
    time_t local;
    if (tm_normalize (date, &local) != TM_ERROR && gmtime_r (&local, date)      // time syscall
        && date->tm_year + 1900 + 1 > date->tm_year)
    {
      date->tm_zone = tm_getregisteredwallclock (TM_REF_UTC, 0);
      return TM_OK;
    }
    else
      return (errno = EINVAL), TM_ERROR;
  }
  else
    return TM_OK;
}

static tm_status
tm_totimezonerepresentation (struct tm *date, const char *rep)
{
  time_t utc;
  if (tm_normalize (date, &utc) != TM_ERROR)
  {
    rep = tm_getregisteredwallclock (rep, 1);
    pthread_mutex_lock (&tzset_mutex);
    const char *old_tz;
    if (tm_tzset (rep, &old_tz) == TM_ERROR)
    {
      pthread_mutex_unlock (&tzset_mutex);
      return TM_ERROR;
    }
    if (localtime_r (&utc, date)        // time syscall
        && date->tm_year + 1900 + 1 > date->tm_year)
    {
      tm_tzunset (old_tz);
      pthread_mutex_unlock (&tzset_mutex);
      date->tm_zone = rep;
      return TM_OK;
    }
    else
    {
      tm_tzunset (old_tz);
      pthread_mutex_unlock (&tzset_mutex);
      return (errno = EINVAL), TM_ERROR;
    }
  }
  else
    return (errno = EINVAL), TM_ERROR;
}

// wc should conform to format accepted by tzset (see man tzset)
// On Linux, possible values can be listed with command 'find /usr/share/zoneinfo \! -type d | sort'
tm_status
tm_changetowallclock (struct tm *date, const char *wc)
{
  if (tm_isutctimezone (wc))
    return tm_toutcrepresentation (date);
  else
    return tm_totimezonerepresentation (date, wc);
}

/*****************************************************
*   CALENDAR PROPERTIES                              *
*****************************************************/

int
tm_getdaysinyear (int year)
{
  struct tm debut, fin;
  if (tm_set (&debut, year, TM_JANUARY, 1, 0, 0, 0) == TM_OK && tm_set (&fin, year + 1, TM_JANUARY, 1, 0, 0, 0) == TM_OK)
    return tm_diffcalendardays (debut, fin);
  else
    return (errno = EINVAL), 0;
}

int
tm_isleapyear (int year)
{
  // Nonzero if year is a leap year (every 4 years, except every 100th isn't, and every 400th is).
  return tm_getdaysinyear (year) == 366 ? 1 : 0;
  //return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

int                             /* set_errno */
tm_getweeksinisoyear (int isoyear)
{
  struct tm date;

  if (isoyear + 1 < isoyear || tm_make_dtrc (&date, isoyear + 1, TM_JANUARY, 4, 0, 0, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST) == TM_ERROR || tm_adddays (&date, -7) == TM_ERROR)
    return (errno = EINVAL), 0;

  return tm_getisoweek (date);
}

int                             /* set_errno */
tm_getdaysinmonth (int year, tm_month month)
{
  int ret = 1;                  // not 0

  struct tm date;

  if (tm_make_dtrc (&date, year, month, 1, 0, 0, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST) == TM_ERROR)
    return (errno = EINVAL), 0;

  ret -= tm_getdayofyear (date);
  if (tm_addmonths (&date, 1) == TM_ERROR || tm_adddays (&date, -1) == TM_ERROR)
    return (errno = EINVAL), 0;
  ret += tm_getdayofyear (date);

  return ret;
}

int                             /* set_errno */
tm_getsecondsinday (int year, tm_month month, int day, const char *rep)
{
  if (rep == TM_REF_UNCHANGED)
    return (errno = EINVAL), 0;

  struct tm date;

  if (tm_make_dtrc (&date, year, month, day, 0, 0, 0, rep, TM_ST_OVER_DST) == TM_ERROR)
    return (errno = EINVAL), 0;

  time_t deb, fin;

  if (tm_normalize (&date, &deb) == TM_ERROR || tm_adddays (&date, 1) == TM_ERROR || tm_normalize (&date, &fin) == TM_ERROR)
    return (errno = EINVAL), 0;

  return (int) (fin - deb);
}

int                             /* set_errno */
tm_getfirstweekdayinmonth (int year, tm_month month, tm_dayofweek dow)
{
  if (dow < 1 || dow > 7)
    return (errno = EINVAL), 0;

  struct tm date;

  if (tm_make_dtrc (&date, year, month, 1, 0, 0, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST) == TM_ERROR)
    return (errno = EINVAL), 0;

  return (int) ((dow - tm_getdayofweek (date) + 7U) % 7U + 1U);
}

int                             /* set_errno */
tm_getlastweekdayinmonth (int year, tm_month month, tm_dayofweek dow)
{
  if (dow < 1 || dow > 7)
    return (errno = EINVAL), 0;

  struct tm date;

  int savederrno = errno;
  errno = 0;
  int last = tm_getdaysinmonth (year, month);
  if (errno)
    return 0;
  errno = savederrno;

  if (tm_make_dtrc (&date, year, month, last, 0, 0, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST) == TM_ERROR)
    return (errno = EINVAL), 0;

  int diff = (int) (dow - tm_getdayofweek (date));

  return last + diff + (diff > 0 ? -7 : 0);
}

int                             /* set_errno */
tm_getfirstweekdayinisoyear (int isoyear, tm_dayofweek dow)
{
  if (dow < 1 || dow > 7)
    return (errno = EINVAL), 0;

  struct tm date;

  int savederrno = errno;
  errno = 0;
  int ret = tm_getfirstweekdayinmonth (isoyear, TM_JANUARY, dow) + 7;
  if (errno)
    return 0;
  errno = savederrno;

  if (tm_make_dtrc (&date, isoyear, TM_JANUARY, ret, 0, 0, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST) == TM_ERROR)
    return (errno = EINVAL), 0;

  return ret + 7 - 7 * tm_getisoweek (date);
}

/*****************************************************
*   DATE PROPERTIES                                  *
*****************************************************/

tm_dayofweek
tm_getdayofweek (struct tm date)
{
  return (date.tm_wday + 6) % 7 + 1;    /* Monday = 1, Sunday = 7 */
}

tm_month
tm_getmonth (struct tm date)
{
  return date.tm_mon + 1;       /* January = 1, December = 12 */
}

int
tm_getyear (struct tm date)
{
  if (date.tm_year + 1900 < date.tm_year)
    return (errno = EINVAL), 0;
  return date.tm_year + 1900;
}

int
tm_getday (struct tm date)
{
  return date.tm_mday;
}

int
tm_gethour (struct tm date)
{
  return date.tm_hour;
}

int
tm_getminute (struct tm date)
{
  return date.tm_min;
}

int
tm_getsecond (struct tm date)
{
  return date.tm_sec;
}

int
tm_getdayofyear (struct tm date)
{
  return date.tm_yday + 1;      /* 1/1 = 1, 31/12 = 365 or 366 */
}

int
tm_getisoweek (struct tm date)
{

  /** ISO 8601 week date: The first week of a year (starting on Monday) is :
     - the first week that contains at least 4 days of calendar year.
     - the week that contains the first Thursday of a year.
     - the week with January 4 in it
   */

  int week = (date.tm_yday - (date.tm_wday + 6) % 7 + 10) / 7;

  if (week == 0)
    return (date.tm_yday + 365 + tm_isleapyear (date.tm_year + 1900 - 1) - (date.tm_wday + 6) % 7 + 10) / 7;
  else if (week > 52 && ((date.tm_yday - 365 - tm_isleapyear (date.tm_year + 1900) - (date.tm_wday + 6) % 7 + 10) / 7) > 0)
    return (date.tm_yday - 365 - tm_isleapyear (date.tm_year + 1900) - (date.tm_wday + 6) % 7 + 10) / 7;
  else
    return week;
}

int
tm_getisoyear (struct tm date)
{
  /* Year of which ISO week of date belongs to. */
  int week = (date.tm_yday - (date.tm_wday + 6) % 7 + 10) / 7;

  int isoyear;
  if (week == 0)
    isoyear = date.tm_year + 1900 - 1;
  else if (week > 52 && ((date.tm_yday - 365 - tm_isleapyear (date.tm_year + 1900) - (date.tm_wday + 6) % 7 + 10) / 7) > 0)
    isoyear = date.tm_year + 1900 + 1;
  else
    isoyear = date.tm_year + 1900;
  if (isoyear < date.tm_year)
    errno = EINVAL;
  return isoyear;
}

int
tm_isdaylightsavingtimeineffect (struct tm date)
{
  if (date.tm_isdst > 0)
    return 1;
  else if (date.tm_isdst == 0)
    return 0;
  else
    return (errno = EINVAL), 0;
}

int
tm_isinsidedaylightsavingtimeoverlap (struct tm date)
{
  return tm_isdaylightsavingextrasummertime (date) || tm_isdaylightsavingextrawintertime (date);
}

int
tm_getutcoffset (struct tm date)
{
  return (int) date.tm_gmtoff;
}

int                             /* set errno */
tm_getsecondsofday (struct tm date)
{
  struct tm tmp = date;

  if (tm_make_dtrc (&tmp, tm_getyear (date), tm_getmonth (date), tm_getday (date), 0, 0, 0, TM_REF_UNCHANGED,
                    TM_ST_OVER_DST /* Suppose DST does get into effect just before midnight */ ) == TM_ERROR)
    return 0;

  return (int) tm_diffseconds (tmp, date);
}

/*****************************************************
*   OPERATORS                                        *
*****************************************************/

tm_status
tm_addseconds (struct tm *date, long int nbSecs)
{
  time_t t0;
  if (tm_normalize (date, &t0) == TM_ERROR)
    return TM_ERROR;
  if ((nbSecs > 0 && t0 + (time_t) nbSecs < t0) || (nbSecs < 0 && t0 + (time_t) nbSecs > t0))
  {
    errno = EOVERFLOW;
    return TM_ERROR;
  }
  t0 += nbSecs;

  if (tm_isdefinedinwallclock (*date, TM_REF_UTC))
  {
    if ((gmtime_r (&t0, date)   // time syscall
         && date->tm_year + 1900 + 1 > date->tm_year))
    {
      date->tm_zone = tm_getregisteredwallclock (TM_REF_UTC, 0);
      return TM_OK;
    }
    else
    {
      errno = EOVERFLOW;
      return TM_ERROR;
    }
  }
  else
  {
    const char *rep = tm_getregisteredwallclock (date->tm_zone, 0);
    pthread_mutex_lock (&tzset_mutex);
    const char *old_tz;
    if (tm_tzset (rep, &old_tz) == TM_ERROR)
    {
      pthread_mutex_unlock (&tzset_mutex);
      return TM_ERROR;
    }
    if ((localtime_r (&t0, date)        // time syscall
         && date->tm_year + 1900 + 1 > date->tm_year))
    {
      date->tm_zone = rep;
      tm_tzunset (old_tz);
      pthread_mutex_unlock (&tzset_mutex);
      return TM_OK;
    }
    else
    {
      tm_tzunset (old_tz);
      pthread_mutex_unlock (&tzset_mutex);
      errno = EOVERFLOW;
      return TM_ERROR;
    }
  }
}

tm_status
tm_adddays (struct tm *date, int nbDays)
{
  if ((nbDays > 0 && date->tm_mday + nbDays < date->tm_mday) || (nbDays < 0 && date->tm_mday + nbDays > date->tm_mday))
  {
    errno = EOVERFLOW;
    return TM_ERROR;
  }
  date->tm_mday += nbDays;
  date->tm_isdst = -1;          // Let timezone information and system databases define DST flag.

  tm_status ret = tm_normalize (date, 0);
  if (ret != TM_OK)
    errno = EOVERFLOW;
  return ret;
}

tm_status
tm_addmonths (struct tm *date, int nbMonths)
{
  int mday = date->tm_mday;     // Day of the month (1-31)

  if ((nbMonths > 0 && date->tm_mon + nbMonths < date->tm_mon) || (nbMonths < 0 && date->tm_mon + nbMonths > date->tm_mon))
  {
    errno = EOVERFLOW;
    return TM_ERROR;
  }
  date->tm_mon += nbMonths;
  date->tm_isdst = -1;          // Let timezone information and system databases define DST flag.

  tm_status ret = tm_normalize (date, 0);
  if (ret == TM_OK && date->tm_mday != mday)    // Handles lasts days of month
  {
    date->tm_mday = 0;          // A 0 in tm_mday is interpreted as meaning the last day of the preceding month.
    ret = tm_normalize (date, 0);
  }
  else if (ret == TM_ERROR)
    errno = EOVERFLOW;
  return ret;
}

tm_status
tm_trimtime (struct tm *tm)
{
  tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
  tm->tm_isdst = -1;            // Let timezone information and system databases define DST flag.

  return tm_normalize (tm, 0);
}

/*****************************************************
*   COMPARATORS                                      *
*****************************************************/
int
tm_equals (struct tm a, struct tm b)
{
  return (a.tm_sec == b.tm_sec && a.tm_min == b.tm_min && a.tm_hour == b.tm_hour &&
          a.tm_mday == b.tm_mday && a.tm_mon == b.tm_mon && a.tm_year == b.tm_year && a.tm_gmtoff == b.tm_gmtoff && a.tm_zone == b.tm_zone);
}

long int
tm_diffseconds (struct tm debut, struct tm fin)
{
  time_t d, f;
  if (tm_normalize (&fin, &f) == TM_OK && tm_normalize (&debut, &d) == TM_OK)
    return f - d;

  errno = EINVAL;               // Both dates have been normalized at creation, errno should not be set.
  return (time_t) - 1;

}

int
tm_compare (const void *pdebut, const void *pfin)
{
  long int diff = -tm_diffseconds (*(const struct tm *) pdebut, *(const struct tm *) pfin);

  return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}

int                             /* set_errno */
tm_diffcalendardays (struct tm debut, struct tm fin)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  int coeff = 1;
  int ret = 0;

  if (fin.tm_year < debut.tm_year)
  {
    struct tm tmp = debut;

    debut = fin;
    fin = tmp;
    coeff = -1;
  }
  ret = fin.tm_yday - debut.tm_yday;

  debut.tm_mon = 11;
  debut.tm_mday = 31;
  for (; debut.tm_year < fin.tm_year; debut.tm_year++)
  {
    if (tm_normalize (&debut, 0) == TM_ERROR)
      return (errno = EINVAL), TM_ERROR;
    ret += debut.tm_yday + 1;
  }

  return coeff * ret;
}

int                             /* set_errno */
tm_diffdays (struct tm debut, struct tm fin, int *seconds)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  int coeff = 1;

  if (tm_diffseconds (debut, fin) < 0)
  {
    struct tm tmp = debut;

    debut = fin;
    fin = tmp;
    coeff = -1;
  }

  int ret = tm_diffcalendardays (debut, fin);

  if (fin.tm_hour < debut.tm_hour || (fin.tm_hour == debut.tm_hour && fin.tm_min < debut.tm_min) ||
      (fin.tm_hour == debut.tm_hour && fin.tm_min == debut.tm_min && fin.tm_sec < debut.tm_sec))
    if (ret > 0)
      ret--;

  if (seconds)
  {
    tm_adddays (&debut, ret);
    *seconds = coeff * (int) tm_diffseconds (debut, fin);
  }

  return coeff * ret;
}

int                             /* set errno */
tm_diffweeks (struct tm debut, struct tm fin, int *days, int *seconds)
{
  int d = tm_diffdays (debut, fin, seconds);

  if (days)
    *days = d >= 0 ? d % 7 : -((-d) % 7);

  return d / 7;
}

int                             /* set_errno */
tm_diffcalendarmonths (struct tm debut, struct tm fin)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  return 12 * (fin.tm_year - debut.tm_year) + fin.tm_mon - debut.tm_mon;
}

int                             /* set_errno */
tm_diffmonths (struct tm debut, struct tm fin, int *days, int *seconds)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  int coeff = 1;

  if (tm_diffseconds (debut, fin) < 0)
  {
    struct tm tmp = debut;

    debut = fin;
    fin = tmp;
    coeff = -1;
  }

  int ret = tm_diffcalendarmonths (debut, fin);

  if (fin.tm_mday < debut.tm_mday ||
      (fin.tm_mday == debut.tm_mday && fin.tm_hour < debut.tm_hour) ||
      (fin.tm_mday == debut.tm_mday && fin.tm_hour == debut.tm_hour && fin.tm_min < debut.tm_min) ||
      (fin.tm_mday == debut.tm_mday && fin.tm_hour == debut.tm_hour && fin.tm_min == debut.tm_min && fin.tm_sec < debut.tm_sec))
    if (ret > 0)
      ret--;

  tm_addmonths (&debut, ret);
  int d = coeff * tm_diffdays (debut, fin, seconds);
  if (days)
    *days = d;
  if (seconds)
    *seconds *= coeff;

  return coeff * ret;
}

int                             /* set_errno */
tm_diffcalendaryears (struct tm debut, struct tm fin)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  return tm_getyear (fin) - tm_getyear (debut);
}

int                             /* set_errno */
tm_diffyears (struct tm debut, struct tm fin, int *months, int *days, int *seconds)
{
  int m = tm_diffmonths (debut, fin, days, seconds);

  if (months)
    *months = m >= 0 ? m % 12 : -((-m) % 12);
  return m / 12;
}

int                             /* set_errno */
tm_diffisoyears (struct tm debut, struct tm fin)
{
  if (tm_getwallclock (debut) != tm_getwallclock (fin))
    return (errno = EINVAL), 0;

  return tm_getisoyear (fin) - tm_getisoyear (debut);
}

time_t
tm_tobinary (struct tm date)
{
  struct tm dt0;
  tm_set (&dt0, 1970, TM_JANUARY, 1, 0, 0, 0, TM_REF_UTC);      // Epoch, 1970-01-01 00:00:00 +0000 (UTC).
  return tm_diffseconds (dt0, date);
}

tm_status
tm_frombinary (struct tm *date, time_t binary, const char *rep)
{
  if (rep == TM_REF_UNCHANGED)
    rep = tm_getwallclock (*date);

  struct tm dt0;
  tm_set (&dt0, 1970, TM_JANUARY, 1, 0, 0, 0, TM_REF_UTC);      // Epoch, 1970-01-01 00:00:00 +0000 (UTC).
  *date = dt0;
  if (tm_addseconds (date, binary) == TM_OK && tm_changetowallclock (date, rep) == TM_OK)
    return TM_OK;
  else
    return TM_ERROR;
}
