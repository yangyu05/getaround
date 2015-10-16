/** @file
 *  Provides test routines for GPS NMEA parser.
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
#include <math.h>

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
static void usage(char *arg);
static void print_rmc_data(nmea_rmc_data_t *data);
static rmc_parse_result test_rmc_input(const char *buf, int buf_size);
static int readline(FILE *f, char *buf, int len);

/*******************************************************************************
*                          Static Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern/Exported Data Definitions
*******************************************************************************/

/*******************************************************************************
*                          Extern/Exported  Function Definitions
*******************************************************************************/

int main(int argc, char **argv)
{
	if ((argc > 3) || (argc == 2 && !strcmp(argv[1], "-h"))) {
		usage(argv[0]);
		exit(0);
	}
	
	if (argc == 1) {
		// valid input with GPS fix
		printf("*** Expect output of valid parse with fix.......");
		char gprmc_str1[] = "$GPRMC,102642.03,A,4813.7943164,S,01621.5693035,W,7.158,156.6705,020713,020.32,E*51";
		if (test_rmc_input(gprmc_str1, strlen(gprmc_str1)) == RMC_PARSE_SUCCESSFUL_WITH_FIX) printf("PASSED\n"); else printf("FAILED\n");
		
		// valid input with no GPS fix
		printf("*** Expect output of valid parse with no fix.......");
		char gprmc_str2[] = "$GPRMC,102642.03,V,4813.7943164,N,01621.5693035,E,7.158,156.6705,020713,020.32,E*49";
		if (test_rmc_input(gprmc_str2, strlen(gprmc_str2)) == RMC_PARSE_SUCCESSFUL_WITH_NO_FIX) printf("PASSED\n"); else printf("FAILED\n");

		// negative case 1: invalid checksum
		printf("*** Expect output of failed checksum.......");
		char gprmc_str_f1[] = "$GPRMC,102642.03,V,4813.7943164,N,01621.5693035,E,7.158,156.6705,020713,020.32,E*5E";
		if (test_rmc_input(gprmc_str_f1, strlen(gprmc_str_f1)) == RMC_PARSE_FAILED) printf("PASSED\n"); else printf("FAILED\n");

		// negative case 2: invalid latitude
		printf("*** Expect output of invalid latitude.......");
		char gprmc_str_f2[] = "$GPRMC,102642.03,A,4813.7943164,I,01621.5693035,E,7.158,156.6705,020713,020.32,E*59";
		if (test_rmc_input(gprmc_str_f2, strlen(gprmc_str_f2)) == RMC_PARSE_FAILED) printf("PASSED\n"); else printf("FAILED\n");

		// negative case 3: invalid longitude
		printf("*** Expect output of invalid longitude.......");
		char gprmc_str_f3[] = "$GPRMC,102642.03,A,4813.7943164,N,1.5693035,E,7.158,156.6705,020713,020.32,E*5B";
		if (test_rmc_input(gprmc_str_f3, strlen(gprmc_str_f3)) == RMC_PARSE_FAILED) printf("PASSED\n"); else printf("FAILED\n");
	} else if (argc == 2) {
		// generate random RMC sentences to a file
		FILE *output_stream = fopen(argv[1], "w");
		if (!output_stream) {
			printf("Failed to open output file %s!\n", argv[1]);
			exit(-2);
		}

		int i = 300;
		struct tm tm;
		time_t t_start = time(NULL);
		double latitude = 3623.3452323;
		double longitude = -12478.242425;
		double speed = 3.728;
		double heading = 54.0897;
		double magnetic_var = 12.4368;

		while (i--) {
			char buffer[256];

			gmtime_r(&t_start, &tm);
			int len = sprintf(buffer, "$GPRMC,%02d%02d%02d.%02d,%c,%.6f,%c,%.6f,%c,%.3f,%.4f,%02d%02d%02d,%.2f,%c*",
					tm.tm_hour, tm.tm_min, tm.tm_sec, 0,
					(float)rand()/(float)RAND_MAX > 0.8 ? 'V' : 'A',
					fabs(latitude), latitude > 0 ? 'N' : 'S',
					fabs(longitude), longitude > 0 ? 'E' : 'W',
					fabs(speed), fabs(heading), 
					tm.tm_mon + 1, tm.tm_mday, tm.tm_year - 100,
					fabs(magnetic_var), magnetic_var > 0 ? 'E' : 'W');

			// calculate checksum
			char checksum = 0;
			char *p = buffer + 1;
			while (*p != '*') {
				checksum ^= *p++;
			}
			sprintf(buffer + len, "%02X", checksum);

			// advance time
			t_start += 5;

			// add random variation to parameters
			latitude += 0.02 * ((float)rand()/(float)RAND_MAX) * ((float)rand()/(float)RAND_MAX > 0.5 ? 1 : -1);
			longitude += 0.02 * ((float)rand()/(float)RAND_MAX) * ((float)rand()/(float)RAND_MAX > 0.5 ? 1 : -1);
			speed += 2 * ((float)rand()/(float)RAND_MAX) * ((float)rand()/(float)RAND_MAX > 0.5 ? 1 : -1);
			heading += 0.02 * ((float)rand()/(float)RAND_MAX) * ((float)rand()/(float)RAND_MAX > 0.5 ? 1 : -1);

			fprintf(output_stream, "%s\n", buffer);
		}

		fclose(output_stream);
	} else {
		// open input RMC stream file
		FILE *input_stream = fopen(argv[1], "r");
		if (!input_stream) {
			printf("Failed to open input file %s!\n", argv[1]);
			exit(-1);
		}
		// open output log file
		FILE *output_stream = fopen(argv[2], "w");
		if (!output_stream) {
			printf("Failed to open output file %s!\n", argv[2]);
			exit(-2);
		}
		
		// go thru the input stream to output valid GPS fix
		char line[256];
		int num_of_sentences = 0, num_of_fixes = 0;
		do {
			int len = readline(input_stream, line, sizeof(line));
			if (len == 0) {
				break;
			}
			
			num_of_sentences++;
			
			nmea_rmc_data_t rmc_data;
			rmc_parse_result parse_res = parse_rmc(&rmc_data, line, len);
			if (parse_res == RMC_PARSE_SUCCESSFUL_WITH_FIX) {
				num_of_fixes++;
				fprintf(output_stream, "%02d:%02d:%02d, %.6f, %.6f\n", 
						rmc_data.hour, rmc_data.min, rmc_data.sec,
						rmc_data.latitude, rmc_data.longitude);
			}
		} while (num_of_fixes < 100);
		
		printf("Done!");
		printf("\tParsed %d sentences to obtain %d fixes\n", num_of_sentences, num_of_fixes);
		fclose(input_stream);
		fclose(output_stream);
	}
	
	return 0;
}

