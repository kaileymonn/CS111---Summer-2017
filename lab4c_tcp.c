//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<mraa.h>
#include<mraa/aio.h>
#include<string.h>
#include<time.h>
#include<math.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<poll.h>
#include<netdb.h>
#include<netinet/in.h>

//------------------ GLOBAL VARIABLES--------------------//
//Command-line variables
FILE *log_file = NULL;
int port;
char *id;

//Edison sensor variables
mraa_aio_context temp_sensor;
mraa_gpio_context button;
char scale = 'F';
int start_flag = 1;
int period = 1;
int raw_temperature = 0;
const int B = 4275;

//Time variables
time_t timer, start, end;
char time_print[10];
struct tm* timeInfo;

//Socket and host variables
struct sockaddr_in serverIP;
struct hostent *server;
int socket_fd;
char *host = "";


//-----------------ADDITIONAL FUNCTIONS------------------//
double convert_temperature(int raw_temperature, char scale);

void command_handler(const char* command);


//---------------------MAIN ROUTINE----------------------//
int main(int argc, char **argv) {
  static struct option long_opts[] = {
    {"id", required_argument, 0, 'i'},
    {"host", required_argument, 0, 'h'},
    {"log", required_argument, 0, 'l'}
  };
  
  int option = 0;
  while ((option = getopt_long(argc, argv, "i:h:l", long_opts, NULL)) != -1) {
    switch (option)
      {
      case 'i':
	id = optarg;
	break;
      case 'h':
	host = optarg;
	break;
      case 'l':
	log_file = fopen(optarg, "w");
	break;
      default:
	perror("ERROR: Unrecognized argument");
	exit(1);
      }
  }
  
  //Store port number (non-switch parameter, last argument passed)
  port = atoi(argv[argc - 1]);
  
  //Setup client-side socket
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(socket_fd < 0) {
    perror("ERROR: Failed to open socket");
    exit(2);
  }  
  server = gethostbyname(host);
  if(server == NULL) {
    perror("ERROR: Invalid host address");
    exit(1);
  }
  
  //Establish tcp connection(lab1b)
  serverIP.sin_family = AF_INET;
  memcpy((char *)&serverIP.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  serverIP.sin_port = htons(port);
  if(connect(socket_fd, (struct sockaddr*) &serverIP, sizeof(serverIP)) < 0) {
    perror("ERROR: Failed to connect to server");
    exit(2);
  }
  
  //Immediately send and log an ID terminated with a newline
  dprintf(socket_fd, "ID=%s\n", id);
    
  //Initialize temperature sensor(A0) and button(D3)
  temp_sensor = mraa_aio_init(0);
  button = mraa_gpio_init(3);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
    
  //Initialize poll 
  struct pollfd pollfdArray[1];
  pollfdArray[0].fd = socket_fd; 
  pollfdArray[0].events = POLLIN | POLLHUP | POLLERR;
  
  while(1) {
    //Read temperature sensor
    raw_temperature = mraa_aio_read(temp_sensor);
    double temperature = convert_temperature(raw_temperature, scale);
    
    //Read current time
    time(&timer);
    timeInfo = localtime(&timer);
    strftime(time_print, 10, "%H:%M:%S", timeInfo);
    
    //Write data to server over socket and update log file 
    dprintf(socket_fd, "%s %.1f\n", time_print, temperature);
    if(log_file) {fprintf(log_file, "%s %.1f\n", time_print, temperature);}
    fflush(log_file);

    //While in between period intervals, check for command inputs from server
    time(&start);
    time(&end);
    while(difftime(end, start) < period) {
      //Check for button signal
      if(mraa_gpio_read(button)) {
	time_t local_time;
	char time_print[10];

	//Read and format current local time
	time(&local_time);
	timeInfo = localtime(&local_time);
	strftime(time_print, 10, "%H:%M:%S", timeInfo);

	//Write local time to server and log file
	dprintf(socket_fd, "%s SHUTDOWN\n", time_print);
	if(log_file) {fprintf(log_file, "%s SHUTDOWN\n", time_print);}
	exit(0);
      }
      
      //Poll for command inputs from server
      int poll_rc = poll(pollfdArray, 1, 0);
      if(poll_rc < 0) {
	perror("ERROR: Could not poll from socket_fd");
	exit(2);
      }
      
      //Process command inputs
      if((pollfdArray[0].revents & POLLIN)) {
	char commands_buffer[1024], c;
	int buffer_offset = 0;
		
	while(1) {
	  if(read(socket_fd, &c, 1) > 0) {
	    //Received nextline from server, split parameter in commands buffer
	    if(c == '\n') {
	      commands_buffer[buffer_offset] = '\0';
	      buffer_offset = 0;
	      break;
	    }
	    //Otherwise just add input to buffer 
	    commands_buffer[buffer_offset] = c;
	    buffer_offset++;
	  }
	}
	command_handler(commands_buffer);
      }
      //Update current time
      if(start_flag) {time(&end);}
    }
  }
  exit(0);
}


//---------------FUNCTION IMPLEMENTATIONS----------------//
double convert_temperature(int raw_temperature, char scale) {
  double temp = 100000.0 * (1023.0 / ((double)raw_temperature) - 1.0);
  double temp_celsius = 1.0 / (log(temp/100000.0) / B + 1/298.15) - 273.15;
  
  if(scale == 'C') {return temp_celsius;}
  return temp_celsius * 9/5 + 32;
}

void command_handler(const char* command) {
  if(strcmp(command, "OFF") == 0) {
    if(log_file) {fprintf(log_file, "OFF\n");}
    fflush(log_file);
   
    time_t local_time;
    char time_print[10];

    //Read and format current local time
    time(&local_time);
    timeInfo = localtime(&local_time);
    strftime(time_print, 10, "%H:%M:%S", timeInfo);
    
    //Write local time to server and log file
    dprintf(socket_fd, "%s SHUTDOWN\n", time_print);
    if(log_file) {fprintf(log_file, "%s SHUTDOWN\n", time_print);}
    exit(0);
  }  
  else if(strcmp(command, "STOP") == 0) {
    start_flag = 0;
    if(log_file) {fprintf(log_file, "%s\n", command);}
    fflush(log_file);
  }
  else if(strcmp(command, "START") == 0) {
    start_flag = 1;
    if(log_file) {fprintf(log_file, "%s\n", command);}
    fflush(log_file);
  }
  else if(strcmp(command, "SCALE=F") == 0) {
    scale = 'F';
    if(log_file) {fprintf(log_file, "%s\n", command);}
    fflush(log_file);
  }
  else if(strcmp(command, "SCALE=C") == 0) {
    scale = 'C';
    if(log_file) {fprintf(log_file, "%s\n", command);}
    fflush(log_file);
  }
  else {
    char prefix[] = "PERIOD=";
    int k = 0;
    int match = 1;

    //Invalid command
    if(strlen(command) <= strlen(prefix)) {
      if(log_file) {
	fprintf(log_file, "%s\n", command);
	fprintf(log_file, "ERROR: Unrecognized command\n");
      }
      fflush(log_file);
      exit(2);
    }
    
    while(prefix[k] != '\0' && command[k] != '\0') {
      if(prefix[k] != command[k]) {match = 0;}
      k++;
    }
    //Invalid command
    if(!match) {
      if(log_file) {
	fprintf(log_file, "%s\n", command);
	fprintf(log_file, "ERROR: Unrecognized command\n");
      }
      fflush(log_file);
      exit(2);
    }
    while(command[k] != '\0') {
      //Invalid command format
      if(!isdigit(command[k])) {
	if(log_file) {
	  fprintf(log_file, "%s\n", command);
	  fprintf(log_file, "ERROR: Unrecognized command format\n");
	}
	fflush(log_file);
	exit(2);
      }
      k++;
    }
    period = atoi(&command[7]);
    if(log_file) {fprintf(log_file, "%s\n", command);}
    fflush(log_file);
  }
}
