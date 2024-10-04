#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE

#include <check.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include "dates.h"

/*************** INITIALISATION *************************/

static locale_t locale;
// A French seating in Italy:
static const char *Spokenlanguage = "fr_FR.utf8";       // The user spoken language: French language, as spoken in France
static const char *Wallclock = "Europe/Rome";   // The user timezone: Italy
/// Done once for all tests
static void
unckecked_setup (void)
{
  if ((locale = newlocale (LC_ALL_MASK, Spokenlanguage, 0)))
    uselocale (locale);         // uselocale is MT-Safe (better than setlocale which is MT-Unsafe const:locale env)
  ck_assert (locale);
}

static void
unckecked_setup_is_TZ_owner (void)
{
  unckecked_setup ();

  tm_is_TZ_owner = 1;
}

static void
unckecked_setup_is_not_TZ_owner (void)
{
  unckecked_setup ();

  tm_is_TZ_owner = 0;
}

/// Done once for all tests
static void
unckecked_teardown (void)
{
  if (locale)
    freelocale (locale);
  printf ("All done.\n");
}

/// Done once for every test
static void
ckecked_setup (void)
{
  ck_assert (tm_setlocalwallclock (Wallclock) == TM_OK);
}

/// Done once for every test
static void
ckecked_teardown (void)
{
}

/****************************************************/

static void
tm_print (struct tm date, int line)
{
  char buf[1000];

  strftime (buf, sizeof (buf), "* %x %X\n" "* %c\n" "* %Z\n" "* %G-W%V-%u\n" "* %Y-D%j", &date);

  char strd[1000];
  char strt[1000];
  char strdt[1000];

  tm_datetostring (date, 1000, strd);
  tm_timetostring (date, 1000, strt);
  tm_tostring (date, 1000, strdt);

  printf ("---- line %i\n"
          "Structure tm (%p):\n"
          " tm_year: %i\n"
          " tm_mon: %i\n"
          " tm_mday: %i\n"
          " tm_hour: %i\n"
          " tm_min: %i\n"
          " tm_sec: %i\n"
          " tm_wday: %i\n"
          " tm_yday: %i\n"
          " tm_isdst: %i\n"
          " tm_isinsidedaylightsavingtimeoverlap: %i\n"
          " nb_hours: %i\n"
          " tm_gmtoff: %+li\n"
          " tm_zone: %s (%s)\n"
          "%04i-W%02i-%ui\n"
          "%04i-D%03i%+is\n"
          "%s\n"
          "%s (%s ; %s ; %s)\n"
          "----\n",
          line,
          (void *) &date,
          date.tm_year,
          date.tm_mon,
          date.tm_mday,
          date.tm_hour,
          date.tm_min,
          date.tm_sec,
          date.tm_wday,
          date.tm_yday,
          date.tm_isdst,
          tm_isinsidedaylightsavingtimeoverlap (date),
          tm_getsecondsinday (tm_getyear (date), tm_getmonth (date), tm_getday (date)) / 3600,
          date.tm_gmtoff,
          date.tm_zone ? date.tm_zone : "(null)",
          tm_isdefinedinlocaltime (date) ? "local" : tm_isdefinedinutc (date) ? "utc" : "alternate",
          tm_getisoyear (date), tm_getisoweek (date), tm_getdayofweek (date), tm_getyear (date), tm_getdayofyear (date),
          tm_getsecondsofday (date), buf, asctime (&date), strd, strt, strdt);
  fflush (stdout);
}

/************************* TESTS ***********************/
START_TEST (tu_now)
{
  struct tm maintenant;

  tm_set (&maintenant);
  tm_print (maintenant, __LINE__);
  ck_assert (tm_isdefinedinlocaltime (maintenant));
  tm_changetowallclock (&maintenant, TM_REF_UTC);
  tm_print (maintenant, __LINE__);
  ck_assert (tm_getutcoffset (maintenant) == 0);
  ck_assert (tm_isdefinedinutc (maintenant));
}

END_TEST
START_TEST (tu_today)
{
  struct tm aujourdhui;

  tm_set (&aujourdhui, TM_TODAY);
  tm_print (aujourdhui, __LINE__);
  ck_assert (tm_isdefinedinlocaltime (aujourdhui));
  ck_assert (tm_gethour (aujourdhui) == 0);
  ck_assert (tm_getminute (aujourdhui) == 0);
  ck_assert (tm_getsecond (aujourdhui) == 0);
  tm_changetowallclock (&aujourdhui, TM_REF_UTC);
  tm_print (aujourdhui, __LINE__);
  ck_assert (tm_getutcoffset (aujourdhui) == 0);
  ck_assert (tm_isdefinedinutc (aujourdhui));
}

END_TEST
START_TEST (tu_today_utc)
{
  struct tm aujourdhui;

  tm_set (&aujourdhui);         // Now
  ck_assert (tm_isdefinedinlocaltime (aujourdhui));
  tm_changetoutc (&aujourdhui);
  tm_trimtime (&aujourdhui);
  tm_print (aujourdhui, __LINE__);
  ck_assert (tm_getutcoffset (aujourdhui) == 0);
  ck_assert (tm_isdefinedinutc (aujourdhui));
  ck_assert (tm_gethour (aujourdhui) == 0);
  ck_assert (tm_getminute (aujourdhui) == 0);
  ck_assert (tm_getsecond (aujourdhui) == 0);
  tm_changetolocaltime (&aujourdhui);
  ck_assert (tm_gethour (aujourdhui) == tm_getutcoffset (aujourdhui) / 3600);
  tm_print (aujourdhui, __LINE__);
  ck_assert (tm_isdefinedinlocaltime (aujourdhui));
  tm_changetosystemtime (&aujourdhui);
  ck_assert (tm_isdefinedinsystemtime (aujourdhui));
}

END_TEST
START_TEST (tu_local)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 2010, 5, 30, 17, 55, 21) != TM_ERROR);
  struct tm aDate = dt;
  tm_addyears (&aDate, -1);
  tm_addseconds (&aDate, 60 * 60 * 24 * 365);
  ck_assert (tm_equals (aDate, dt));
  ck_assert (tm_set (&dt, 2010, 4, 31, 17, 55, 21) == TM_ERROR);        // Does not exist
  ck_assert (tm_set (&dt, 2015, 2, 29, 17, 55, 21) == TM_ERROR);
  ck_assert (tm_set (&dt, 2016, 2, 29, 17, 55, 21) != TM_ERROR);
  ck_assert (tm_set (&dt, 2016, 2, 29, 17, 65, 21) == TM_ERROR);        // Does not exist
  ck_assert (tm_set (&dt, 2016, 3, 27, 2, 12, 21) == TM_ERROR); // Does not exist (DST change)
  ck_assert (tm_set (&dt, 2016, 10, 30, 2, 22, 21) != TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (dt));
  tm_print (dt, __LINE__);
  ck_assert (tm_set (&dt, 2016, 10, 30, 2, 22, 21, TM_REF_LOCALTIME, TM_DST_OVER_ST) != TM_ERROR);
  ck_assert (tm_setlocalwallclock ("Europe/Berlin") == TM_OK);
  ck_assert (!tm_isdefinedinlocaltime (dt));
  tm_print (dt, __LINE__);
}

