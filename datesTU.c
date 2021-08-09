#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <assert.h>

#include "dates.h"

/****************************************************/

static void
tm_print (struct tm date)
{
  char buf[1000];

  strftime (buf, sizeof (buf), "*%x %X\n" "*%c\n" "*%G-W%V-%u\n" "*%Y-D%j", &date);

  printf ("----\n"
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
          " tm_zone: %s (%p)\n"
          "%04i-W%02i-%i\n"
          "%04i-D%03i%+is\n"
          "%s\n"
          "%s----\n",
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
          date.tm_zone ? date.tm_zone : "(null)", (void *) date.tm_zone,
          tm_getisoyear (date), tm_getisoweek (date), tm_getdayofweek (date),
          tm_getyear (date), tm_getdayofyear (date), tm_getsecondsofday (date), buf, asctime (&date));
}

static void
test_date (const char *buf)
{
  struct tm start;
  tm_set (&start, TM_TODAY);

  if (tm_setdatefromstring (&start, buf) == TM_OK)
  {
    tm_print (start);
    struct tm stop = start;
    tm_adddays (&stop, 1);
    for (struct tm dt = start; tm_compare (&dt, &stop) < 0; tm_addseconds (&dt, 3600))
    {
      fprintf (stdout, "%02i-", tm_gethour (dt));
      int dc = tm_isinsidedaylightsavingtimeoverlap (dt);
      int dst = tm_isdaylightsavingtimeineffect (dt);
      fprintf (stdout, "%02i%s;", tm_gethour (dt) + 1, dc ? (dst ? "A" : "B") : "");
    }
    fprintf (stdout, "\n");
  }
  else
    puts ("Incorrect");
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "fr_FR.utf8");

  time_t tnow;

  time (&tnow);

  tzset ();
  struct tm now;

  localtime_r (&tnow, &now);

  char s_now[200];
  tm_tostring (now, sizeof (s_now) / sizeof (*s_now), s_now);
  puts (s_now);

  struct tm moon_walk;
  assert (tm_set (&moon_walk, TM_TODAY, "Nowhere") == TM_ERROR);
  // Armstrong became the first person to step onto the lunar surface on July 21 at 02:56 UTC
  assert (tm_set (&moon_walk, 1969, TM_JULY, 21, 2, 56, 0, TM_REF_UTC) == TM_OK);
  tm_changetowallclock (&moon_walk, TM_REF_LOCALTIME);
  assert (tm_gethour (moon_walk) == 3); // in Paris

  struct tm control, dt;
  char str[49];
  assert (tm_set (&control, 2019, 8, 27, 1, 2, 3, TM_REF_LOCALTIME) == TM_OK &&
          tm_setfromiso8601 (&dt, "20190827T010203.123456789+0200") == TM_OK &&
          tm_equals (dt, control) && tm_toiso8601 (dt, 49, str) == TM_OK && strcmp (str, "20190827T010203+0200") == 0);

  struct tm aDate, bDate;

  tm_set (&aDate, 2002, 10, 27, 2, 40, 10, TM_REF_LOCALTIME, TM_DST_OVER_ST);
  tm_print (aDate);
  tm_set (&aDate, 2002, 10, 27, 2, 40, 10, TM_REF_LOCALTIME, TM_ST_OVER_DST);
  tm_print (aDate);

  tm_set (&aDate, 2002, 10, 27, 1, 30, 0);
  tm_print (aDate);
  tm_addseconds (&aDate, 3600);
  tm_print (aDate);
  tm_addseconds (&aDate, 3600);
  tm_print (aDate);
  tm_addseconds (&aDate, 3600);
  tm_print (aDate);
  tm_addseconds (&aDate, -3600);
  tm_print (aDate);
  tm_addseconds (&aDate, -3600);
  tm_print (aDate);
  tm_addseconds (&aDate, -3600);
  tm_print (aDate);

  tm_set (&aDate, 2010, 6, 30, 10, 0, 0);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);
  tm_adddays (&aDate, 180);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);
  tm_adddays (&aDate, 200);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);

  tm_set (&aDate, 2010, 8, 21, 10, 0, 0, TM_REF_UTC);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);
  tm_adddays (&aDate, 180);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);
  tm_adddays (&aDate, 200);
  tm_print (aDate);
  tm_adddays (&aDate, 0);
  tm_print (aDate);

  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);
  tm_addmonths (&aDate, 6);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);
  tm_addmonths (&aDate, 6);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);

  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);
  tm_addmonths (&aDate, 6);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);
  tm_addmonths (&aDate, 6);
  tm_print (aDate);
  tm_addmonths (&aDate, 0);
  tm_print (aDate);

  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);

  bDate = aDate;
  tm_addmonths (&bDate, 6);
  tm_adddays (&bDate, 27);
  tm_addseconds (&bDate, 3600 * 18);
  tm_print (aDate);
  tm_print (bDate);
  printf ("%+i jours\n", tm_diffcalendardays (aDate, bDate));
  printf ("%+i jours\n", tm_diffcalendardays (bDate, aDate));
  printf ("%+i jours\n", tm_diffdays (aDate, bDate, 0));
  printf ("%+i jours\n", tm_diffdays (bDate, aDate, 0));
  printf ("%+i semaines\n", tm_diffweeks (aDate, bDate, 0, 0));
  printf ("%+i semaines\n", tm_diffweeks (bDate, aDate, 0, 0));
  printf ("%+i mois\n", tm_diffcalendarmonths (aDate, bDate));
  printf ("%+i mois\n", tm_diffcalendarmonths (bDate, aDate));
  printf ("%+i mois\n", tm_diffmonths (aDate, bDate, 0, 0));
  printf ("%+i mois\n", tm_diffmonths (bDate, aDate, 0, 0));
  printf ("%+li secondes\n", tm_diffseconds (aDate, bDate));
  printf ("%+li secondes\n", tm_diffseconds (bDate, aDate));

  tm_addmonths (&aDate, 6);
  tm_addmonths (&bDate, 6);
  tm_adddays (&bDate, 32);
  tm_addseconds (&bDate, 3600 * 18);
  tm_print (aDate);
  tm_print (bDate);
  printf ("%+i jours\n", tm_diffcalendardays (aDate, bDate));
  printf ("%+i jours\n", tm_diffcalendardays (bDate, aDate));
  printf ("%+i jours\n", tm_diffdays (aDate, bDate));
  printf ("%+i jours\n", tm_diffdays (bDate, aDate));
  printf ("%+i semaines\n", tm_diffweeks (aDate, bDate));
  printf ("%+i semaines\n", tm_diffweeks (bDate, aDate));
  printf ("%+i mois\n", tm_diffcalendarmonths (aDate, bDate));
  printf ("%+i mois\n", tm_diffcalendarmonths (bDate, aDate));
  printf ("%+i mois\n", tm_diffmonths (aDate, bDate));
  printf ("%+i mois\n", tm_diffmonths (bDate, aDate));
  printf ("%+li secondes\n", tm_diffseconds (aDate, bDate));
  printf ("%+li secondes\n", tm_diffseconds (bDate, aDate));

  tm_set (&aDate, 2003, 12, 28, 10, 0, 0);
  tm_print (aDate);
  tm_set (&aDate, 2003, 12, 29, 10, 0, 0);
  tm_print (aDate);
  tm_set (&aDate, 2005, 1, 2, 10, 0, 0);
  tm_print (aDate);
  tm_set (&aDate, 2005, 1, 3, 10, 0, 0);
  tm_print (aDate);

  tm_set (&aDate, 2003, 12, 28, 12, 0, 0);
  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);
  tm_set (&aDate, 2003, 6, 28, 12, 0, 0);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);

  if (tm_setdatefromstring (&aDate, "2002-02-01") == TM_OK)
    tm_print (aDate);
  if (tm_setdatefromstring (&aDate, "3/4/3") == TM_OK)
    tm_print (aDate);
  if (tm_setdatefromstring (&aDate, "4/5/04") == TM_OK)
    tm_print (aDate);
  if (tm_settimefromstring (&aDate, "13:02:45") == TM_OK)
    tm_print (aDate);

  tm_changetowallclock (&aDate, TM_REF_UTC);

  if (tm_setdatefromstring (&aDate, "05/06/05") == TM_OK)
    tm_print (aDate);
  if (tm_settimefromstring (&aDate, "11:32:05") == TM_OK)
    tm_print (aDate);
  if (tm_settimefromstring (&aDate, "05/06/2006") == TM_OK)     // Invalid input
    tm_print (aDate);
  if (tm_settimefromstring (&aDate, "11:35") == TM_OK)
    tm_print (aDate);

  tm_set (&aDate, TM_TODAY);
  tm_print (aDate);
  tm_set (&aDate);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_UTC);
  tm_print (aDate);
  tm_changetowallclock (&aDate, TM_REF_LOCALTIME);

  test_date ("2018-10-28");
  test_date ("2019-03-31");

  if (tm_setfromiso8601 (&aDate, "20190827T010203.012345678+0200") == TM_OK)
    tm_print (aDate);

  static char buf[255];
  while (fscanf (stdin, "%254s", buf) != EOF && *buf)
    test_date (buf);

  return EXIT_SUCCESS;
}
