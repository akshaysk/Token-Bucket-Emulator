/********************************************************************
Token Bucket emulator
*********************************************************************/
#include "warmup2.h"
#define MAXLOGSIZE 256
void LOG(FILE *fp,char *format,...)
{
	va_list arguments;
	char message[MAXLOGSIZE];
	int msgsize;
	flockfile(fp);
	va_start(arguments,format);
	msgsize = vsnprintf(message,sizeof(message),format,arguments);
	va_end(arguments);
	if(msgsize < 0)
		return;
	fprintf(fp,"%s",message);
	funlockfile(fp);

}
void print_command_line(struct command_line_args *ob)
{
	LOG(stdout,"Emulation Parameters:\n");
	if(TraceDriven == 1)
		LOG(stdout,"\tr = %.6g\n\tB = %d\n\ttsfile = %s\n",ob->token_rate,ob->bucket_depth,ob->tsfile_name);
	else
		LOG(stdout,"\tlambda = %.6g\n\tmu = %.6g\n\tr = %.6g\n\tB = %d\n\tP = %d\n\tnumber to arrive = %d\n",ob->packet_rate,ob->serving_rate,ob->token_rate,ob->bucket_depth,ob->token_per_packet,ob->no_of_packets);

}

void parse_command_line(int argc, char **argv, struct command_line_args *ob)
{
	int opt = 0, longIndex = 0;
	double temp =0;
	memset(ob,'\0',sizeof(struct command_line_args));
	opt = getopt_long_only(argc, argv, ":", longOpts, &longIndex);
	
	while (opt != -1)
	{
		switch(opt) 
		{
			case 'l':
				ob->packet_rate = atof(optarg);
				break;		
			case 'm':
				ob->serving_rate = atof(optarg);
				break;		
			case 'r':
				ob->token_rate = atof(optarg);
				break;				
			case 'B':
				temp = atof(optarg);
				if(temp != (int)temp || temp < 0)	
				{
					LOG(stderr,"\nError: Argument for option -B should be positive integer less than 2147483647\n");
					exit(-1);
				}
				ob->bucket_depth = (int)temp;
				break;				

			case 'P':
				temp = atof(optarg);
				if(temp != (int)temp || temp < 0)	
				{
					LOG(stderr,"\nError: Argument for option -P should be positive integer less than 2147483647\n");
					exit(-1);
				}
				ob->token_per_packet = (int)temp;
				break;				

			case 'n':
				temp = atof(optarg);
				if(temp != (int)temp || temp < 0)	
				{
					LOG(stderr,"\nError: Argument for option -n should be positive integer less than 2147483647\n");
					exit(-1);
				}
				ob->no_of_packets = (int)temp;

				break;				

			case 't':
				TraceDriven = 1;
				ob->tsfile_name = optarg;
				break;

			case ':':
			case '?':
			default :
				LOG(stderr, "\nInvalid command line arguments passed\n");
				exit(-1);
				break;

		}

	opt = getopt_long_only(argc, argv, ":", longOpts, &longIndex);
	}
	
	if(TraceDriven)
		parse_tsfile(ob);

	if(!ob->packet_rate)
		ob->packet_rate = 0.5;
	if(!ob->serving_rate)
		ob->serving_rate = 0.35;
	if(!ob->token_rate)
		ob->token_rate = 1.5;
	if(!ob->bucket_depth)
		ob->bucket_depth = 10;
	if(!ob->token_per_packet)
		ob->token_per_packet = 3;
	if(!ob->no_of_packets)
		ob->no_of_packets = 20;

	print_command_line(ob);
}

void parse_packet_attributes(struct packet *object)
{
	char line[MAXARRAYSIZE], *start_ptr;
	int i = 0;
	
	fgets(line,sizeof(line),fp);
	start_ptr = strtok(line, " \t");
	while(start_ptr != NULL)
	{
		switch(i)
		{
			case 0:
				object->inter_arrival_time = atoi(start_ptr);
				break;
			case 1:
				object->token_per_packet = atoi(start_ptr);
				break;
			case 2:
				object->service_time = atoi(start_ptr);
				break;
			default:
				break;
		}
		i++;
		start_ptr = strtok(NULL, " \t");
	}
}