END_TEST
START_TEST (tu_utc)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 1969, 12, 31, 23, 59, 59, TM_REF_UTC) != TM_ERROR);
  struct tm aDate = dt;
  tm_addyears (&aDate, -1);
  tm_addseconds (&aDate, 60 * 60 * 24 * 365);
  ck_assert (tm_equals (aDate, dt));
  ck_assert (tm_set (&dt, 2010, 5, 30, 17, 55, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_set (&dt, 2010, 4, 31, 17, 55, 21, TM_REF_UTC) == TM_ERROR);    // Does not exist
  ck_assert (tm_set (&dt, 2015, 2, 29, 17, 55, 21, TM_REF_UTC) == TM_ERROR);    // Does not exist
  ck_assert (tm_set (&dt, 2016, 2, 29, 17, 55, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_set (&dt, 2016, 2, 29, 17, 65, 21, TM_REF_UTC) == TM_ERROR);    // Does not exist
  ck_assert (tm_set (&dt, 2016, 3, 27, 2, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_set (&dt, 2016, 10, 30, 2, 22, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_getutcoffset (dt) == 0);
  ck_assert (tm_isdefinedinutc (dt));
}

END_TEST
START_TEST (tu_set_from_local)
{
  struct tm dt;

  tm_set (&dt);
  ck_assert (tm_isdefinedinlocaltime (dt));
  ck_assert (tm_set (&dt, 2016, 3, 27, 2, 12, 21) == TM_ERROR); // Missing hour when switching to DST
  ck_assert (tm_isdefinedinlocaltime (dt));
  ck_assert (tm_set (&dt, 2012, 12, 31, 23, 59, 59) != TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (dt));

  ck_assert (tm_setdatefromstring (&dt, "23/4/1987") != TM_ERROR);
  ck_assert (tm_setdatefromstring (&dt, "23/09/1987") != TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (dt));
  ck_assert (tm_setdatefromstring (&dt, "33/4/1987") == TM_ERROR);
  ck_assert (tm_setdatefromstring (&dt, "23/4") == TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (dt));
  ck_assert (tm_setdatefromstring (&dt, "1999-03-17") != TM_ERROR);     // ISO format
  ck_assert (tm_setdatefromstring (&dt, "1999-02-29") == TM_ERROR);

  ck_assert (tm_settimefromstring (&dt, "23:04") != TM_ERROR);
  ck_assert (tm_settimefromstring (&dt, "25:04") == TM_ERROR);  // 25
  ck_assert (tm_isdefinedinlocaltime (dt));
  ck_assert (tm_settimefromstring (&dt, "33/4/1987") == TM_ERROR);      // date
  ck_assert (tm_settimefromstring (&dt, "23:04:03") != TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (dt));
}

END_TEST
START_TEST (tu_set_from_utc)
{
  struct tm dt;

  tm_set (&dt);
  tm_changetowallclock (&dt, TM_REF_UTC);
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_set (&dt, 2012, 12, 31, 23, 59, 59, TM_REF_UNCHANGED) != TM_ERROR);
  ck_assert (tm_set (&dt, 1970, 1, 1, 0, 0, 0, TM_REF_UNCHANGED) == TM_OK);
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_getutcoffset (dt) == 0);

  ck_assert (tm_setdatefromstring (&dt, "23/4/1987") != TM_ERROR);
  ck_assert (tm_setdatefromstring (&dt, "23/09/1987") != TM_ERROR);
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_setdatefromstring (&dt, "33/4/1987") == TM_ERROR);      // 33
  ck_assert (tm_setdatefromstring (&dt, "23/4") == TM_ERROR);
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_getutcoffset (dt) == 0);
  ck_assert (tm_setdatefromstring (&dt, "1999-03-17") != TM_ERROR);     // ISO format
  ck_assert (tm_setdatefromstring (&dt, "1999-02-29") == TM_ERROR);

  ck_assert (tm_settimefromstring (&dt, "23:04") != TM_ERROR);
  ck_assert (tm_settimefromstring (&dt, "25:04") == TM_ERROR);  // 25
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_settimefromstring (&dt, "33/4/1987") == TM_ERROR);      // date
  ck_assert (tm_settimefromstring (&dt, "23:04:03") != TM_ERROR);
  ck_assert (tm_isdefinedinutc (dt));
  ck_assert (tm_getutcoffset (dt) == 0);
}

END_TEST
START_TEST (tu_tostring_local)
{
  struct tm dt;
  char str[100];

  ck_assert (tm_set (&dt, 2012, 12, 31, 23, 59, 59) != TM_ERROR);

  ck_assert (tm_datetostring (dt, 0, 0) == TM_ERROR);
  ck_assert (tm_datetostring (dt, 0, str) == TM_ERROR);

  ck_assert (tm_datetostring (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "31/12/2012") == 0);
  ck_assert (tm_timetostring (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "23:59:59") == 0);
  ck_assert (tm_toiso8601 (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "20121231T235959+0100") == 0);
}

END_TEST
START_TEST (tu_tostring_utc)
{
  struct tm dt;
  char str[100];

  ck_assert (tm_set (&dt, 2013, 11, 30, 22, 58, 57, TM_REF_UTC) != TM_ERROR);

  ck_assert (tm_datetostring (dt, 0, 0) == TM_ERROR);
  ck_assert (tm_datetostring (dt, 0, str) == TM_ERROR);

  ck_assert (tm_datetostring (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "30/11/2013 (UTC)") == 0);
  ck_assert (tm_timetostring (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "22:58:57 (UTC)") == 0);
  ck_assert (tm_tostring (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "30/11/2013 22:58:57 (UTC)") == 0);
  ck_assert (tm_toiso8601 (dt, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "20131130T225857+0000") == 0);
}

END_TEST
START_TEST (tu_getters_local)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 2012, 12, 31, 23, 59, 58) != TM_ERROR);
  ck_assert (tm_getyear (dt) == 2012);
  ck_assert (tm_getmonth (dt) == TM_DECEMBER);
  ck_assert (tm_getday (dt) == 31);
  ck_assert (tm_gethour (dt) == 23);
  ck_assert (tm_getminute (dt) == 59);
  ck_assert (tm_getsecond (dt) == 58);
  ck_assert (tm_getdayofyear (dt) == 366);
  ck_assert (tm_getdayofweek (dt) == TM_MONDAY);
}

END_TEST
START_TEST (tu_getters_utc)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 2013, 11, 30, 22, 58, 57, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_getyear (dt) == 2013);
  ck_assert (tm_getmonth (dt) == TM_NOVEMBER);
  ck_assert (tm_getday (dt) == 30);
  ck_assert (tm_gethour (dt) == 22);
  ck_assert (tm_getminute (dt) == 58);
  ck_assert (tm_getsecond (dt) == 57);
  ck_assert (tm_getdayofyear (dt) == 334);
  ck_assert (tm_getdayofweek (dt) == TM_SATURDAY);
}

END_TEST
START_TEST (tu_ops_local)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 9) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_FEBRUARY);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, TM_MARCH, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 2);

  ck_assert (tm_set (&dt, 2016, TM_MARCH, 26, 2, 12, 21) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 1) == TM_OK);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 3);

  ck_assert (tm_set (&dt, 2016, TM_MARCH, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, TM_FEBRUARY, 27, 2, 12, 21) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 1) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_MARCH);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 3);

  ck_assert (tm_set (&dt, 2016, TM_MARCH, 27, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 9) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_DECEMBER);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 10, 30, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 5);
  ck_assert (tm_gethour (dt) == 0);

  ck_assert (tm_set (&dt, 2016, 10, 30, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 5);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, TM_OCTOBER, 30, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 6) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_APRIL);
  ck_assert (tm_getday (dt) == 30);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, TM_OCTOBER, 31, 1, 12, 21) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 1) == TM_OK);
  ck_assert (tm_getday (dt) == 30);
  ck_assert (tm_addmonths (&dt, 3) == TM_OK);
  ck_assert (tm_getday (dt) == 28);
  ck_assert (tm_addmonths (&dt, -4) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_OCTOBER);
  ck_assert (tm_getday (dt) == 28);

  ck_assert (tm_set (&dt, 2016, 3, 26, 2, 12, 21) != TM_ERROR);
  ck_assert (tm_getday (dt) == 26);
  ck_assert (tm_gethour (dt) == 2);
  ck_assert (tm_adddays (&dt, 1) == TM_OK);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 3);
  ck_assert (tm_adddays (&dt, -1) == TM_OK);
  ck_assert (tm_getday (dt) == 26);
  ck_assert (tm_gethour (dt) == 3);

  ck_assert (tm_set (&dt, 2016, TM_MARCH, 14, 9, 0, 0) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 14) == TM_OK);
  ck_assert (tm_getday (dt) == 28);
  ck_assert (tm_gethour (dt) == 9);
}

