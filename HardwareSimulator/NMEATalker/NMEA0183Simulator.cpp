/*! \file NMEA0183Simulator.cpp
 * \brief Generate faked NMEA0183-formated data on the software serial output line
 *
 * This generates example NMEA0183 data for a GNSS with depth sounder on a single line,
 * changing the position and depth as a function of time.  The position information is
 * passed out at 1Hz, and the depth data at 0.5-1.0Hz (randomly).  The position and depth
 * are changed over time, with suitable constraints to maintain consistency.
 *
 * Copyright (c) University of New Hampshire, Center for Coastal and Ocean Mapping &
 * NOAA-UNH Joint Hydrographic Center, 2019.  All Rights Reserved.
 *
 */

#include <Arduino.h>
#include "NMEA0183Simulator.h"

bool iset = false;
float gset;
 
unsigned long target_depth_time = 0;
unsigned long target_position_time = 0;
unsigned long target_zda_time = 0;
//unsigned long last_position_time = 0;
unsigned long last_zda_time = 0;

double current_depth = 10.0;            /* Depth in metres */
double measurement_uncertainty = 0.06;  /* Depth sounder measurement uncertainty, std. dev. */
double depth_random_walk = 0.02;        /* Standard deviation, metres change in one second */

int current_year = 2020;        /* Current year for the simulation */
int current_day_of_year = 0;    /* Current day of the year (a.k.a. Julian Day) for the simulation */
int current_hours = 0;        /* Current time of day, hours past midnight */
int current_minutes = 0;      /* Current time of day, minutes past the hour */
double current_seconds = 0.0; /* Current time of day, seconds past the minute */

double current_longitude = -75.0;       /* Longitude in degrees */
double current_latitude = 43.0;         /* Latitude in degress */

double position_step = 3.2708e-06;      /* Change in latitude/longitude per second */
double latitude_scale = +1.0;           /* Current direction of latitude change */
double last_latitude_reversal = 0.0;    /* Last time the latitude changed direction */

double unit_normal(void)
{
    float fac, rsq, v1, v2;
    float u, v, r;

    if (!iset) {
        do {
            u = random(1000)/1000.0;
            v = random(1000)/1000.0;
            v1 = 2.0*u - 1.0;
            v2 = 2.0*v - 1.0;
            rsq = v1*v1 + v2*v2;
        } while (rsq >= 1.0 || rsq == 0.0);
        fac = sqrt(-2.0*log(rsq)/rsq);
        gset = v1*fac;
        iset = true;
        r = v2*fac;
    } else {
        iset = false;
        r = gset;
    }
    return r;
}

int compute_checksum(const char *msg)
{
    int chk = 0;
    int len = (int)strlen(msg);
    for (int i = 1; i < len-1; ++i) {
        chk ^= msg[i];
    }
    return chk;
}

void GenerateDepth(unsigned long now, StatusLED *led)
{
    if (now < target_depth_time) return;
    
    current_depth += depth_random_walk*unit_normal();
    
    double depth_metres = current_depth + measurement_uncertainty*unit_normal();
    double depth_feet = depth_metres * 3.2808;
    double depth_fathoms = depth_metres * 0.5468;

    char msg[255];
    int pos = sprintf(msg, "$SDDBT,%.1lf,f,%.1lf,M,%.1lf,F*", depth_feet, depth_metres, depth_fathoms);
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);

    String txt = "Sending SDDBT: ";
    Serial.print(txt + msg);
    Serial1.print(msg);
    led->TriggerDataIndication();
    
    target_depth_time = now + 1000 + (int)(1000*(random(1000)/1000.0));
}

void format_angle(double angle, int *d, double *m, int *hemi)
{
    if (angle > 0.0) {
        *hemi = 1;
    } else {
        *hemi = 0;
        angle = -angle;
    }

    *d = (int)angle;
    *m = angle - *d;
}

void GeneratePosition(unsigned long now, StatusLED *led)
{
    if (now < target_position_time) return;
    
    current_latitude += latitude_scale * position_step;
    current_longitude += 1.0 * position_step;

    if (now - last_latitude_reversal > 3600000.0) {
        latitude_scale = -latitude_scale;
        last_latitude_reversal = now;
    }

    char msg[255];
    int pos = sprintf(msg, "$GPGGA,%02d%02d%06.3f,", current_hours, current_minutes, current_seconds);
    int degrees;
    double minutes;
    int hemisphere;
    format_angle(current_latitude, &degrees, &minutes, &hemisphere);
    pos += sprintf(msg + pos, "%02d%09.6lf,%c,", degrees, minutes, hemisphere == 1 ? 'N' : 'S');
    format_angle(current_longitude, &degrees, &minutes, &hemisphere);
    pos += sprintf(msg + pos, "%03d%09.6lf,%c,", degrees, minutes, hemisphere == 1 ? 'E' : 'W');
    pos += sprintf(msg + pos, "3,12,1.0,-19.5,M,22.5,M,0.0,0000*");
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);

    String txt = "Sending GGA: ";
    Serial.print(txt + msg);
    Serial2.print(msg);
    led->TriggerDataIndication();
    
    target_position_time = now + 1000;
}

void ToDayMonth(int year, int year_day, int& month, int& day)
{
    unsigned     leap;
    static unsigned months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    
    /* Determine whether this is a leap year.  Leap years in the Gregorian
     * calendar are years divisible by four, unless they are also a century
     * year, except when the century is also divisible by 400.  Thus 1900 is
     * not a leap year (although it is divisible by 4), but 2000 is, because
     * it is divisible by 4 and 400).
     */
    if ((year%4) == 0) {
            /* Potentially a leap year, check for century year */
            if ((year%100) == 0) {
                    /* Century year, check for 400 years */
                    if ((year%400) == 0)
                            leap = 1;
                    else
                            leap = 0;
            } else
                    leap = 1;
    } else
            leap = 0;
    day = year_day + 1; // External is [0, 364], but we assume here [1, 365]
    
    months[1] += leap;      /* Correct February */
    
    /* Compute month by reducing days until we have less than the next months
     * total number of days.
     */
    month = 0;
    while (day > months[month]) {
            day -= months[month];
            ++month;
    }
    ++month; // External is [1, 12] but here it's [0, 11]
    
    months[1] -= leap;      /* Uncorrect February */
}

void GenerateZDA(unsigned long now, StatusLED *led)
{
    if (now < target_zda_time) return;
    
    unsigned long delta = now - last_zda_time;
    current_seconds += delta/1000.0;
    if (current_seconds >= 60.0) {
        current_minutes++;
        current_seconds -= 60.0;
        if (current_minutes >= 60) {
            current_hours++;
            current_minutes = 0;
            if (current_hours >= 24) {
                current_day_of_year++;
                current_hours = 0;
                if (current_day_of_year > 365) {
                    current_year++;
                    current_day_of_year = 0;
                }
            }
        }
    }
    
    char msg[255];
    int day, month;
    
    // We track day-of-year (a.k.a. Julian Day), so we need to convert to day/month for output
    ToDayMonth(current_year, current_day_of_year, month, day);
    int pos = sprintf(msg, "$GPZDA,%02d%02d%06.3lf,%02d,%02d,%04d,00,00*",
                      current_hours, current_minutes, current_seconds,
                      day, month, current_year);
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);

    String txt = "Sending ZDA: ";
    Serial.print(txt + msg);
    last_zda_time = millis();
    Serial2.print(msg);
    led->TriggerDataIndication();
    
    target_zda_time = now + 1000;
}
