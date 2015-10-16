/** @file
 *  Provides implementation for initialization and other management functions 
 *  for GPS NMEA parser.
 *
 */

/** @addtogroup nmea_parser NMEA0183 Parser
 *  @{
 */


/*******************************************************************************
*                          Include Files
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "nmea0183_parser.h"

/*******************************************************************************
*                          Extern Data Declarations
*******************************************************************************/

/*******************************************************************************
*                          Extern Function Declarations
*******************************************************************************/

/*******************************************************************************
*                          Type & Macro Definitions
*******************************************************************************/
#define RMC_HEADER					"$GPRMC,"
#define ASSERT_RMC(t, m, b)			do {if (!(t)) { printf(m); goto b; } } while(0);

/*******************************************************************************
*                          Static Function Prototypes
*******************************************************************************/

/*******************************************************************************
*                          Static Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern/Exported Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern/Exported  Function Definitions
*******************************************************************************/

/**
********************************************************************************
* GPS gives information about year, date, time along with longitude and latitude
* @param  data: Pointer to nmea RMC data structure
* @param  buf: Pointer to the buffer
* @param  buf_size: Buffer Size 
* @return Result of parsing operatoin 
********************************************************************************/
/* Format

  $GPRMC,102642.03,A,4813.7943164,N,01621.5693035,E,7.158,156.6705,020713,020.32,E*5E
     |   |         | |              |               |     |        |      |        |
     |   |         | |              |               |     |        |      |      *5E Checksum data
     |   |         | |              |               |     |        |      |        
     |   |         | |              |               |     |        |      |
     |   |         | |              |               |     |        |      020.32,E Magnetic Variation - 20 deg 32 off true north
     |   |         | |              |               |     |        |
     |   |         | |              |               |     |        020713 Date - February 7, 2013
     |   |         | |              |               |     |
     |   |         | |              |               |     156.6705 Heading
     |   |         | |              |               |     
     |   |         | |              |               7.158 Ground speed in knots
     |   |         | |              |
     |   |         | |              01621.5693035,E Longitude 16 deg 21.5693035' E
     |   |         | |
     |   |         | 4813.7943146, N Latitude 48 deg 13.7943146' N
     |   |         |
     |   |         A Status A=active or V=Void
     |   |
     |   102642.03 Time of fix10:26:42 UTC
     |
     $GPRMC Signature + "Talker" ID + Sentence ID
     ********************************************************************************/
     
