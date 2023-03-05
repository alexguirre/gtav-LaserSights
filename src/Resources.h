#pragma once
#define LASERSIGHTS_VERSION_XSTR(s) #s
#define LASERSIGHTS_VERSION_STR(s) LASERSIGHTS_VERSION_XSTR(s)

#define LASERSIGHTS_VERSION_MAJOR 0
#define LASERSIGHTS_VERSION_MINOR 1
#define LASERSIGHTS_VERSION_PATCH 0
#define LASERSIGHTS_VERSION_STRING LASERSIGHTS_VERSION_STR(LASERSIGHTS_VERSION_MAJOR) "." \
                                   LASERSIGHTS_VERSION_STR(LASERSIGHTS_VERSION_MINOR) "." \
                                   LASERSIGHTS_VERSION_STR(LASERSIGHTS_VERSION_PATCH)

#define LASERSIGHTS_FILENAME "LaserSights"

#define LASERSIGHTS_RES_ID_LASER_NOISE_DDS  101
#define LASERSIGHTS_RES_ID_LASERBEAM_FXC    102
#define LASERSIGHTS_RES_ID_LASERBEAM_LQ_FXC 103
