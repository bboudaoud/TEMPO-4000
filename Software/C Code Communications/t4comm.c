/*********************************************************
 * Magic C-based Communications Code for TEMPO 4000
 * -------------------------------------------------------
 * Author: Ben Boudaoud (bb3jd@virginia.edu)
 * Date: January 6, 2014
 * University of Virginia INERTIA Team 2014
 *********************************************************
 * This is a VERY crude pass at a simple communications
 * interface for the TEMPO 4000 platform. This code
 * runs in the command window and prompts the user for a 
 * command character. Once entered, the tools lets the
 * user enter/displays any relevant system info, along with
 * the produced command and prints said command to an 
 * output command file entitled 'comm_out.txt'.
 *
 * This output file is produced for the purpose of being
 * able to easily send the command to a device over the FTDI
 * Virtual Comm Port (VCP) interface using a tool like
 * RealTerm or Putty. One useful trick regarding these files
 * is to generate generic commands (such as handshake, or get
 * node time) once the rename these 'comm_out.txt' files to
 * 'handshake.txt' or 'gettime.txt' for future use.
 *
 * This tool is not intended ONLY FOR INTERNAL USE and 
 * should NEVER be provided to any collaborator who has
 * not specificially requested it. The source code is not
 * protected, and can be used as an intro to understanding
 * the TEMPO communications interface.
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Enums and Typedefs (DO NOT TOUCH)
typedef enum month_type{		/// Enumerated month type
	JAN = 1,
	FEB = 2,
	MAR = 3,
	APR = 4,
	MAY = 5,
	JUN = 6,
	JUL = 7,
	AUG = 8,
	SEP = 9,
	OCT = 10,
	NOV = 11,
	DEC = 12
} month;
typedef struct systemTime{		/// Typedef for system time
	unsigned short year;
	month mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
} sysTime;

// Function Prototypes (DO NOT TOUCH)
sysTime get_time_obj(void);		// Get time function prototype
inline void printTime(sysTime t);	// Print time function prototype
unsigned short runChecksum(unsigned char *Buffer, unsigned int numBytes, unsigned short checksum);

// Command info (DO NOT TOUCH)
#define	ARG_LEN		16
#define CRC_LEN		ARG_LEN + 2
#define CMD_LEN		ARG_LEN + 4
#define CMD_ARG_START	1

// Flash Command Set (CAN BE EXPANDED)
#define			CMD_HANDSHAKE			'K'
#define 		CMD_GET_STATUS			'?'
#define			CMD_SET_CLK			'C'
#define			CMD_GET_CLK			'T'	
#define			CMD_LED_ON			'L'
#define			CMD_LED_OFF			'O'

main(){
	// General mamagement variables
	char comm;
	unsigned char cmdStr[CMD_LEN] = {0};
	unsigned short newcheck, checksum;
	FILE *f;

	// Command specific variables
	unsigned int rate, i;
	unsigned short numOfSectors, sessSerial, timeSerial, timeEpoch, nodeID;
	unsigned long sector;
	sysTime t = { 0 };

	// Initialization
	f = fopen("comm_out.txt", "w");		// Open file for output
	printf("Enter command character: ");	// Print prompt to console
	scanf("%c", &comm);			// Read user input
	cmdStr[1] = 0;
	cmdStr[0] = comm;			// Set command character

	// Command parsing
	switch(comm) {	
		case CMD_HANDSHAKE:
			printf("\nCommand: Handshake\n");
			break;
		case CMD_GET_STATUS:
			printf("\nCommand: Status\n");
			break;
		case CMD_GET_CLK:
			printf("\nCommand: Get Node Time\n");
			break;
		case CMD_SET_CLK:
			printf("\nCommand: Set Node Time\n");
			t = get_time_obj();
			printTime(t);
			memcpy(&cmdStr[CMD_ARG_START], (unsigned char*)&t, sizeof(sysTime));
			break;
		case CMD_LED_ON:
			printf("\nCommand: Turn on LED\n");
			break;
		case CMD_LED_OFF:
			printf("\nCommand: Turn off LED\n");
			break;
		default:
			printf("\nCommand not recognized\n");
			return;
	}

	// Checksums
	checksum = runChecksum(cmdStr, CRC_LEN, 0);
	printf("\nChecksum = %04x\n", checksum);

	*((unsigned int *)(&cmdStr[CRC_LEN])) = checksum;	// Add checksum
	cmdStr[1] = 'S';					// Add start of sequence

	// Shift and Start of Sequence
//	for(i = 0; i < CMD_LEN; i++) {
//		out[i+1] = cmdStr[i];
//	}
//	out[0] = 'S';	// Add start of sequence

	// File and console output
	printf("\nCommand = %s\n", cmdStr); 	// Print command to console
	fwrite(cmdStr, 1, sizeof(cmdStr), f);	// Print command to file
	fclose(f);				// Close output file
	return;
}

sysTime get_time_obj(void)
{
	struct tm *tm;
	time_t t;
	sysTime sTime;
	t = time(NULL);			// Get time since epoch
	tm = localtime(&t);		// Convert to local time
	sTime.year = tm->tm_year+1900;
	sTime.mon = tm->tm_mon+1;
	sTime.day = tm->tm_mday;
	sTime.hour = tm->tm_hour;
	sTime.min = tm->tm_min;
	sTime.sec = tm->tm_sec;
	
	return sTime;
}

inline void printTime(sysTime t){
	printf("\nCurrent Date and Time:\n----------------------\n");
	printf("Date: %d/%d/%d\n", t.mon, t.day, t.year);
        printf("Time: %d:%d:%d\n", t.hour, t.min, t.sec);
}

unsigned short runChecksum(unsigned char *Buffer, unsigned int numBytes, unsigned short checksum) {

	unsigned int len = numBytes;
	unsigned short tlen;
	unsigned char *data = Buffer;
	unsigned short sum1, sum2;

	if (checksum == 0) { // Check for non-initialized checksum (0 impossible result)
		sum1 = 0xff;
		sum2 = 0xff;
	} 
	else { // Parse the previous two component sums out of the old checksum
		sum2 = checksum >> 8;
		sum1 = checksum & 0xff;
	}

	while (len > 0) {
		if(len > 20){
		       tlen = 20;
		}
		else {
			tlen = len;
		}
		len -= tlen;
		do {
			sum1 += *data++;
			sum2 += sum1;
		} while (--tlen);

		// The reduction step below is equivalent to a partial modulo 255 (may have a 1 in the upper byte)
		// This works by taking sum1 % 256 then adding a 1 for each 256 in sum1 (sum1/256 = sum1 >> 8)
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}
	// Second reduction step to assure reduction to 8 bits (no overlap in combination for final checksum value)
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	checksum = (sum2 << 8) | sum1;
	return checksum;
}

int fletcherChecksum(unsigned char *Buffer, int numBytes)
{	
	int len = numBytes;
	int result = 0;
	unsigned char *data = Buffer;
	unsigned short sum1 = 0xff, sum2 = 0xff;
 
        while (len) {
                int tlen = len > 20 ? 20 : len;
                len -= tlen;
                do {
                        sum1 += *data++;
                        sum2 += sum1;
                } while (--tlen);
                sum1 = (sum1 & 0xff) + (sum1 >> 8);
                sum2 = (sum2 & 0xff) + (sum2 >> 8);
        }
        /* Second reduction step to reduce sums to 8 bits */
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
       // *data++ = (unsigned char)sum1;
       // *data++ = (unsigned char)sum2;
       result = (sum2 << 8) + sum1;
       
        return result;
}

unsigned int fletcher16(unsigned char *data, int numBytes){

	unsigned short sum1 = 0, sum2 = 0;
	unsigned char *b = data;
	int i;

    	for(i = 0; i < numBytes; i++){
    	    sum1 = (sum1 + *b) % 255;
    	    sum2 = (sum2 + sum1) % 255;
	    b++;
	}

    	return ((sum2 << 8) | sum1);
}