/*******************************************************************************
*                          Static Function Definitions
*******************************************************************************/

static void usage(char *arg)
{
	printf("%s [output_file]\n", arg);
	printf("    Generate random RMC data to output_file\n");
	printf("%s [input_file output_file]\n", arg);
	printf("    Parse RMC sentences from input_file and give valid time/lat/long to output_file\n");
}

static void print_rmc_data(nmea_rmc_data_t *data) 
{
	// print time (HH:MM:SS), status, latitude, longitude, speed, heading, date (MM:DD:YY), and magnetic var
	printf("%02d:%02d:%02d, %c, %.6f, %.6f, %.3f, %.4f, %02d-%02d-%02d, %.2f\n", 
			data->hour, data->min, data->sec, data->status,
			data->latitude, data->longitude, data->ground_speed, data->heading,
			data->month, data->day, data->year,
			data->magnetic_var);
}

static rmc_parse_result test_rmc_input(const char *buf, int buf_size)
{
	nmea_rmc_data_t rmc_data;
	
	rmc_parse_result parse_res = parse_rmc(&rmc_data, buf, buf_size);
	switch (parse_res) {
		case RMC_PARSE_SUCCESSFUL_WITH_FIX:
			printf("Valid RMC with fix\n");
			print_rmc_data(&rmc_data);
			break;
		case RMC_PARSE_SUCCESSFUL_WITH_NO_FIX:
			printf("Valid RMC with no fix\n");
			break;
		case RMC_PARSE_FAILED:
			break;
		default:
			printf("Unknow input\n");
	}	
	
	return parse_res;
}

static int readline(FILE *f, char *buf, int len)
{
    char c;
    int i = 0;

    do {
		fread(&c, 1, 1, f);
		if (feof(f)) 	{	break;	}
		buf[i++] = c;
    } while (i < len && c != '\n');

	if (feof(f)) {
		i++;
	}

	if (c = '\n') {
		buf[--i] = '\0';
	}
	
    return i;
}

/**
 *	@}		// end of nmea_parser
 */ 
	
/*******************************************************************************
*                          End of File
*******************************************************************************/