END_TEST
START_TEST (tu_ops_utc)
{
  struct tm dt;

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 5, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 9) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_FEBRUARY);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 3, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 3, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 2);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 3, 27, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 9) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_DECEMBER);
  ck_assert (tm_getday (dt) == 27);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 10, 30, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addhours (&dt, 6 * 24) == TM_OK);
  ck_assert (tm_getday (dt) == 5);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 10, 30, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_adddays (&dt, 6) == TM_OK);
  ck_assert (tm_getday (dt) == 5);
  ck_assert (tm_gethour (dt) == 1);

  ck_assert (tm_set (&dt, 2016, 10, 30, 1, 12, 21, TM_REF_UTC) != TM_ERROR);
  ck_assert (tm_addmonths (&dt, 6) == TM_OK);
  ck_assert (tm_getmonth (dt) == TM_APRIL);
  ck_assert (tm_getday (dt) == 30);
  ck_assert (tm_gethour (dt) == 1);
}

END_TEST
START_TEST (tu_dst)
{
  ck_assert (tm_getsecondsinday (2016, 9, 27) == 24 * 3600);
  ck_assert (tm_getsecondsinday (2016, 10, 30) == 25 * 3600);
  ck_assert (tm_getsecondsinday (2016, 3, 27) == 23 * 3600);

  ck_assert (tm_getsecondsinday (2016, 9, 27, TM_REF_UTC) == 24 * 3600);
  ck_assert (tm_getsecondsinday (2016, 10, 30, TM_REF_UTC) == 24 * 3600);
  ck_assert (tm_getsecondsinday (2016, 3, 27, TM_REF_UTC) == 24 * 3600);
}

END_TEST
START_TEST (tu_dst_summer)
{
  struct tm aDate;

  tm_set (&aDate, 2016, 3, 27, 1, 30, 0);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 1);
  int utcoffset = tm_getutcoffset (aDate);

  ck_assert (tm_getsecondsofday (aDate) == 3600 + 1800);

  tm_addminutes (&aDate, 60);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 3);
  ck_assert (tm_getutcoffset (aDate) == utcoffset + 3600);
  ck_assert (tm_getsecondsofday (aDate) == 2 * 3600 + 1800);
}

END_TEST
START_TEST (tu_dst_winter)
{
  struct tm aDate;

  tm_set (&aDate, 2002, 10, 27, 1, 30, 0);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 1);
  int utcoffset = tm_getutcoffset (aDate);
  ck_assert (tm_getsecondsofday (aDate) == 3600 + 1800);

  struct tm bDate = aDate;
  tm_addyears (&bDate, -1);
  tm_addseconds (&bDate, 60 * 60 * 24 * 365);
  ck_assert (tm_equals (aDate, bDate));

  tm_addseconds (&aDate, 3600);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 2 * 3600 + 1800);

  tm_addminutes (&aDate, 60);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 3 * 3600 + 1800);

  tm_addhours (&aDate, 1);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 3);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 4 * 3600 + 1800);

  tm_addseconds (&aDate, -3600);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 3 * 3600 + 1800);

  tm_addminutes (&aDate, -60);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 2 * 3600 + 1800);

  tm_addhours (&aDate, -1);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 1);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 1, 30, 0, TM_REF_LOCALTIME, TM_DST_OVER_ST);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 1);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 1, 30, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST);
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 1);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 2, 30, 0, TM_REF_LOCALTIME, TM_DST_OVER_ST);    // 2.30am Summer time
  ck_assert (tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset);
  ck_assert (tm_getsecondsofday (aDate) == 2 * 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 2, 30, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST);    // 2.30am Winter time
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 2);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 3 * 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 3, 30, 0, TM_REF_LOCALTIME, TM_DST_OVER_ST);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 3);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 4 * 3600 + 1800);

  tm_set (&aDate, 2002, 10, 27, 3, 30, 0, TM_REF_LOCALTIME, TM_ST_OVER_DST);
  ck_assert (!tm_isdaylightsavingtimeineffect (aDate));
  ck_assert (!tm_isinsidedaylightsavingtimeoverlap (aDate));
  ck_assert (tm_gethour (aDate) == 3);
  ck_assert (tm_getutcoffset (aDate) == utcoffset - 3600);
  ck_assert (tm_getsecondsofday (aDate) == 4 * 3600 + 1800);
}

END_TEST
START_TEST (tu_iso)
{
  struct tm aDate;

  tm_set (&aDate, 2003, 12, 28, 10, 0, 0);
  ck_assert (tm_getisoyear (aDate) == 2003);
  ck_assert (tm_getisoweek (aDate) == 52);
  ck_assert (tm_getweeksinisoyear (2003) == 52);

  tm_set (&aDate, 2003, 12, 29, 10, 0, 0);
  ck_assert (tm_getisoyear (aDate) == 2004);
  ck_assert (tm_getisoweek (aDate) == 1);

  tm_set (&aDate, 2005, 1, 2, 10, 0, 0);
  ck_assert (tm_getisoyear (aDate) == 2004);
  ck_assert (tm_getisoweek (aDate) == 53);
  ck_assert (tm_getweeksinisoyear (2004) == 53);

  tm_set (&aDate, 2005, 1, 3, 10, 0, 0);
  ck_assert (tm_getisoyear (aDate) == 2005);
  ck_assert (tm_getisoweek (aDate) == 1);
  ck_assert (tm_getweeksinisoyear (2005) == 52);
}

END_TEST
START_TEST (tu_calendar)
{
  ck_assert (!tm_isleapyear (1900));
  ck_assert (tm_isleapyear (2000));
  ck_assert (!tm_isleapyear (2001));
  ck_assert (tm_isleapyear (2004));
  ck_assert (tm_getdaysinyear (1900) == 365);
  ck_assert (tm_getdaysinyear (2000) == 366);
  ck_assert (tm_getdaysinyear (2001) == 365);
  ck_assert (tm_getdaysinyear (2004) == 366);

  ck_assert (tm_getdaysinmonth (2000, 1) == 31);
  ck_assert (tm_getdaysinmonth (2000, 2) == 29);
  ck_assert (tm_getdaysinmonth (2000, 3) == 31);
  ck_assert (tm_getdaysinmonth (2000, 4) == 30);
  ck_assert (tm_getdaysinmonth (2000, 5) == 31);
  ck_assert (tm_getdaysinmonth (2000, 6) == 30);
  ck_assert (tm_getdaysinmonth (2000, 7) == 31);
  ck_assert (tm_getdaysinmonth (2000, 8) == 31);
  ck_assert (tm_getdaysinmonth (2000, 9) == 30);
  ck_assert (tm_getdaysinmonth (2000, 10) == 31);
  ck_assert (tm_getdaysinmonth (2000, 11) == 30);
  ck_assert (tm_getdaysinmonth (2000, 12) == 31);
}

