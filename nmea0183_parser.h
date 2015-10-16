/** @file
 *  Provides prototypes for initialization and other management functions 
 *  for GPS NMEA parser.
 *
 */

/** @addtogroup nmea_parser NMEA0183 Parser
 *  Initializes, configures, and manages high level control for the  
 *  nmea0183 parser.
 *  @{
 */

#ifndef __NMEA0183_PRASER_H__
#define __NMEA0183_PRASER_H__


/*******************************************************************************
*                          Include Files
*******************************************************************************/
#include <time.h>

/*******************************************************************************
*                          C++ Declaration Wrapper
*******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                          Type & Macro Declarations
*******************************************************************************/
/**
 * Structure to store RMC data
 */
typedef struct nmea_rmc_data_t
{
    char hour;
    char min;
    char sec;
    char status;
    char day;
    char month;
    char year;
	char mode;
    double latitude;
    double longitude;
    double ground_speed;
    double heading;
    double magnetic_var;
} nmea_rmc_data_t;

/**
 * Result from parsing an RMC sentence
 */
typedef enum {
	RMC_PARSE_SUCCESSFUL_WITH_FIX = 0,			/**< Parse successfully completed and a valid GPS fix is found. */
	RMC_PARSE_SUCCESSFUL_WITH_NO_FIX = 1,		/**< Parse successfully completed but no valid GPS fix is found. */
	RMC_PARSE_FAILED = 2,						/**< Parse failed. */
	RMC_PARSE_CODE_INVALID
} rmc_parse_result;

/**
 * Structure to store GPS info of various sentences
 */
typedef struct nmea_0183_data_t
{
   	nmea_rmc_data_t rmcData;
} nmea_0183_data_t;

/*******************************************************************************
*                          Extern Data Declarations
*******************************************************************************/

/*******************************************************************************
*                          Extern Function Prototypes
*******************************************************************************/

rmc_parse_result parse_rmc(nmea_rmc_data_t *data, const char *buf, const int bufSize);

#ifdef __cplusplus
}
#endif

#endif

/**
 *	@}		// end of nmea_parser
 */ 
	
/*******************************************************************************
*                          End File
********************************************************************************/