rmc_parse_result parse_rmc(nmea_rmc_data_t *data, const char *buf, int buf_size)
{
    char aux_buf[15];
    const char *p1, *p2;
	char checksum;
    
	// unused parameter
	(void)buf_size;
	
    // Verify $GPRMC
    ASSERT_RMC(!strncmp(buf, RMC_HEADER, strlen(RMC_HEADER)), "Not a RMC sentence\n", parse_rmc_bailout);

	// Verify checksum first
    p1 = buf + 1;	// skip '$' sign
    ASSERT_RMC(p2 = strchr(p1, '*'), "No checksum\n", parse_rmc_bailout);
    ASSERT_RMC(*(p2 + 3) == 0, "Inproper ending\n", parse_rmc_bailout);
	checksum = strtol(p2 + 1, NULL, 16);
	while (p1 != p2) {
		checksum ^= *p1++;
	}
    ASSERT_RMC(checksum == 0, "Wrong checksum\n", parse_rmc_bailout);

    // Time
    p1 = buf + strlen(RMC_HEADER);
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid time\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->hour = atoi(aux_buf);
    p1 += 2;
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->min = atoi(aux_buf);
    p1 += 2;
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->sec = atoi(aux_buf);
	p1 += 2;
    ASSERT_RMC(*p1 == '.', "Invalid time\n", parse_rmc_bailout);
	// ignore milliseconds
    p1 = p2 + 1;

    // Status 
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid status\n", parse_rmc_bailout);
    data->status = *p1;
    if (data->status == 'V') {
		// no valid fix, stop here
        return RMC_PARSE_SUCCESSFUL_WITH_NO_FIX;
	}
    ASSERT_RMC(data->status == 'A', "Invalid status\n", parse_rmc_bailout);
    p1 = p2 + 1;

    // Latitude
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid latitude\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, p2 - p1);
    aux_buf[p2 - p1] = '\0';
    p1 = p2 + 1;
    ASSERT_RMC(p2 = strchr(aux_buf, '.'), "Invalid latitude\n", parse_rmc_bailout);
    ASSERT_RMC(p2 - aux_buf >= 2, "Invalid latitude\n", parse_rmc_bailout);
    data->latitude = atof(p2 - 2) / 60.0;
    aux_buf[p2 - 2 - aux_buf] = '\0';
    data->latitude += atof(aux_buf);
	// Direction
    ASSERT_RMC(*(p1 + 1) == ',', "Invalid latitude\n", parse_rmc_bailout);
    if (*p1 == 'S') {
         data->latitude = -data->latitude;
	} else {
		ASSERT_RMC(*p1 == 'N', "Invalid latitude\n", parse_rmc_bailout);
	}
    p1 += 2;

    // Longitude
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid longitude\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, p2 - p1);
    aux_buf[p2 - p1] = '\0';
    p1 = p2 + 1;
    ASSERT_RMC(p2 = strchr(aux_buf, '.'), "Invalid longitude", parse_rmc_bailout);
    ASSERT_RMC(p2 - aux_buf >= 2, "Invalid longitude\n", parse_rmc_bailout);
    data->longitude = atof(p2 - 2) / 60.0;
    aux_buf[p2 - 2 - aux_buf] = '\0';
    data->longitude += atof((char *)aux_buf);
	// Direction
    ASSERT_RMC(*(p1 + 1) == ',', "Invalid longitude\n", parse_rmc_bailout);
    if (*p1 == 'W') {
         data->longitude = -data->longitude;
    } else {
		ASSERT_RMC(*p1 == 'E', "Invalid longitude\n", parse_rmc_bailout);
	}
    p1 += 2;

    // Ground speed
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid speed\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, p2 - p1);
    aux_buf[p2 - p1] = '\0';
    p1 = p2 + 1;
    data->ground_speed = atof(aux_buf);

    // heading (degrees) 
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid heading\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, p2 - p1);
    aux_buf[p2 - p1] = '\0';
    p1 = p2 + 1;
    data->heading = atof(aux_buf);

    // Date
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid date\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->day = atoi(aux_buf);
    p1 += 2;
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->month = atoi(aux_buf);
    p1 += 2;
    memcpy(aux_buf, p1, 2);
    aux_buf[2] = '\0';
    data->year = atoi(aux_buf);
    p1 = p2 + 1;

    // Magnetic variation
    ASSERT_RMC(p2 = strchr(p1, ','), "Invalid magnetic var\n", parse_rmc_bailout);
    memcpy(aux_buf, p1, p2 - p1);
    aux_buf[p2 - p1] = '\0';
    p1 = p2 + 1;
    data->magnetic_var = atof(aux_buf);
    ASSERT_RMC(*(p1 + 1) == '*', "Invalid magnetic var\n", parse_rmc_bailout);
    if (*p1 == 'W') {
        data->magnetic_var = -data->magnetic_var;
	} else {
		ASSERT_RMC(*p1 == 'E', "Invalid magnetic var\n", parse_rmc_bailout);
	}
    p1 = p2 + 1;

	return RMC_PARSE_SUCCESSFUL_WITH_FIX;
	
parse_rmc_bailout:
    return RMC_PARSE_FAILED;
}

/*******************************************************************************
*                          Static Function Definitions
*******************************************************************************/

/**
 *	@}		// end of nmea_parser
 */ 
	
/*******************************************************************************
*                          End of File
*******************************************************************************/