END_TEST
START_TEST (tu_diff_local)
{
  struct tm debut, fin;
  int m, d, s;

  ck_assert (tm_set (&debut, 2015, 11, 28, 11, 20, 0) == TM_OK);
  ck_assert (tm_set (&fin, 2016, 7, 16, 9, 20, 57) == TM_OK);

  ck_assert (tm_diffseconds (debut, fin) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) * 24 * 3600 - 12 * 24 * 3600 - 2 * 3600 - 3600 + 57);
  ck_assert (tm_diffcalendardays (debut, fin) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12);
  ck_assert (tm_diffdays (debut, fin, &s) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12 - 1);
  ck_assert (s == 22 * 3600 + 57);
  ck_assert (tm_diffweeks (debut, fin, &d, &s) == ((30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12 - 1) / 7);
  ck_assert (d == 6 && s == 22 * 3600 + 57);
  ck_assert (tm_diffcalendarmonths (debut, fin) == 12 - 4);
  ck_assert (tm_diffmonths (debut, fin, &d, &s) == 12 - 4 - 1);
  ck_assert (d == 17 && s == 22 * 3600 + 57);
  ck_assert (tm_diffcalendaryears (debut, fin) == 1);
  ck_assert (tm_diffyears (debut, fin, &m, &d, &s) == 0);
  ck_assert (m == 7 && d == 17 && s == 22 * 3600 + 57);
  ck_assert (tm_diffisoyears (debut, fin) == 1);

  ck_assert (tm_set (&debut, 2016, TM_MARCH, 14, 9, 0, 0) == TM_OK);
  ck_assert (tm_set (&fin, 2016, TM_MARCH, 28, 9, 0, 0) == TM_OK);

  ck_assert (tm_diffdays (debut, fin) == 14);
  ck_assert (tm_diffseconds (debut, fin) == 335 * 3600);        // 335 hours instaed of 336
}

END_TEST
START_TEST (tu_diff_utc)
{
  struct tm debut, fin;
  int m, d, s;

  ck_assert (tm_set (&debut, 2015, 11, 28, 11, 20, 50, TM_REF_UTC) == TM_OK);
  ck_assert (tm_set (&fin, 2016, 7, 16, 9, 20, 0, TM_REF_UTC) == TM_OK);

  ck_assert (tm_diffseconds (debut, fin) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) * 24 * 3600 - 12 * 24 * 3600 - 2 * 3600 - 50);
  ck_assert (tm_diffcalendardays (debut, fin) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12);
  ck_assert (tm_diffdays (debut, fin, &s) == (30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12 - 1);
  ck_assert (s == 22 * 3600 - 50);
  ck_assert (tm_diffweeks (debut, fin, &d, &s) == ((30 + 31 + 31 + 29 + 31 + 30 + 31 + 30) - 12 - 1) / 7);
  ck_assert (d == 6 && s == 22 * 3600 - 50);
  ck_assert (tm_diffcalendarmonths (debut, fin) == 12 - 4);
  ck_assert (tm_diffmonths (debut, fin, &d, &s) == 12 - 4 - 1);
  ck_assert (d == 17 && s == 22 * 3600 - 50);
  ck_assert (tm_diffcalendaryears (debut, fin) == 1);
  ck_assert (tm_diffyears (debut, fin, &m, &d, &s) == 0);
  ck_assert (m == 7 && d == 17 && s == 22 * 3600 - 50);
  ck_assert (tm_diffisoyears (debut, fin) == 1);

  ck_assert (tm_set (&debut, 2016, TM_MARCH, 14, 9, 0, 0, TM_REF_UTC) == TM_OK);
  ck_assert (tm_set (&fin, 2016, TM_MARCH, 28, 9, 0, 0, TM_REF_UTC) == TM_OK);

  ck_assert (tm_diffdays (debut, fin, &s) == 14);
  ck_assert (tm_diffseconds (debut, fin) == 336 * 3600);
}

END_TEST
START_TEST (tu_equality)
{
  struct tm now, utcnow;

  tm_set (&now);
  utcnow = now;
  tm_changetowallclock (&utcnow, TM_REF_UTC);

  ck_assert (tm_diffseconds (now, utcnow) == 0);
  ck_assert (tm_compare (&now, &utcnow) == 0);
  ck_assert (tm_equals (now, utcnow) == 0);
}

END_TEST
START_TEST (tu_change_timezone)
{
  char str[200];
  // Make use of command zdump -v -c2009,2011 'Europe/Kiev' 'Australia/Adelaide' 'America/Los_Angeles' 'Europe/Paris' for informations on timezones

  // Setting local time in Kiev
  ck_assert (tm_setlocalwallclock ("Europe/Kyiv") == TM_OK);    // Changes from Kiev to Kyiv since 2022 (use tzselect to list timezones).
  struct tm ki;

  tm_set (&ki, 2010, TM_MARCH, 21, 18, 0, 0);   // UTC+2h
  ck_assert (tm_isdaylightsavingtimeineffect (ki) == 0);        // DST starts on march, the 28th, at 3am on Kiev
  tm_print (ki, __LINE__);
  ck_assert (tm_tostring (ki, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "21/03/2010 18:00:00") == 0);

  // Time in Adelaide, 8h30 ahead of Kiev (UTC+10:30h)  -- TZ=Australia/Adelaide date --date='TZ="Europe/Kyiv" 03/21/2010 18:00'
  ck_assert (tm_changetowallclock (&ki, "Australia/Adelaide") == TM_OK);
  tm_print (ki, __LINE__);
  ck_assert (tm_tostring (ki, sizeof (str) / sizeof (*str), str) == TM_OK);
  ck_assert (strcmp (str, "22/03/2010 02:30:00 (Australia/Adelaide)") == 0);

  ck_assert (tm_isdaylightsavingtimeineffect (ki) != 0);        // DST ends on april, the 4th, at 2am
  ck_assert (tm_getday (ki) == 21 + (18 + 8) / 24);
  ck_assert (tm_gethour (ki) == (18 + 8) % 24);
  ck_assert (tm_getminute (ki) == (0 + 30) % 60);

  // Time in Los Angeles, 9h before Kiev (UTC-7h)
  ck_assert (tm_changetowallclock (&ki, "America/Los_Angeles") == TM_OK);

  ck_assert (tm_isdaylightsavingtimeineffect (ki) != 0);        // DST starts on march, the 14th, at 2am
  ck_assert (tm_getday (ki) == 21 + (18 - 9) / 24);
  ck_assert (tm_gethour (ki) == (18 - 9) % 24);
  ck_assert (tm_getminute (ki) == (0 + 0) % 60);

  // Time in Paris, 1h before Kiev (UTC+1h)
  ck_assert (tm_changetowallclock (&ki, "Europe/Paris") == TM_OK);

  ck_assert (tm_isdaylightsavingtimeineffect (ki) == 0);        // DST starts on march, the 28th, at 2am
  ck_assert (tm_getday (ki) == 21 + (18 - 1) / 24);
  ck_assert (tm_gethour (ki) == (18 - 1) % 24);
  ck_assert (tm_getminute (ki) == (0 + 0) % 60);

  ck_assert (tm_changetowallclock (&ki, "Pacific/Kiritimati") == TM_OK);
  ck_assert (tm_getutcoffset (ki) == 14 * 3600);
  ck_assert (tm_changetowallclock (&ki, "Pacific/Tahiti") == TM_OK);
  ck_assert (tm_getutcoffset (ki) == -10 * 3600);
}

END_TEST
START_TEST (tu_serialization)
{
  struct tm utc;

  tm_set (&utc, 2016, TM_JANUARY, 1, 18, 0, 0, TM_REF_UTC);
  ck_assert (tm_isdefinedinutc (utc));

  long int instant = tm_tobinary (utc);
  (void) instant;
  struct tm local;

  ck_assert (tm_frombinary (&local, instant) == TM_OK);
  ck_assert (tm_isdefinedinlocaltime (local));

  ck_assert (tm_diffseconds (utc, local) == 0);
  ck_assert (tm_compare (&utc, &local) == 0);
  ck_assert (tm_equals (utc, local) == 0);

  tm_changetowallclock (&utc, TM_REF_LOCALTIME);
  ck_assert (tm_isdefinedinlocaltime (utc));

  ck_assert (tm_diffseconds (utc, local) == 0);
  ck_assert (tm_compare (&utc, &local) == 0);
  ck_assert (tm_equals (utc, local) != 0);
}

