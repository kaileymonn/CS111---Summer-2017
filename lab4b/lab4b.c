//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<poll.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<mraa.h>
#include<mraa/aio.h>
#include<math.h>
#include<time.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<ctype.h>

//------------------GLOBAL VARIABLES----------------//


//Option argument flags, default values
int period_flag = 1; 
char scale_flag = 'F';
FILE* log_fd = NULL;

//Timer variables
struct tm* time_info;
time_t timer, start_time, end_time;
char print_time[10];

//Poll variables
char POLL_BUFFER[100];
char PERIOD_PREFIX[] = "PERIOD=";
int poll_flag = 1; //STOP:0, START:1 by default
int valid_period = 1; //True(valid command received) by default

//Temperature variables
const int B = 4275; //B value of thermistor
int single_digit_flag = 0; //False by default


//----------------ADDITIONAL FUNCTIONS--------------//


double get_temp(int temp_raw, char scale);

void button_handler(FILE* log_fd);

void poll_handler(const char* command);


//--------------------MAIN ROUTINE------------------//

int main(int argc, char **argv) {

  static struct option long_opts[] = {
    {"period", required_argument, 0, 'p'},
    {"scale", required_argument, 0, 's'},
    {"log", required_argument, 0, 'l'}
  };

  int opt = 0;

  while ((opt = getopt_long(argc, argv, "p:s:l", long_opts, NULL)) != -1) {
    switch (opt)
      {
      case 'p':
	period_flag = atoi(optarg);
	break;
      case 's':
	if((optarg[0] == 'C' || optarg[0] == 'F') && (strlen(optarg) == 1))
	  scale_flag = optarg[0];
	else {
	  perror("ERROR: Unrecognized 'scale' argument");
	  exit(1);
	}
	break;
      case 'l':
	//Create log file, now we can use fopen yay
	log_fd = fopen(optarg, "w");
	if(log_fd == NULL) {
	  perror("ERROR: Failed to create log file");
	  exit(1);
	}
	break;
      default:
	perror("ERROR: Unrecognized option");
	exit(1);
      }
  }

  //Initialize temperature sensor
  int temp_raw = 0;
  mraa_aio_context temp_sensor;
  temp_sensor = mraa_aio_init(0);
  if(temp_sensor == NULL) {
    perror("ERROR: Failed to initialize temperature sensor");
    exit(2);
  }
  
  //Initialize button
  mraa_gpio_context button;
  button = mraa_gpio_init(3);
  mraa_gpio_dir(button, MRAA_GPIO_IN);

  //Initialize poll
  struct pollfd poll_fd[1];
  poll_fd[0].fd = STDIN_FILENO;
  poll_fd[0].events = POLLIN | POLLHUP | POLLERR;
  
  while(1) {
    //Read temperature from sensor and format raw reading
    temp_raw = mraa_aio_read(temp_sensor);
    double temp_good = get_temp(temp_raw, scale_flag);

    //Read present time and load to print_time variable
    time(&timer);
    time_info = localtime(&timer);
    strftime(print_time, 10, "%H:%M:%S", time_info);
    
    //Print temperature readings to STDOUT and log
    if(!single_digit_flag) {
      fprintf(stdout, "%s %.1f\n", print_time, temp_good);
      if(log_fd) {fprintf(log_fd, "%s %.1f\n", print_time, temp_good);}
      fflush(log_fd); //Flush fopen() buffer, ensure complete write of buffer contents
    }
    else {
      fprintf(stdout, "%s 0%.1f\n", print_time, temp_good);
      if(log_fd) {fprintf(log_fd, "%s 0%.1f\n", print_time, temp_good);}
      fflush(log_fd);
    }
    
    //Spin for duration of period and poll from STDIN 
    time(&start_time);
    time(&end_time);

    while(difftime(end_time, start_time) < period_flag) {
      //Check for button hits
      if(mraa_gpio_read(button)) {button_handler(log_fd);}
      
      int poll_ret = poll(poll_fd, 1, 0);
      if(poll_ret < 0) {
	perror("ERROR: Failed to poll from stdin");
	exit(1);
      }

      //Process received commands stored in buffer
      if((poll_fd[0].revents & POLLIN)) {
	scanf("%s", POLL_BUFFER);
	poll_handler(POLL_BUFFER);
      }

      //Update current time if we're still polling
      if(poll_flag) {time(&end_time);} 
    }
  }
  
  exit(0);
}


//--------------FUNCTION IMPLEMENTATIONS------------//


double get_temp(int temp_raw, char scale) {
  double R = 1023.0 / ((double)temp_raw) - 1.0;
  R = 100000 * R;

  //Convert to temperature(Celsius) via datasheet
  double temp_cel = 1.0 / (log(R/100000.0) / B + 1/298.15) - 273.15; 
  if(scale == 'C') {
    if(temp_cel < 10) {single_digit_flag = 1;}
    return temp_cel;
  }

  //Return default farenheit reading
  double temp_far = temp_cel * 1.8 + 32;
  return temp_far; 
}

void button_handler(FILE* log_fd_local) {
  //Read current time and load into local_time variable
  time(&timer);
  time_info = localtime(&timer);
  strftime(print_time, 10, "%H:%M:%S", time_info);

  //Print SHUTDOWN and exit
  if(log_fd_local) {fprintf(log_fd_local, "%s SHUTDOWN\n", print_time);}
  exit(0);
}

void poll_handler(const char* command) {
  //OFF
  if(strcmp(command, "OFF") == 0) {
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);

    //Execute routine for button press
    button_handler(log_fd);
  }

  //START
  else if(strcmp(command, "START") == 0) {
    poll_flag = 1;
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);
  }

  //STOP
  else if(strcmp(command, "STOP") == 0) {
    poll_flag = 0;
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);
  }

  //SCALE=F  
  else if(strcmp(command, "SCALE=F") == 0) {
    scale_flag = 'F';
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);
  }

  //SCALE=C
  else if(strcmp(command, "SCALE=C") == 0) {
    scale_flag = 'C';
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);
  }

  //PERIOD=#
  else {
    //Check length of received command
    if(strlen(command) <= strlen(PERIOD_PREFIX)) {
      if(log_fd) {fprintf(log_fd, "%s\n", command);}
      fflush(log_fd);
      perror("ERROR: Invalid command length");      
      exit(1);
    }

    //Check if received command matches prefix
    int k = 0;
    while(PERIOD_PREFIX[k] != '\0' && command[k] != '\0') {
      if(PERIOD_PREFIX[k] != command[k]) {valid_period = 0;}
      k++;
    }

    if(!valid_period) {
      if(log_fd) {fprintf(log_fd, "%s\n", command);}
      fflush(log_fd);
      perror("ERROR: Invalid command received");      
      exit(1);
    }

    //Check if argument is an integer
    while(command[k] != '\0') {
      if(!isdigit(command[k])) {
	if(log_fd) {fprintf(log_fd, "%s\n", command);}
	fflush(log_fd);
	perror("ERROR: Invalid command argument received");      
	exit(1);
      }
      k++;
    }

    //Set new period
    period_flag = atoi(&command[7]);
    if(log_fd) {fprintf(log_fd, "%s\n", command);}
    fflush(log_fd);
  }
}