struct packet* create_packet(double inter_arrival_time, double service_time, int token_per_packet)
{
	struct packet *p;
	p = (struct packet* )malloc(sizeof(struct packet));
	if(!p)
	{
		LOG(stderr, "Error: Could not allocate memory for packet");
		exit(0);
	}
	p->packet_id = packet_count; 
	p->precise_packet_inter_arrival = inter_arrival_time;			
	p->inter_arrival_time = (int)inter_arrival_time;		//in msec
	p->precise_packet_service_time = service_time;		//in msec
	p->service_time = (int)service_time;			//in msec
	p->token_per_packet = token_per_packet;
	memset(&(p->Q1timestamp),0,sizeof(struct timeval));
	memset(&(p->Q2timestamp),0,sizeof(struct timeval));
	memset(&(p->Q1leaves),0,sizeof(struct timeval));
	memset(&(p->Q2leaves),0,sizeof(struct timeval));
	memset(&(p->Arrival_timestamp),0,sizeof(struct timeval));
	memset(&(p->Leaves_server),0,sizeof(struct timeval));
	p->time_in_Q1 = 0;
	p->time_in_Q2 = 0;
	p->time_in_system = 0;
	return p;	
}


void TIMESTAMP(struct packet *p,int que_num)
{
	if(que_num == 1)
		gettimeofday(&(p->Q1timestamp), NULL);
	if(que_num == 2)
		gettimeofday(&(p->Q2timestamp), NULL);
}

void move_packet_q1_to_q2(struct command_line_args *object, My402ListElem *elem, struct packet *p)
{
	double time;
	int isEmpty = 0;
	My402ListUnlink(&Q1PacketList, elem);
	gettimeofday(&(p->Q1leaves),NULL);
	time = (TIME_IN_USEC(p->Q1leaves) - TIME_IN_USEC(GlobalStartTime))/1000;
	p->time_in_Q1 = (TIME_IN_USEC(p->Q1leaves) - TIME_IN_USEC(p->Q1timestamp))/1000;
	LOG(stdout, "%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token%c\n",time,p->packet_id,p->time_in_Q1,tokens_in_bucket,(tokens_in_bucket > 1 ?'s':' '));	
	q1_temp_count++;
		
	isEmpty = My402ListEmpty(&Q2PacketList);
	My402ListAppend(&Q2PacketList, p);
	gettimeofday(&(p->Q2timestamp),NULL);
	time = (TIME_IN_USEC(p->Q2timestamp) - TIME_IN_USEC(GlobalStartTime))/1000;
	LOG(stdout, "%012.3fms: p%d enters Q2\n",time,p->packet_id);	
	if(isEmpty)
		pthread_cond_signal(&is_q2_empty);
		
}

void calculate_stats(struct packet * p)
{

	sum_packet_service_time += p->precise_packet_service_time;
	total_time_spent_at_Q1 += p->time_in_Q1;	
	total_time_spent_at_Q2 += p->time_in_Q2;	
	total_time_spent_in_system += p->time_in_system;
	squared_time_in_system += p->time_in_system * p->time_in_system;
}