END_TEST static int
tu_day_loop_check (struct tm start, const char *result)
{
  char string[8];
  tm_trimtime (&start);
  struct tm stop = start;
  tm_adddays (&stop, 1);
  for (struct tm dt = start; tm_compare (&dt, &stop) < 0; tm_addhours (&dt, 1))
  {
    int dc = tm_isinsidedaylightsavingtimeoverlap (dt);
    int dst = tm_isdaylightsavingtimeineffect (dt);
    sprintf (string, "%02i-%02i%s;", tm_gethour (dt), tm_gethour (dt) + 1, dc ? (dst ? "A" : "B") : " ");
    if (strncmp (string, result, 7) != 0)
      return TM_ERROR;
    result += 7;
  }
  return *result ? TM_ERROR : TM_OK;
}

START_TEST (tu_day_loop)
{
  struct tm hour;
  const char *result;

  result = "00-01 ;01-02 ;03-04 ;04-05 ;05-06 ;06-07 ;07-08 ;08-09 ;09-10 ;10-11 ;11-12 ;12-13 ;13-14 ;14-15 ;15-16 ;16-17 ;17-18 ;18-19 ;19-20 ;20-21 ;21-22 ;22-23 ;23-24 ;";
  tm_set (&hour, 2016, TM_MARCH, 27, 0, 0, 0);
  ck_assert (tu_day_loop_check (hour, result) == TM_OK);

  result =
    "00-01 ;01-02 ;02-03 ;03-04 ;04-05 ;05-06 ;06-07 ;07-08 ;08-09 ;09-10 ;10-11 ;11-12 ;12-13 ;13-14 ;14-15 ;15-16 ;16-17 ;17-18 ;18-19 ;19-20 ;20-21 ;21-22 ;22-23 ;23-24 ;";
  tm_set (&hour, 2016, TM_AUGUST, 27, 0, 0, 0);
  ck_assert (tu_day_loop_check (hour, result) == TM_OK);

  result =
    "00-01 ;01-02 ;02-03A;02-03B;03-04 ;04-05 ;05-06 ;06-07 ;07-08 ;08-09 ;09-10 ;10-11 ;11-12 ;12-13 ;13-14 ;14-15 ;15-16 ;16-17 ;17-18 ;18-19 ;19-20 ;20-21 ;21-22 ;22-23 ;23-24 ;";
  tm_set (&hour, 2016, TM_OCTOBER, 30, 0, 0, 0);
  ck_assert (tu_day_loop_check (hour, result) == TM_OK);
}

END_TEST
START_TEST (tu_beginingoftheday)
{
  struct tm date;

  tm_set (&date, 2016, TM_DECEMBER, 25, 14, 58, 39);
  ck_assert (tm_trimtime (&date) != TM_ERROR);
  ck_assert (tm_isdefinedinlocaltime (date));
  ck_assert (tm_getyear (date) == 2016);
  ck_assert (tm_getmonth (date) == TM_DECEMBER);
  ck_assert (tm_getday (date) == 25);
  ck_assert (tm_gethour (date) == 0);
  ck_assert (tm_getminute (date) == 0);
  ck_assert (tm_getsecond (date) == 0);
  ck_assert (tm_getsecondsofday (date) == 0);

  tm_set (&date, 2016, TM_DECEMBER, 25, 14, 58, 39, TM_REF_UTC);
  ck_assert (tm_trimtime (&date) != TM_ERROR);
  ck_assert (tm_isdefinedinutc (date));
  ck_assert (tm_getyear (date) == 2016);
  ck_assert (tm_getmonth (date) == TM_DECEMBER);
  ck_assert (tm_getday (date) == 25);
  ck_assert (tm_gethour (date) == 0);
  ck_assert (tm_getminute (date) == 0);
  ck_assert (tm_getsecond (date) == 0);
  ck_assert (tm_getsecondsofday (date) == 0);
}

END_TEST
START_TEST (tu_moon_walk)
{
  struct tm moon_walk;
  // Armstrong became the first person to step onto the lunar surface on July 21 at 02:56 UTC
  ck_assert (tm_set (&moon_walk, 1969, TM_JULY, 21, 2, 56, 0, TM_REF_UTC) == TM_OK);
  ck_assert (tm_getutcoffset (moon_walk) == 0);
  //tm_print (moon_walk);

  // TZ='Europe/Rome' date --date='TZ="UTC" 1969-07-21 02:56:00' +'%Y-%m-%d %H:%M:%S' returns '1969-07-21 04:56:00'
  tm_changetowallclock (&moon_walk, TM_REF_LOCALTIME);
  ck_assert (tm_gethour (moon_walk) == 4);      // in Rome
  ck_assert (tm_getutcoffset (moon_walk) == 3600 * 2);
  ck_assert (tm_isdaylightsavingtimeineffect (moon_walk) == 1);

  // TZ='Europe/Chisinau' date --date='TZ="UTC" 1969-07-21 02:56:00' +'%Y-%m-%d %H:%M:%S' returns '1969-07-21 05:56:00'
  ck_assert (tm_changetowallclock (&moon_walk, "Europe/Chisinau") == TM_OK);
  // Moon walk time in Moldova: 1969-07-21 05:56:00 +03:00
  ck_assert (tm_getyear (moon_walk) == 1969);
  ck_assert (tm_getmonth (moon_walk) == TM_JULY);
  ck_assert (tm_getday (moon_walk) == 21);
  ck_assert (tm_gethour (moon_walk) == 5);
  ck_assert (tm_getminute (moon_walk) == 56);
  ck_assert (tm_getsecond (moon_walk) == 0);
  ck_assert (tm_isdaylightsavingtimeineffect (moon_walk) == 0);
  ck_assert (tm_getutcoffset (moon_walk) == 3600 * 3);

  // TZ='Europe/Paris' date --date='TZ="UTC" 1969-07-21 02:56:00' +'%Y-%m-%d %H:%M:%S' returns '1969-07-21 03:56:00'
  ck_assert (tm_changetowallclock (&moon_walk, "Europe/Paris") == TM_OK);
  // Moon walk time in Paris: 1969-07-21 03:56:00 +01:00
  ck_assert (tm_getyear (moon_walk) == 1969);
  ck_assert (tm_getmonth (moon_walk) == TM_JULY);
  ck_assert (tm_getday (moon_walk) == 21);
  ck_assert (tm_gethour (moon_walk) == 3);
  ck_assert (tm_getminute (moon_walk) == 56);
  ck_assert (tm_getsecond (moon_walk) == 0);
  ck_assert (tm_getutcoffset (moon_walk) == 3600 * 1);
  ck_assert (tm_isdaylightsavingtimeineffect (moon_walk) == 0); // There were no DST in Paris by that time...
  ck_assert (tm_getutcoffset (moon_walk) == 3600 * 1);

  ck_assert (tm_changetowallclock (&moon_walk, "Australia/Sydney") == TM_OK);

  // Moon walk in Sydney: 1969-07-21 12:56:00 +10:00
  ck_assert (tm_getyear (moon_walk) == 1969);
  ck_assert (tm_getmonth (moon_walk) == TM_JULY);
  ck_assert (tm_getday (moon_walk) == 21);
  ck_assert (tm_gethour (moon_walk) == 12);
  ck_assert (tm_getminute (moon_walk) == 56);
  ck_assert (tm_getsecond (moon_walk) == 0);
}

END_TEST
START_TEST (tu_weekday)
{
  ck_assert (tm_getdaysinmonth (2047, TM_MARCH) == 31);
  ck_assert (tm_getfirstweekdayinmonth (2017, TM_MARCH, TM_TUESDAY) == 7);
  ck_assert (tm_getfirstweekdayinmonth (2017, TM_MARCH, TM_SATURDAY) == 4);

  ck_assert (tm_getlastweekdayinmonth (2017, TM_MARCH, TM_TUESDAY) == 28);
  ck_assert (tm_getlastweekdayinmonth (2017, TM_MARCH, TM_SATURDAY) == 25);

  ck_assert (tm_getfirstweekdayinisoyear (2017, TM_TUESDAY) == 3);
  ck_assert (tm_getfirstweekdayinisoyear (2017, TM_SUNDAY) == 8);
}

