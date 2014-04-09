#ifndef _WARMUP2_H_
#define _WARMUP2_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include "my402list.h"

#ifndef TIME
#define TIME	ctime(NULL)
#endif

#ifndef MAXARRAYSIZE
#define MAXARRAYSIZE	30
#endif

#define TIME_IN_USEC(tv)  ((double)(tv.tv_sec*1000000 + tv.tv_usec))
#define MAX_INT		2147483647L

struct command_line_args
{
	double packet_rate, serving_rate, token_rate;
 	int bucket_depth, token_per_packet, no_of_packets;
	char *tsfile_name; 
};

struct packet
{
	int packet_id; 
	int inter_arrival_time, service_time, token_per_packet;	//inter_arrival_time and serving time in msec
	struct timeval Arrival_timestamp, Q1timestamp, Q2timestamp, Q1leaves, Q2leaves, Leaves_server;	
	double precise_packet_inter_arrival, precise_packet_service_time, time_in_Q1, time_in_Q2, time_in_system;
};

/*struct trace_object
{
	struct command_line_args object;
	FILE *fp;
};
*/
struct timeval GlobalStartTime, GlobalEndTime;
//double usec_GlobalStartTime = 0;
static const struct option longOpts[] = {
{"lambda", required_argument, NULL, 'l'},
{"mu", required_argument, NULL, 'm'},
{"B", required_argument, NULL, 'B'},
{"r", required_argument, NULL, 'r'},
{"P", required_argument, NULL, 'P'},
{"n", required_argument, NULL, 'n'},
{"t", required_argument, NULL, 't'},
{0,0,0,0}
};
	
pthread_t packet_thread, token_thread, server_thread, interrupt_thread;
extern int opterr;
int TraceDriven = 0, EndTokenThread = 0, EndServerThread = 0;
int packet_count = 0, discarded_packets = 0, cancelled_packets = 0, completed_packets = 0;
int token_count = 0, dropped_tokens = 0;
int tokens_in_bucket = 0;
int q1_temp_count = 0, join_interrupt = 0;
double sum_packet_inter_arrival = 0, sum_packet_service_time = 0;
double total_time_spent_at_Q1 = 0, total_time_spent_at_Q2 = 0, total_time_spent_in_system;
double squared_time_in_system = 0;
My402List Q1PacketList, Q2PacketList, token_bucket_list;
pthread_mutex_t token_bucket = PTHREAD_MUTEX_INITIALIZER; 
//pthread_mutex_t server_gateway = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_q2_empty = PTHREAD_COND_INITIALIZER; 
sigset_t signal_set;
struct sigaction act;
FILE *fp = NULL;

void print_command_line(struct command_line_args *);
void parse_command_line(int , char **, struct command_line_args *);
void parse_input_from_file(struct command_line_args *);
//void tracedriven_packet_procedure(struct command_line_args *object);
void deterministic_packet_procedure(struct command_line_args *);
void tracedriven_packet_procedure(struct command_line_args *);
void parse_packet_attributes( struct packet *);
void parse_tsfile(struct command_line_args *object);
void CleanUp();
#endif