void server_procedure(struct command_line_args *object)
{
	struct packet *p;
	My402ListElem *elem = NULL;
	struct timespec tim;
	double time;
	long time_diff_in_nsec;
	int i = 0;
	while( i < object->no_of_packets && !EndServerThread) 
	{
		pthread_mutex_lock(&token_bucket);
		while(My402ListEmpty(&Q2PacketList)&&!EndServerThread)
			pthread_cond_wait(&is_q2_empty,&token_bucket);

		if(EndServerThread == 1)
		{
			pthread_mutex_unlock(&token_bucket);
			break;
		}

		elem = My402ListFirst(&Q2PacketList);
		if(elem == NULL)
		{
			pthread_mutex_unlock(&token_bucket);
			break;
		}
		p = (struct packet *)elem->obj;
		My402ListUnlink(&Q2PacketList, elem);

		pthread_mutex_unlock(&token_bucket);
	
		gettimeofday(&(p->Q2leaves), NULL);
		time = (TIME_IN_USEC(p->Q2leaves) - TIME_IN_USEC(GlobalStartTime))/1000;
		p->time_in_Q2 = (TIME_IN_USEC(p->Q2leaves) - TIME_IN_USEC(p->Q2timestamp))/1000;

		LOG(stdout, "%012.3fms: p%d begin service at S, time in Q2 = %.3fms\n",time,p->packet_id,p->time_in_Q2);
 
		time_diff_in_nsec = (long)((((p->precise_packet_service_time)/1000) - (p->service_time/1000))*1000000000L);
		tim.tv_sec = (p->service_time)/1000;
		tim.tv_nsec = time_diff_in_nsec;
	
		nanosleep(&tim, NULL);
	
		gettimeofday(&(p->Leaves_server), NULL);
		time = (TIME_IN_USEC(p->Leaves_server) - TIME_IN_USEC(GlobalStartTime))/1000;
		p->time_in_system = (TIME_IN_USEC(p->Leaves_server) - TIME_IN_USEC(p->Arrival_timestamp))/1000;
		p->precise_packet_service_time = (TIME_IN_USEC(p->Leaves_server) - TIME_IN_USEC(p->Q2leaves))/1000;
		LOG(stdout, "%012.3fms: p%d departs from S, service time = %.3fms, time in system = %.3fms\n",time,p->packet_id,p->precise_packet_service_time,p->time_in_system);
		completed_packets ++;
		calculate_stats(p);
		if((packet_count == object->no_of_packets) &&(completed_packets == (packet_count-discarded_packets)) && My402ListEmpty(&Q2PacketList))
		{
			EndServerThread = 1;
			pthread_cond_signal(&is_q2_empty);
		}
		i++;
	}
	pthread_exit(NULL);
	
}

void token_bucket_procedure(struct command_line_args *object)
{
	double time, precise_inter_arrival_time;
	int inter_arrival_time = 0;	
	struct timespec tim;
	struct timeval now;
	struct packet *p;
	long time_diff_in_nsec;
	My402ListElem *elem = NULL;
	
	precise_inter_arrival_time = (double)(1/object->token_rate);
	inter_arrival_time = (int)precise_inter_arrival_time;
	if( inter_arrival_time > 10)
	{
		precise_inter_arrival_time = 10; inter_arrival_time = 10;
	}

	time_diff_in_nsec = (long)((precise_inter_arrival_time - inter_arrival_time)*1000000000L);
	
	tim.tv_sec = inter_arrival_time;
	tim.tv_nsec = time_diff_in_nsec;

	while(1)
	{
		nanosleep(&tim,NULL);
		pthread_mutex_lock(&token_bucket);
		token_count++;
		gettimeofday(&now, NULL);
		time = (TIME_IN_USEC(now) - TIME_IN_USEC(GlobalStartTime))/1000;

		if(tokens_in_bucket >= object->bucket_depth)
		{
			dropped_tokens++;
			LOG(stdout, "%012.3fms: token t%d arrives, dropped\n",time, token_count);
			pthread_mutex_unlock(&token_bucket);
			continue;
		}
		tokens_in_bucket++;
		LOG(stdout, "%012.3fms: token t%d arrives, token bucket now has %d token%c\n",time,token_count, tokens_in_bucket,(tokens_in_bucket > 1?'s':' '));
		
		elem = My402ListFirst(&Q1PacketList);
		if(elem == NULL)
		{
			pthread_mutex_unlock(&token_bucket);
			continue;
		}	
		p = (struct packet *)elem->obj;
		if(tokens_in_bucket >= p->token_per_packet)
		{
			tokens_in_bucket -= p->token_per_packet;
			move_packet_q1_to_q2(object,elem,p); 

		}
		pthread_mutex_unlock(&token_bucket);
		if((q1_temp_count == object->no_of_packets) && My402ListEmpty(&Q1PacketList))
		{
			break;
		}
	}

	if(My402ListEmpty(&Q2PacketList))
	{
		EndServerThread = 1;
		pthread_cond_broadcast(&is_q2_empty);
	}
	pthread_exit(NULL);

} 