END_TEST
START_TEST (tu_error)
{
  struct tm dt;
  ck_assert (tm_set (&dt) == TM_OK);
  ck_assert (tm_set (&dt, TM_TODAY, TM_REF_LOCALTIME) == TM_OK);
  ck_assert (tm_set (&dt, TM_TODAY, "Nowhere") == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 15, 9, 51, 7) == TM_OK);
  ck_assert (tm_set (&dt, 2001, 13, 15, 9, 51, 7) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 31, 9, 51, 7) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 15, 24, 51, 7) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 15, 9, 61, 7) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 15, 9, 51, 67) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2001, 9, 15, 9, 51, 7, TM_REF_UTC, 8) == TM_OK);
  ck_assert (tm_set (&dt, 2001, 9, 15, 9, 51, 7, TM_REF_LOCALTIME, 8) == TM_ERROR && errno == EINVAL);
  ck_assert (tm_set (&dt, 2000000000, 9, 15, 9, 51, 7) == TM_OK);
  ck_assert (tm_set (&dt, -2000000000, 9, 15, 9, 51, 7) == TM_OK);

  tm_changetowallclock (&dt, TM_REF_LOCALTIME);
  ck_assert (tm_setdatefromstring (&dt, "15/09/2001") == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "15/9/2001") == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "15/9/01") == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "15/9/1") == TM_OK);
  errno = 0;
  ck_assert (tm_setdatefromstring (&dt, "31/09/2001") == TM_ERROR && errno == EINVAL);
  errno = 0;
  ck_assert (tm_setdatefromstring (&dt, "15/13/2001") == TM_ERROR && errno == EINVAL);
  errno = 0;
  ck_assert (tm_setdatefromstring (&dt, "15/09/2001", TM_REF_LOCALTIME, 8) == TM_ERROR && errno == EINVAL);

  ck_assert (tm_settimefromstring (&dt, "09:51:07") == TM_OK);
  ck_assert (tm_settimefromstring (&dt, "9:51:07") == TM_OK);
  ck_assert (tm_settimefromstring (&dt, "9:51:7") == TM_OK);
  errno = 0;
  ck_assert (tm_settimefromstring (&dt, "24:51:07") == TM_ERROR && errno == EINVAL);
  errno = 0;
  ck_assert (tm_settimefromstring (&dt, "09:51:67") == TM_ERROR && errno == EINVAL);
  errno = 0;
  ck_assert (tm_settimefromstring (&dt, "15/09/2001") == TM_ERROR && errno == EINVAL);
  errno = 0;
  ck_assert (tm_settimefromstring (&dt, "09:51:07", TM_REF_LOCALTIME, 8) == TM_ERROR && errno == EINVAL);

  errno = 0;
  ck_assert ((tm_getdaysinmonth (2000, 13), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getsecondsinday (2000, 11, 5, TM_REF_UNCHANGED), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getsecondsinday (2000, 13, 5), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getfirstweekdayinmonth (2000, 13, TM_MONDAY), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getfirstweekdayinmonth (2000, TM_MAY, 8), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getlastweekdayinmonth (2000, 13, TM_MONDAY), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getlastweekdayinmonth (2000, TM_MAY, 8), 1) && errno == EINVAL);
  errno = 0;
  ck_assert ((tm_getfirstweekdayinisoyear (2000, 8), 1) && errno == EINVAL);

  struct tm max;
  ck_assert (tm_set (&max, 2147483646, 12, 31, 23, 59, 59, TM_REF_UTC) == TM_OK);
  tm_set (&dt, TM_NOW, TM_REF_UTC);
  for (int incr = 1000000; incr >= 1; incr /= 10)
  {
    errno = 0;
    for (struct tm d = dt; tm_addyears (&d, incr) == TM_OK; dt = d);
    ck_assert (errno == EOVERFLOW);
  }
  errno = 0;
  for (struct tm d = dt; tm_addmonths (&d, 1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_adddays (&d, 1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addhours (&d, 1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addminutes (&d, 1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addseconds (&d, 1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  ck_assert (tm_equals (dt, max));
  ck_assert (tm_changetowallclock (&dt, TM_REF_LOCALTIME) == TM_ERROR && errno == EINVAL);

  struct tm min;
  ck_assert (tm_set (&min, -2147481748, 1, 1, 0, 0, 0, TM_REF_LOCALTIME) == TM_OK);
  tm_set (&dt, TM_NOW, TM_REF_LOCALTIME);
  for (int incr = 1000000; incr >= 1; incr /= 10)
  {
    errno = 0;
    for (struct tm d = dt; tm_addyears (&d, -incr) == TM_OK; dt = d);
    ck_assert (errno == EOVERFLOW);
  }
  errno = 0;
  for (struct tm d = dt; tm_addmonths (&d, -1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_adddays (&d, -1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addhours (&d, -1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addminutes (&d, -1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  errno = 0;
  for (struct tm d = dt; tm_addseconds (&d, -1) == TM_OK; dt = d);
  ck_assert (errno == EOVERFLOW);
  ck_assert (tm_equals (dt, min));
  ck_assert (tm_changetowallclock (&dt, TM_REF_UTC) == TM_ERROR && errno == EINVAL);
}

END_TEST
START_TEST (tu_date)
{
  struct tm date;
  ck_assert (dt_set (&date) == TM_OK);
  ck_assert (dt_set (&date, TM_REF_UTC) == TM_OK);
  ck_assert (dt_set (&date, "America/Fortaleza") == TM_OK);
  ck_assert (dt_set (&date, "No/Where") == TM_ERROR);
  ck_assert (dt_set (&date, 2020, 2, 29) == TM_OK);
  ck_assert (dt_set (&date, 2020, 2, 30) == TM_ERROR);
  ck_assert (dt_setfromiso8601 (&date, "20210416") == TM_OK);
  ck_assert (dt_setfromiso8601 (&date, "2021-04-16") == TM_OK);
  ck_assert (dt_setfromstring (&date, "16/05/2023") == TM_OK);
  ck_assert (dt_setfromstring (&date, "2021-03-16") == TM_OK);
  ck_assert (dt_setfromstring (&date, "20210416") == TM_OK);
  ck_assert (dt_getyear (date) == 2021);
  ck_assert (dt_getmonth (date) == TM_APRIL);
  ck_assert (dt_getday (date) == 16);
  ck_assert (dt_getdayofyear (date) == 31 + 28 + 31 + 16);
  ck_assert (dt_getdayofweek (date) == TM_FRIDAY);
  ck_assert (dt_getisoweek (date) == 15);
  ck_assert (dt_getisoyear (date) == 2021);
  struct tm control = date;
  ck_assert (dt_equals (date, control));
  ck_assert (dt_adddays (&control, 30 + 3) == TM_OK);
  ck_assert (dt_addmonths (&control, 12 + 4) == TM_OK);
  ck_assert (dt_addyears (&control, 10) == TM_OK);
  ck_assert (dt_getyear (control) == 2032);
  ck_assert (dt_getmonth (control) == TM_SEPTEMBER);
  ck_assert (dt_getday (control) == 19);
  ck_assert (dt_tostring (control, 0, 0));
  ck_assert (dt_toiso8601 (control, 0, 0, 1));
  ck_assert (dt_toiso8601 (control, 0, 0, 0));
  ck_assert (dt_compare (&date, &control) < 0);
  ck_assert (dt_equals (date, control) == 0);
  ck_assert (dt_diffcalendaryears (date, control) == 11);
  ck_assert (dt_diffcalendarmonths (date, control) == 11 * 12 + 5);
  ck_assert (dt_diffcalendardays (date, control) == 11 * 365 + 3 /* leap years */  + 30 + 31 + 30 + 31 + 31 + 19 - 16);
  int months, days;
  ck_assert (dt_diffyears (date, control, &months, &days) == 11 && months == 5 && days == 3);
  ck_assert (dt_diffmonths (date, control, &days) == 11 * 12 + 5 && days == 3);
  ck_assert (dt_diffdays (date, control) == 11 * 365 + 3 /* leap years */  + 30 + 31 + 30 + 31 + 31 + 19 - 16);
  ck_assert (dt_diffweeks (date, control, &days) == 11 * 52 + 1 /* 2026 has 53 iso-weeks */  + 38 - 15 && days == 2);
  ck_assert (dt_diffisoyears (date, control) == 11);
  ck_assert (dt_frombinary (&control, dt_tobinary (date)) == TM_OK && dt_equals (date, control));
}

END_TEST
START_TEST (tu_iso8601)
{
  struct tm control, dt;
  char str[49];
  ck_assert (tm_set (&control, 2019, 8, 21, 0, 0, 0, TM_REF_LOCALTIME) == TM_OK &&
             tm_setfromiso8601 (&dt, "2019-08-21") == TM_OK && tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190821T000000+0200") == 0);
  ck_assert (tm_set (&control, 2019, 8, 22, 0, 0, 0, TM_REF_LOCALTIME) == TM_OK &&
             tm_setfromiso8601 (&dt, "2019−08−22") == TM_OK &&
             tm_equals (dt, control) && tm_toiso8601 (dt, 49, str, 1) == TM_OK && strcmp (str, "2019-08-22T00:00:00+0200") == 0);
  ck_assert (tm_set (&control, 2019, 8, 23, 0, 0, 0, TM_REF_LOCALTIME) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190823") == TM_OK && tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190823T000000+0200") == 0);
  ck_assert (tm_set (&control, 2019, 8, 24, 1, 2, 3, TM_REF_LOCALTIME) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190824T010203") == TM_OK && tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190824T010203+0200") == 0);
  ck_assert (tm_set (&control, 2019, 8, 25, 3, 32, 3, TM_REF_UTC) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190825T010203−0230") == TM_OK &&
             tm_equals (dt, control) && tm_toiso8601 (dt, 49, str, 1) == TM_OK && strcmp (str, "2019-08-25T03:32:03+0000") == 0);
  ck_assert (tm_set (&control, 2019, 8, 26, 9, 30, 0, TM_REF_UTC) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190826T07-0230") == TM_OK && tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190826T093000+0000") == 0);
  ck_assert (tm_set (&control, 2019, 8, 27, 1, 2, 3, TM_REF_LOCALTIME) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190827T010203.123456789+0200") == TM_OK &&
             tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190827T010203+0200") == 0);
  ck_assert (tm_set (&control, 2019, 8, 28, 1, 2, 3, TM_REF_UTC) == TM_OK &&
             tm_setfromiso8601 (&dt, "20190828T010203,000Z") == TM_OK &&
             tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190828T010203+0000") == 0);
}

END_TEST
START_TEST (tu_perf)
{
  ck_assert (tm_setlocalwallclock ("Europe/Dublin") == TM_OK);
  struct tm dt;
  ck_assert (tm_set (&dt) == TM_OK);
  for (size_t i = 200000; i; i--)
    tm_set (&dt);
}

END_TEST
START_TEST (tu_localwallclock)
{
  struct tm dt_system, dt_paris, dt_tokyo, dt_santiago, dt_antartica;

  ck_assert (tm_setlocalwallclock ("Europe/Paris") == TM_OK);
  ck_assert (!strcmp (tm_getlocalwallclock (), "Europe/Paris"));
  tm_set (&dt_paris, 2021, 7, 5, 23, 45, 02);
  ck_assert (tm_getdayofweek (dt_paris) == TM_MONDAY);
  ck_assert (tm_isdefinedinlocaltime (dt_paris));

  dt_tokyo = dt_paris;
  ck_assert (tm_changetowallclock (&dt_tokyo, "Asia/Tokyo") == TM_OK);
  ck_assert (tm_getdayofweek (dt_tokyo) == TM_TUESDAY);
  ck_assert (tm_getday (dt_tokyo) == 6);
  ck_assert (tm_isdefinedinwallclock (dt_tokyo, "Asia/Tokyo"));
  ck_assert (tm_diffseconds (dt_paris, dt_tokyo) == 0);

  ck_assert (tm_setlocalwallclock ("America/Santiago") == TM_OK);
  ck_assert (tm_islocalwallclock ("America/Santiago"));
  tm_set (&dt_santiago, 2021, 7, 5, 17, 45, 02);
  ck_assert (tm_isdefinedinlocaltime (dt_santiago));
  ck_assert (tm_isdefinedinwallclock (dt_paris, "Europe/Paris"));
  ck_assert (tm_diffseconds (dt_paris, dt_santiago) == 0);

  tm_setlocalwallclock (TM_REF_SYSTEMTIME);
  tm_set (&dt_system);
  ck_assert (tm_isdefinedinsystemtime (dt_system));
  ck_assert (tm_isdefinedinlocaltime (dt_system));
  tm_set (&dt_antartica, 2021, 7, 5, 23, 45, 02, "Antarctica/Davis");
  ck_assert (tm_isdefinedinwallclock (dt_antartica, "Antarctica/Davis"));
  ck_assert (tm_diffhours (dt_paris, dt_antartica) == -5);
}

END_TEST
START_TEST (tu_coverage)
{
  struct tm dt;
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "No/Where") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "No/Where") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "Europe/Paris") == TM_OK);
  ck_assert (tm_changetowallclock (&dt, TM_REF_UTC) == TM_OK);
  ck_assert (tm_changetowallclock (&dt, TM_REF_UTC) == TM_OK);
  ck_assert (tm_set (&dt, TM_TODAY, "No/Where") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "No/Where") == TM_ERROR);
  ck_assert (tm_changetowallclock (&dt, "No/Where") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "No/Where") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "6th_continent") == TM_ERROR);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, "Atlantic/Cape_Verde") == TM_OK);
  ck_assert (tm_isdefinedinwallclock (dt, "Atlantic/Cape_Verde"));
  ck_assert (!tm_isdefinedinwallclock (dt, "6th_continent"));
  ck_assert (!tm_isdefinedinwallclock (dt, "Europe/Paris"));
  char toolong[300] = { 0 };
  for (size_t i = 0; i < sizeof (toolong) / sizeof (*toolong) - 1; i++)
    toolong[i] = 'a';
  errno = 0;
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02, toolong) == TM_OK && errno == ENOMEM);
  ck_assert (tm_setlocalwallclock (TM_REF_LOCALTIME) == TM_OK);
  ck_assert (tm_setlocalwallclock ("6th_continent") == TM_ERROR);
  ck_assert (tm_setlocalwallclock (TM_REF_UTC) == TM_OK);
  ck_assert (tm_getlocalwallclock () == TM_REF_UTC);
  ck_assert (tm_setlocalwallclock (0) == TM_OK);
  ck_assert (tm_set (&dt, TM_TODAY, TM_REF_UNCHANGED) == TM_OK);
  ck_assert (tm_set (&dt, -1) == TM_ERROR);
  int max = INT_MAX;
  ck_assert (tm_set (&dt, max + 1, 7, 5, 23, 45, 02) == TM_ERROR);
  ck_assert (tm_set (&dt, 2001, 7, 5, 23, 45, 02, TM_REF_LOCALTIME, -1) == TM_ERROR);
  ck_assert (tm_setdatefromstring (&dt, "15/09/2001") == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "15/09/11") == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "1999-02-25", "rfhzh") == TM_ERROR);
  ck_assert (tm_setdatefromstring (&dt, "1999-02-25", TM_REF_LOCALTIME, TM_DST_OVER_ST) == TM_OK);
  ck_assert (tm_setdatefromstring (&dt, "1999-02-25", TM_REF_LOCALTIME, -1) == TM_ERROR);
  ck_assert (tm_settimefromstring (&dt, "00:00:00") == TM_OK);
  ck_assert (tm_settimefromstring (&dt, "00:00:00", "rfhzh") == TM_ERROR);
  ck_assert (tm_settimefromstring (&dt, "00:00:00", TM_REF_LOCALTIME, TM_DST_OVER_ST) == TM_OK);
  ck_assert (tm_settimefromstring (&dt, "00:00:00", TM_REF_LOCALTIME, -1) == TM_ERROR);
  ck_assert (tm_settimefromstring (&dt, "00:00:60") == TM_ERROR);
  ck_assert (tm_setfromiso8601 (&dt, "abc") == TM_ERROR);
  ck_assert (tm_setfromiso8601 (&dt, "20010823T24:00:00") == TM_OK);
  ck_assert (tm_setfromiso8601 (&dt, "20010823T04:18:60") == TM_OK);
  ck_assert (tm_setfromiso8601 (&dt, "20010823T04:18:60+02:00") == TM_OK);
  ck_assert (tm_setfromiso8601 (&dt, "20010823T04:18:60+04:00") == TM_OK);
  ck_assert (tm_setfromiso8601 (&dt, "20010229T04:18:20") == TM_ERROR);
  ck_assert (dt_setfromiso8601 (&dt, "20010229") == TM_ERROR);
  ck_assert (tm_getdaysinmonth (2021, 15) == 0);
  ck_assert (tm_getsecondsinday (2021, 15, 35) == 0);
  ck_assert (tm_getsecondsinday (2021, 11, 15, TM_REF_UNCHANGED) == 0);
  ck_assert (tm_getfirstweekdayinmonth (2020, 2, 8) == 0);
  ck_assert (tm_getfirstweekdayinmonth (2020, 22, 2) == 0);
  ck_assert (tm_getlastweekdayinmonth (2020, 2, 8) == 0);
  ck_assert (tm_getlastweekdayinmonth (2020, 22, 2) == 0);
  ck_assert (tm_getfirstweekdayinisoyear (2020, 8) == 0);
  ck_assert (tm_set (&dt, 2021, 7, 5, 23, 45, 02) == TM_OK);
  ck_assert (tm_addseconds (&dt, LONG_MAX) == TM_ERROR && errno == EOVERFLOW);
  ck_assert (tm_adddays (&dt, INT_MAX) == TM_ERROR && errno == EOVERFLOW);
  ck_assert (tm_addmonths (&dt, INT_MAX) == TM_ERROR && errno == EOVERFLOW);
  ck_assert (tm_changetowallclock (&dt, "No/Where") == TM_ERROR);
  ck_assert (tm_frombinary (&dt, 0, "No/Where") == TM_ERROR);

  struct tm debut, fin;
  ck_assert (tm_set (&debut, 2000, 12, 12, 10, 10, 10) == TM_OK);
  ck_assert (tm_set (&fin, 2000, 12, 12, 10, 10, 10) == TM_OK && tm_addmonths (&fin, 2) == TM_OK && tm_addseconds (&fin, -1) == TM_OK);
  ck_assert (tm_diffmonths (debut, fin) == 1);
  ck_assert (tm_set (&fin, 2000, 12, 12, 0, 0, 0) == TM_OK && tm_addyears (&fin, -1) == TM_OK && tm_adddays (&fin, -1) == TM_OK);
  ck_assert (tm_diffcalendardays (debut, fin) == -367);
  ck_assert (tm_diffdays (debut, fin) == -367);
  ck_assert (tm_diffmonths (debut, fin) == -12);
  ck_assert (tm_changetowallclock (&fin, TM_REF_UTC) == TM_OK);
  ck_assert (tm_diffcalendardays (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_diffdays (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_diffyears (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_diffcalendaryears (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_diffcalendarmonths (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_diffisoyears (debut, fin) == 0 && errno == EINVAL);
  ck_assert (tm_frombinary (&dt, 0, TM_REF_UNCHANGED) == TM_OK);
  ck_assert (tm_getdaysinyear (INT_MAX) == 0 && errno == EINVAL);
  ck_assert (tm_getweeksinisoyear (INT_MAX) == 0 && errno == EINVAL);
  ck_assert (tm_getweeksinisoyear (INT_MAX - 1) == 0 && errno == EINVAL);
  ck_assert (tm_getdaysinmonth (2147483646, 12) == 0 && errno == EINVAL);
  ck_assert (tm_getsecondsinday (2147483646, 12, 31) == 0 && errno == EINVAL);
}

END_TEST
/**************** SEQUENCEMENT DES TESTS ***************/
static Suite *
mm_suite (int is_TZ_owner)
{
  char *case_name;
  SFun my_unckecked_setup;
  if (!is_TZ_owner)
  {
    case_name = "Tests";
    my_unckecked_setup = unckecked_setup_is_not_TZ_owner;
  }
  else
  {
    case_name = "Tests optimized";
    my_unckecked_setup = unckecked_setup_is_TZ_owner;
  }

  Suite *s = suite_create ("Dates toolkit");
  TCase *tc = tcase_create (case_name);
  // Unchecked fixture : le code setup n'est exécuté qu'une seule fois avant tous les cas de tests.
  tcase_add_unchecked_fixture (tc, my_unckecked_setup, unckecked_teardown);
  tcase_add_checked_fixture (tc, ckecked_setup, ckecked_teardown);

  tcase_add_test (tc, tu_moon_walk);
  tcase_add_test (tc, tu_now);
  tcase_add_test (tc, tu_today);
  tcase_add_test (tc, tu_today_utc);
  tcase_add_test (tc, tu_local);
  tcase_add_test (tc, tu_utc);
  tcase_add_test (tc, tu_set_from_local);
  tcase_add_test (tc, tu_set_from_utc);
  tcase_add_test (tc, tu_tostring_local);
  tcase_add_test (tc, tu_tostring_utc);
  tcase_add_test (tc, tu_iso8601);
  tcase_add_test (tc, tu_getters_local);
  tcase_add_test (tc, tu_getters_utc);
  tcase_add_test (tc, tu_ops_local);
  tcase_add_test (tc, tu_ops_utc);
  tcase_add_test (tc, tu_diff_local);
  tcase_add_test (tc, tu_diff_utc);
  tcase_add_test (tc, tu_dst);
  tcase_add_test (tc, tu_dst_winter);
  tcase_add_test (tc, tu_dst_summer);
  tcase_add_test (tc, tu_iso);
  tcase_add_test (tc, tu_calendar);
  tcase_add_test (tc, tu_equality);
  tcase_add_test (tc, tu_change_timezone);
  tcase_add_test (tc, tu_serialization);
  tcase_add_test (tc, tu_day_loop);
  tcase_add_test (tc, tu_beginingoftheday);
  tcase_add_test (tc, tu_weekday);
  tcase_add_test (tc, tu_date);
  tcase_add_test (tc, tu_localwallclock);
  tcase_add_test (tc, tu_perf);
  tcase_add_test (tc, tu_error);
  tcase_add_test (tc, tu_coverage);

  suite_add_tcase (s, tc);

  return s;
}

/**************** EXECUTION DES TESTS ******************/

int
main (void)
{
  // Required localization for designed unit tests:
  SRunner *sr = srunner_create (0);
  Suite *s;
  s = mm_suite (0);
  srunner_add_suite (sr, s);
  s = mm_suite (1);
  srunner_add_suite (sr, s);

  srunner_run_all (sr, CK_ENV);
  int number_failed = srunner_ntests_failed (sr);

  srunner_free (sr);

  return number_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
