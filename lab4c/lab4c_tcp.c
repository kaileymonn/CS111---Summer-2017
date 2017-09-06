//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<string.h>
#include<mraa.h>
#include<mraa/aio.h>
#include<sys/time.h>
#include<time.h>
#include<math.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<poll.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>

// GLOBAL VARIABLES
const int B = 4275; // value of thermistor
int period = 1;
char scale = 'F';
int run = 1;

// option variables
FILE *logFile = NULL;
int port;
char *id;

// host structures
struct hostent *server;
char *host = "";

// socket structures
struct sockaddr_in serverAddress;
int socketFD;

double getTemperature(int rawTemperature, char scale) {
  double temp = 1023.0 / ((double)rawTemperature) - 1.0;
  temp = 100000.0 * temp;
  double celsius = 1.0 / (log(temp/100000.0) / B + 1/298.15) - 273.15;

  if (scale == 'C')
    return celsius;
  return celsius * 9/5 + 32; // farenheit
}

void printCommand(const char* command) {
  if (logFile) fprintf(logFile, "%s\n", command);
  fflush(logFile);
}

void handleShutdown(FILE *logFile) {
  time_t localTimer;
  char timeString[10];
  struct tm* localTimeInfo;

  // get current time
  time(&localTimer);
  localTimeInfo = localtime(&localTimer);
  strftime(timeString, 10, "%H:%M:%S", localTimeInfo);

  // print log and exit
  dprintf(socketFD, "%s SHUTDOWN\n", timeString);
  if (logFile) {
    fprintf(logFile, "%s SHUTDOWN\n", timeString);
  }
  exit(0);
}

void handleStartStop(const int newValue, const char* command) {
  run = newValue;
  printCommand(command);
}

void handleScale(char newScale, const char* command) {
  scale = newScale;
  printCommand(command);
}

void handlePeriod(int newPeriod, const char* command) {
  printCommand(command);
  period = newPeriod;
}

void handleInvalidCommand(const char* command) {
  printCommand(command);
  printCommand("error: invalid command");
  exit(1);
}

void handleCommand(const char* command) {
  int newPeriod;
  if (strcmp(command, "OFF") == 0) {
    printCommand("OFF");
    handleShutdown(logFile);
  }
  else if (strcmp(command, "STOP") == 0) handleStartStop(0, command);
  else if (strcmp(command, "START") == 0) handleStartStop(1, command);
  else if (strcmp(command, "SCALE=F") == 0) handleScale('F', command);
  else if (strcmp(command, "SCALE=C") == 0) handleScale('C', command);
  else {
    char prefix[] = "PERIOD=";
    int k = 0;
    int match = 1;
    if (strlen(command) <= strlen(prefix)) handleInvalidCommand(command);
    while (prefix[k] != '\0' && command[k] != '\0') {
      if (prefix[k] != command[k])
	match = 0;
      k++;
    }
    if (!match) {
      handleInvalidCommand(command);
    }
    while (command[k] != '\0') {
      if (!isdigit(command[k])) {
	handleInvalidCommand(command);
      }
      k++;
    }
    handlePeriod(atoi(&command[7]), command);
  }
}

int main(int argc, char **argv) {
  int option = 0; // used to hold option

  // arguments this program supports
  static struct option options[] = {
    {"id", required_argument, 0, 'i'},
    {"host", required_argument, 0, 'h'},
    {"log", required_argument, 0, 'l'}
  };

  // iterate through options
  while ((option = getopt_long(argc, argv, "l:i:h", options, NULL)) != -1) {
    switch (option) {
    case 'l':
      logFile = fopen(optarg, "w"); break;
    case 'i':
      id = optarg; break;
    case 'h':
      host = optarg; break;
    default:
      fprintf(stderr, "error: unrecognized argument\n");
      exit(1);
    }
  }

  // get port number (guaratneed to be last argument)
  port = atoi(argv[argc - 1]);

  // create a socket and find host
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) {
    fprintf(stderr, "error: error in opening socket\n");
    exit(1);
  }
  server = gethostbyname(host);
  if (server == NULL) {
    fprintf(stderr, "error: error in finding host\n");
    exit(1);
  }

  // initialize connection
  serverAddress.sin_family = AF_INET; // set address family
  memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)server->h_addr, server->h_length); // get ip address of server
  serverAddress.sin_port = htons(port); // store port number
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    fprintf(stderr, "error: error in connecting to server\n");
    exit(1);
  }

  // print ID
  dprintf(socketFD, "ID=%s\n", id);

  // initialize temperature sensor at A0
  mraa_aio_context temperatureSensor;
  temperatureSensor = mraa_aio_init(0);

  // initialize button at D3
  mraa_gpio_context button;
  button = mraa_gpio_init(3);
  mraa_gpio_dir(button, MRAA_GPIO_IN);

  // to hold raw temperature read by temperature sensor
  int rawTemperature = 0;

  // initialize time variables
  time_t timer, start, end;
  char timeString[10];
  struct tm* timeInfo;

  // initialize poll structures
  struct pollfd pollfdArray[1];
  pollfdArray[0].fd = socketFD; //STDIN_FILENO; // polls from stdin
  pollfdArray[0].events = POLLIN | POLLHUP | POLLERR;

  while (1) {
    // get temperature
    rawTemperature = mraa_aio_read(temperatureSensor);
    double processedTemperature = getTemperature(rawTemperature, scale);

    // get current time
    time(&timer);
    timeInfo = localtime(&timer);
    strftime(timeString, 10, "%H:%M:%S", timeInfo);

    // print to socket and log file
    dprintf(socketFD, "%s %.1f\n", timeString, processedTemperature);
    if (logFile) {
      fprintf(logFile, "%s %.1f\n", timeString, processedTemperature);
    }
    fflush(logFile); // flush out buffer to make sure log file is written

    time(&start); // start the timer
    time(&end);// sample ending time
    while (difftime(end, start) < period) {
      // read from button and shutdown if needed
      if (mraa_gpio_read(button))
	handleShutdown(logFile);

      // poll for input
      int returnValue = poll(pollfdArray, 1, 0);
      if (returnValue < 0) {
	fprintf(stderr, "error: error while polling\n");
	exit(1);
      }

      // if there's input, handle it
      if ((pollfdArray[0].revents & POLLIN)) {
	FILE* fdf = fdopen(socketFD, "r");
	char commBuff[1024];
	char c;
	int buffIndex = 0;
	while (1) {
	  if(read(socketFD, &c, 1) > 0) {
	    if (c == '\n') {
	      commBuff[buffIndex] = '\0';
	      buffIndex = 0;
	      break;
	    }
	    commBuff[buffIndex] = c;
	    buffIndex++;
	  }
	}
	handleCommand(commBuff);
      }
      
      if (run) time(&end); // sample new ending time
    }
  }

  exit(0);
}