void parse_tsfile(struct command_line_args *object)
{
	struct stat s;
	char line[MAXARRAYSIZE];
	if(stat(object->tsfile_name,&s)!=0)
	{
		LOG(stderr,"\nFile doesn't exist. Exiting\n");
		exit(-1);
	}
	else if(s.st_mode & S_IFDIR){
		LOG(stderr,"\nError: %s is a directory\n",object->tsfile_name);
		exit(-1);	
	}

	fp = fopen(object->tsfile_name,"r");
	if(!fp)
	{
		LOG(stderr, "\nError: Could not open the source file %s. Access denied\n",object->tsfile_name);
		exit(-1);
	}

	if(!fgets(line,sizeof(line),fp))
	{	
		LOG(stderr,"\nError reading from the tsfile. Exiting");
		exit(0);
	}
	sscanf(line,"%d",&object->no_of_packets);
}

void tracedriven_packet_procedure(struct command_line_args *object)
{
	struct packet *p,*p2;
	int i = 0;
	char line[MAXARRAYSIZE];
	int inter_arrival_time = 0, service_time = 0, token_per_packet = 0;
	double time, precise_inter_arrival_time, precise_service_time;
	struct timespec tim;
	struct timeval prev_packet_arrival_time,now,delta;
	long time_diff_in_nsec,delta_time;
	My402ListElem *elem = NULL;

	prev_packet_arrival_time.tv_sec = GlobalStartTime.tv_sec;
	prev_packet_arrival_time.tv_usec = GlobalStartTime.tv_usec;

	while(i < object->no_of_packets)
	{

		if((packet_count == object->no_of_packets) &&(q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
		{
			pthread_cancel(token_thread);
		}
		if(!fgets(line,sizeof(line),fp))
		{
			LOG(stderr,"\nError reading from the tsfile. Exiting");
			exit(0);
		}		
		
		sscanf(line,"%d\t%d\t%d",&inter_arrival_time, &token_per_packet, &service_time);
	
		precise_inter_arrival_time = ((double)inter_arrival_time)/1000;
		inter_arrival_time = (int)(precise_inter_arrival_time);
		time_diff_in_nsec = (long)((precise_inter_arrival_time - inter_arrival_time)*1000000000L);
	
		tim.tv_sec = inter_arrival_time;
		tim.tv_nsec = time_diff_in_nsec;

		precise_service_time = ((double)service_time/1000);
		service_time = (int)(precise_service_time);
	
		gettimeofday(&delta,NULL);
		delta_time = TIME_IN_USEC(delta) - TIME_IN_USEC(prev_packet_arrival_time);
		tim.tv_sec = inter_arrival_time - (delta_time/1000000);
		tim.tv_nsec = (time_diff_in_nsec - (delta_time*1000)) <= 0 ? time_diff_in_nsec : ((time_diff_in_nsec - (delta_time*1000)));

		nanosleep(&tim,NULL);

		packet_count++;
		p = create_packet((precise_inter_arrival_time*1000),(precise_service_time*1000),token_per_packet);
		gettimeofday(&(p->Arrival_timestamp), NULL);


		p->precise_packet_inter_arrival = (TIME_IN_USEC(p->Arrival_timestamp) - TIME_IN_USEC(prev_packet_arrival_time))/1000;
		time = (TIME_IN_USEC(p->Arrival_timestamp) - TIME_IN_USEC(GlobalStartTime))/1000;

		sum_packet_inter_arrival += p->precise_packet_inter_arrival;
		prev_packet_arrival_time = p->Arrival_timestamp;
		pthread_mutex_lock(&token_bucket);
		if(p->token_per_packet > object->bucket_depth)
		{
			gettimeofday(&now, NULL);
			p->time_in_system = (TIME_IN_USEC(now) - TIME_IN_USEC(p->Arrival_timestamp))/1000;
			LOG(stdout, "%012.3fms: packet p%d arrives, needs %d token%c, dropped\n",time, p->packet_id, p->token_per_packet,(p->token_per_packet > 1 ?'s':' '));
			free(p);
			discarded_packets++;
			pthread_mutex_unlock(&token_bucket);
			i++;
			continue;
		}
		LOG(stdout, "%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n",time,p->packet_id, p->token_per_packet, p->precise_packet_inter_arrival);
		My402ListAppend(&Q1PacketList, p);
		gettimeofday(&(p->Q1timestamp), NULL);
		time = (TIME_IN_USEC(p->Q1timestamp) - TIME_IN_USEC(GlobalStartTime))/1000;
		LOG(stdout, "%012.3fms: p%d enters Q1\n",time,p->packet_id);
			
		elem = My402ListFirst(&Q1PacketList);
		if(elem == NULL)
		{
			pthread_mutex_unlock(&token_bucket);
			break;
		}

		p2 = (struct packet *)elem->obj;
		if(p2->token_per_packet <= tokens_in_bucket)
		{
			tokens_in_bucket -= p2->token_per_packet;
			move_packet_q1_to_q2(object,elem, p2);
		} 	
		
		pthread_mutex_unlock(&token_bucket);
		if((packet_count == object->no_of_packets) &&(q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
		{
			pthread_cancel(token_thread);
		}

		i++;

	}
	if((packet_count == object->no_of_packets) &&(q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
	{
		pthread_cancel(token_thread);
	}

	if((packet_count == object->no_of_packets) &&(completed_packets == (packet_count - discarded_packets)) && My402ListEmpty(&Q2PacketList))
	{
		EndServerThread = 1;
		pthread_cond_signal(&is_q2_empty);
	}


}

void deterministic_packet_procedure(struct command_line_args *object)
{

	struct packet *p,*p2;
	int i = 0;
	double time, precise_inter_arrival_time, precise_service_time;
	int inter_arrival_time = 0, service_time = 0;
	struct timespec tim;
	struct timeval prev_packet_arrival_time, now, delta;
	long time_diff_in_nsec,delta_time;
	My402ListElem *elem = NULL;

	precise_inter_arrival_time = ((double)1/object->packet_rate);
	inter_arrival_time = (int)(precise_inter_arrival_time);
	if(inter_arrival_time > 10)
	{	
		precise_inter_arrival_time = 10;inter_arrival_time = 10;
	}
	time_diff_in_nsec = (long)((precise_inter_arrival_time - inter_arrival_time)*1000000000L);
	
	
	precise_service_time = ((double)1/object->serving_rate);
	service_time = (int)(precise_service_time);	
	
	if(service_time > 10)
	{	
		precise_service_time = 10;service_time = 10;
	}
	prev_packet_arrival_time.tv_sec = GlobalStartTime.tv_sec;
	prev_packet_arrival_time.tv_usec = GlobalStartTime.tv_usec;

	while(i < object->no_of_packets)
	{
		if((packet_count == object->no_of_packets) && (q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
		{
			pthread_cancel(token_thread);
		}
		gettimeofday(&delta,NULL);
		delta_time = TIME_IN_USEC(delta) - TIME_IN_USEC(prev_packet_arrival_time);
		tim.tv_sec = inter_arrival_time - (delta_time/1000000);
		tim.tv_nsec = (time_diff_in_nsec - (delta_time*1000)) <= 0 ? time_diff_in_nsec : ((time_diff_in_nsec - (delta_time*1000)));
		nanosleep(&tim,NULL);

		packet_count++;
		p = create_packet((precise_inter_arrival_time*1000),(precise_service_time*1000),object->token_per_packet);
		gettimeofday(&(p->Arrival_timestamp), NULL);
		p->precise_packet_inter_arrival = ((TIME_IN_USEC(p->Arrival_timestamp) - TIME_IN_USEC(prev_packet_arrival_time)))/1000;
		time = (TIME_IN_USEC(p->Arrival_timestamp) - TIME_IN_USEC(GlobalStartTime))/1000;

		sum_packet_inter_arrival += p->precise_packet_inter_arrival;
		prev_packet_arrival_time = p->Arrival_timestamp;
		pthread_mutex_lock(&token_bucket);
		if(p->token_per_packet > object->bucket_depth)
		{
			gettimeofday(&now, NULL);
			p->time_in_system = (TIME_IN_USEC(now) - TIME_IN_USEC(p->Arrival_timestamp))/1000;

			LOG(stdout, "%012.3fms: packet p%d arrives, needs %d token%c, dropped\n",time, p->packet_id,p->token_per_packet,(p->token_per_packet > 1?'s':' '));
			free(p);
			discarded_packets++;
			pthread_mutex_unlock(&token_bucket);
			i++;
			continue;
		}
		LOG(stdout, "%012.3fms: p%d arrives, needs %d token%c, inter-arrival time = %.3fms\n",time,p->packet_id, p->token_per_packet,(p->token_per_packet > 1? 's':' ') ,p->precise_packet_inter_arrival);
		My402ListAppend(&Q1PacketList, p);
		gettimeofday(&(p->Q1timestamp), NULL);
		time = (TIME_IN_USEC(p->Q1timestamp) - TIME_IN_USEC(GlobalStartTime))/1000;
		LOG(stdout, "%012.3fms: p%d enters Q1\n",time,p->packet_id);
		elem = My402ListFirst(&Q1PacketList);
		p2 = (struct packet *)elem->obj;
		if(p2->token_per_packet <= tokens_in_bucket)
		{
			tokens_in_bucket -= p2->token_per_packet;
			move_packet_q1_to_q2(object,elem, p2);
		} 	
		
		pthread_mutex_unlock(&token_bucket);
		if((packet_count == object->no_of_packets) && (q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
		{
			pthread_cancel(token_thread);
		}

		i++;
	}


	if((packet_count == object->no_of_packets) &&(q1_temp_count == (packet_count-discarded_packets)) && My402ListEmpty(&Q1PacketList))
	{
			pthread_cancel(token_thread);
	}

	if((packet_count == object->no_of_packets) &&(completed_packets == (packet_count - discarded_packets)) && My402ListEmpty(&Q2PacketList))
	{
		EndServerThread = 1;
		pthread_cond_signal(&is_q2_empty);
	}
}


void Statistics(struct command_line_args *object)
{
	
	double 	average_packet_inter_arrival = (sum_packet_inter_arrival)/(packet_count);
	double	packet_drop_probability = ((double)discarded_packets)/packet_count;	
	 
	double average_packet_service_time = sum_packet_service_time/completed_packets;
	double	average_time_spent_in_system = (total_time_spent_in_system)/completed_packets;
	double	average_squared_time_in_system = (squared_time_in_system)/completed_packets;

	double token_drop_probability = ((double)dropped_tokens)/token_count;
	double total_emulation_time = (TIME_IN_USEC(GlobalEndTime) - TIME_IN_USEC(GlobalStartTime))/1000;
	double std_dev_time_spent_in_system = 0;

	double average_no_of_packets_at_Q1 = (total_time_spent_at_Q1)/(total_emulation_time);
	double average_no_of_packets_at_Q2 = (total_time_spent_at_Q2)/(total_emulation_time);
	double average_no_of_packets_at_S = (sum_packet_service_time)/(total_emulation_time);
	std_dev_time_spent_in_system = sqrt(average_squared_time_in_system - (average_time_spent_in_system * average_time_spent_in_system));
	LOG(stdout,"Statistics:\n\n");
	if(packet_count)
		LOG(stdout,"\taverage packet inter-arrival time = %.6g sec\n",(average_packet_inter_arrival/1000));
	else
		LOG(stdout,"\taverage packet inter-arrival time = N/A : No packets arrived at this facility\n");
		
	if(completed_packets)
		LOG(stdout,"\taverage packet service time = %.6g sec\n\n",(average_packet_service_time/1000));
	else
		LOG(stdout,"\taverage packet service time = N/A : No packets processed at this facility\n\n");

	if(total_emulation_time)
	{
		LOG(stdout,"\taverage number of packets in Q1 = %.6g packets\n",average_no_of_packets_at_Q1);
		LOG(stdout,"\taverage number of packets in Q2 = %.6g packets\n",average_no_of_packets_at_Q2);
		LOG(stdout,"\taverage number of packets at S = %.6g packets\n\n",average_no_of_packets_at_S);
	}
	else
	{
		LOG(stdout,"\taverage number of packets in Q1 = N/A : Emulation Time is zero\n");
		LOG(stdout,"\taverage number of packets in Q2 = N/A : Emulation Time is zero");
		LOG(stdout,"\taverage number of packets at S = N/A : Emulation Time is zero\n\n");
	}

	if(completed_packets)
	{
		LOG(stdout,"\taverage time a packet spent in system = %.6g sec\n",(average_time_spent_in_system/1000));
		LOG(stdout,"\tstandard deviation for time spent in system = %.6g sec\n\n",(std_dev_time_spent_in_system/1000));
	}
	else
	{
		LOG(stdout,"\taverage time a packet spent in system = N/A : No packets processed at this facility\n");
		LOG(stdout,"\tstandard deviation for time spent in system = N/A : No packets processed at this facility\n\n");
	}

	if(token_count)
		LOG(stdout,"\ttoken drop probability = %.6g\n",token_drop_probability);
	else
		LOG(stdout,"\ttoken drop probability = N/A : No tokens arrived at this facility\n");

	if(packet_count)
		LOG(stdout,"\tpacket drop probability = %.6g\n",packet_drop_probability);
	else
		LOG(stdout,"\tpacket drop probability = N/A : No packets arrived at this facility\n");

}

void Initialize()
{
	My402ListInit(&Q1PacketList);
	My402ListInit(&Q2PacketList);
	My402ListInit(&token_bucket_list);	
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
}

void CleanUp(int sig_no)
{
	EndServerThread = 1;
	pthread_cond_broadcast(&is_q2_empty);
	cancelled_packets = Q1PacketList.num_members + Q2PacketList.num_members;
	pthread_cancel(packet_thread);
	pthread_cancel(token_thread);
	pthread_exit(NULL);	
}

void handler()
{

	act.sa_handler = CleanUp;
	sigaction(SIGINT, &act,NULL);
	pthread_sigmask(SIG_UNBLOCK,&signal_set,NULL);
	while(!join_interrupt)
	{
		sleep(10);
	}
}

int main(int argc, char **argv)
{
	double time = 0;
	struct command_line_args object;
	memset(&object,0,sizeof(struct command_line_args));
	parse_command_line(argc,argv,&object);	
	Initialize();

	gettimeofday(&GlobalStartTime,0);
	LOG(stdout, "%012.3fms: emulation begins\n",time);	
	

	pthread_sigmask(SIG_BLOCK,&signal_set,NULL);
	if(TraceDriven){
		pthread_create(&packet_thread, 0, (void *)tracedriven_packet_procedure, &object);
	}	else
		pthread_create(&packet_thread, 0, (void *)&deterministic_packet_procedure, &object);
	
	
	pthread_create(&token_thread, 0,(void *)&token_bucket_procedure,&object);
	pthread_create(&server_thread, 0,(void *)&server_procedure,&object);
	pthread_create(&interrupt_thread, 0,(void *)&handler,NULL);
	
	pthread_join(packet_thread,0);
	pthread_join(token_thread,0);
	pthread_join(server_thread,0);
	join_interrupt = 1;
	pthread_join(interrupt_thread, 0);
	gettimeofday(&GlobalEndTime,0);
	Statistics(&object);
	My402ListUnlinkAll(&Q1PacketList);
	My402ListUnlinkAll(&Q2PacketList);
	return 0;
}
