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
unsigned long last_position_time = 0;

double current_depth = 10.0;            /* Depth in metres */
double measurement_uncertainty = 0.06;  /* Depth sounder measurement uncertainty, std. dev. */
double depth_random_walk = 0.02;        /* Standard deviation, metres change in one second */

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
    int len = strlen(msg);
    for (int i = 1; i < len-1; ++i) {
        chk ^= msg[i];
    }
    return chk;
}

void GenerateDepth(unsigned long now)
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

void GeneratePosition(unsigned long now)
{
    if (now < target_position_time) return;
    
    current_latitude += latitude_scale * position_step;
    current_longitude += 1.0 * position_step;

    if (now - last_latitude_reversal > 3600000.0) {
        latitude_scale = -latitude_scale;
        last_latitude_reversal = now;
    }

    unsigned int delta = now - last_position_time;
    current_seconds += delta/1000.0;
    if (current_seconds >= 60.0) {
        current_minutes++;
        current_seconds -= 60.0;
        if (current_minutes >= 60) {
            current_hours++;
            current_minutes = 0;
            if (current_hours >= 24)
                current_hours = 0;
        }
    }

    char msg[255];
    int pos = sprintf(msg, "$GPGGA,%02d%02d%06.3f,", current_hours, current_minutes, current_seconds);
    int degrees;
    double minutes;
    int hemisphere;
    format_angle(current_latitude, &degrees, &minutes, &hemisphere);
    pos += sprintf(msg + pos, "%02d%09.6lf%c,", degrees, minutes, hemisphere == 1 ? 'N' : 'S');
    format_angle(current_longitude, &degrees, &minutes, &hemisphere);
    pos += sprintf(msg + pos, "%03d%09.6lf%c,", degrees, minutes, hemisphere == 1 ? 'E' : 'W');
    pos += sprintf(msg + pos, "3,12,1.0,-19.5,M,22.5,M,0.0,0000*");
    int chksum = compute_checksum(msg);
    sprintf(msg + pos, "%02X\r\n", chksum);

    String txt = "Sending GGA: ";
    Serial.print(txt + msg);
    last_position_time = millis();
    Serial2.print(msg);
    
    target_position_time = now + 1000;
}
