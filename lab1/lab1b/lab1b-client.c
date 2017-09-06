//NAME: Kai Wong
//EMAIL: kaileymon@g.ucla.edu
//ID: 704451679

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include <mcrypt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>



//----------------------GLOBALS----------------------//

//Terminal modes
struct termios tattr;
struct termios new_tattr;

//Initialize option flags to 0 (false)
int encrypt_flag = 0;
int log_flag = 0;

//Character reads (constants)
const char EOT = 0x4; // ctrl+D
const char LF = 0xA; // '\n'
const char CR = 0xD; // '\r'

//Client/Server stuff
int port_num = 0;
int sockfd = 0;
int pid = 1; //Let server pid = 1
struct sockaddr_in serv_addr;
struct hostent *server;

//Encryption stuff
MCRYPT td;
char *key;
char *IV;
int keyfd;
const int KEYSIZE = 16; 

//Logging stuff
int logfd = 0;
const char SENT_MSG[] = "SENT 1 bytes: ";
const int SENT_MSG_LEN = 14;
const char REC_MSG[] = "RECEIVED 1 bytes: ";
const int REC_MSG_LEN = 18;



//---------------ADDITIONAL FUNCTIONS----------------//

//Resets terminal mode to original
void reset();

//Initialize a new terminal mode
void initTerm();

//Handles terminal reads and respective writes
void readWrite();

//Thread: Write socket output (server output) to stdout
void *tfunc(void *message); 

//Generate key for TWOFISH encryption algorithm in CFB mode
void crypto();

//Signal handler
void sig_handler(int sig);



//-----------------MAIN ROUTINE---------------------//

int main(int argc, char **argv) {
  
  int opt = 0;
  
  static struct option long_options[] = {
    {"port", required_argument, NULL, 'p'},
    {"encrypt", no_argument, NULL, 'e'},
    {"log", required_argument, NULL, 'l'}
  };
  
  //Parse through arguments
  while((opt = getopt_long(argc, argv, "p:e:l", long_options, NULL)) != -1) {
    switch(opt)
      {
      case 'p':
	port_num = atoi(optarg);
	break;
      case 'e':
	encrypt_flag = 1;
	break;
      case 'l':
	log_flag = 1;
	//Open specified logfile for writing, then dup() logfd
	logfd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if((dup(logfd)) < 0) {
	  perror("Error: Failed to initialize log");
	  exit(1);
	}
	break;      
      default: 
	perror("Error: Unrecognized option");
	exit(1);	
      }
  }
  
  //Initialize new terminal mode
  initTerm();

  //Socket initialization for client
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error: Could not create socket");
    exit(1);
  } 
  printf("Client socket successfully created...\n");
  
  //Server address initializations
  server = gethostbyname("localhost");
  memset((char*) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *) &serv_addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
  serv_addr.sin_port = htons(port_num);


  //Socket Connect
  printf("Client is attempting to connect...\n");
  if((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
    perror("Error");
    exit(1);
  } 

  //Encryption enabled
  if(encrypt_flag == 1) {crypto();}

  //readWrite() from server to socket, socket to stdout
  pthread_t tid;
  pthread_create(&tid, NULL, &tfunc, &sockfd); 
  
  readWrite();
  
  //Close mcrypt module if encryption was used
  if(encrypt_flag == 1) {
    mcrypt_generic_deinit(td);
    mcrypt_module_close(td);
  }

  exit(0);
}






//------------------FUNCTION IMPLEMENTATIONS----------------------//

void reset() {
  tcsetattr(STDIN_FILENO, TCSANOW, &tattr);
  printf("Reset terminal mode...\n");
}

void initTerm() {
  //Check STDIN_FILENO 
  if(!isatty(STDIN_FILENO)) {
    perror("Error: Invalid terminal (STDIN_FILENO)");
    exit(1);
  }
  
  //Save current terminal mode, reset before shutdown, setup new terminal mode (non-canonical, no echo)
  tcgetattr(STDIN_FILENO, &tattr);
  tcgetattr(STDIN_FILENO, &new_tattr);
  new_tattr.c_lflag &= ~(ICANON | ECHO);
  new_tattr.c_cc[VMIN] = 1;

  //Check if new terminal mode has been set up properly
  if(tcsetattr(STDIN_FILENO, TCSANOW, &new_tattr) < 0) {
    perror("Error: Failed to initialize new terminal mode");
    exit(1);
  }

  atexit(reset);
}


void readWrite() {
  char current;
  
  //Read from STDIN
  while(1) {
    //EOF Check
    if(read(STDIN_FILENO, &current, 1) == 0) {
      perror("Error: Read failure (EOF received)");
      close(sockfd);
      exit(1);
    }
    
    //EOT check
    if(current == EOT) {
      close(sockfd);
      exit(0);
    }
    
    //Map <cr> or <lf> to <cr><lf>
    else if(current == LF || current == CR) {
      //Echoing to the display (stdout)
      if((write(STDOUT_FILENO, &current, 1)) != 1) {
	perror("Error: Client failed to write to stdout");
      }

      //Proper CR->NL mapping
      current = LF;

      //Encryption
      if(encrypt_flag == 1) {
	if((mcrypt_generic(td, &current, 1)) != 0) {
	  perror("Error: Encryption failure during write to socket");
	  exit(1);
	}
      }
      
      //While writing to server (socket)
      if(write(sockfd, &current, 1) != 1) {
	perror("Error: Failed to write '\n' or '\r' to socket");
	exit(1);
      }
    }
    
    //Write all other characters 
    else {
      //Echoing to the display
      write(STDOUT_FILENO, &current, 1);

      //Encryption
      if(encrypt_flag == 1) {
	if((mcrypt_generic(td, &current, 1)) != 0) {
	  perror("Error: Encryption failure during write to socket");
	  exit(1);
	}
      }
      
      //While writing to server (socket)
      if(write(sockfd, &current, 1) != 1) {
	perror("Error: Failed to write generic char to server");
	exit(1);
      }
    }

    //Logging
    if(log_flag == 1) {
      if((write(logfd, &SENT_MSG, SENT_MSG_LEN)) != 1) {
	perror("Error: Failed to write SENT message to log");
	exit(1);
      }
      if((write(logfd, &current, 1)) != 1) {
	perror("Error: Failed to write from stdin to log");
	exit(1);
      }
      write(logfd, &LF, 1);
    }
  }
}

void *tfunc(void *message) {
  char current;
  int readfd = *((int*)message); //Typecast *message to an int

  while(1) {
    //EOF Check
    if(read(readfd, &current, 1) == 0) {signal(SIGPIPE, sig_handler);}

    //Logging
    if(log_flag == 1) {
      if((write(logfd, &REC_MSG, REC_MSG_LEN)) != REC_MSG_LEN) {
	perror("Error: Failed to write RECEIVED message to log");
	exit(1);
      }
      if((write(logfd, &current, 1)) != 1) {
	perror("Error: Failed to write server output to log");
	exit(1);
      }
      write(logfd, &LF, 1);

    }
    
    //Decryption
    if(encrypt_flag == 1) {
      if((mdecrypt_generic(td, &current, 1)) != 0) {
	perror("Error: Failed to decrypt server output");
	exit(1);
      }
    }

    //Write terminal output to stdout
    write(STDOUT_FILENO, &current, 1);    
  }

  return NULL;
}

void crypto() {
  //Open module
  if((td = mcrypt_module_open("twofish", NULL, "cfb", NULL)) == MCRYPT_FAILED) {
    perror("Error: Failed to initialize mcrypt module");
    exit(1);
  }

  //Read my.key file into *key buffer
  keyfd = open("my.key", O_RDONLY);
  key = calloc(1, KEYSIZE);
  if((read(keyfd, key, KEYSIZE)) != KEYSIZE) {
    perror("Error: Failed to read my.key file");
    exit(1);
  }
  
  //Close my.key file
  close(keyfd);

  //Generate key
  IV = malloc(mcrypt_enc_get_iv_size(td));
  
  //Put random data into IV
  int i;
  for(i = 0; i < mcrypt_enc_get_iv_size(td); i++) {
    IV[i]=rand();
  } 

  if((mcrypt_generic_init(td, key, KEYSIZE, IV)) < 0) {
    perror("Error: Failed to generic_init()");
    exit(1);
  }
}

void sig_handler(int sig) {
  if(sig == SIGPIPE) {
    perror("Error: SIGPIPE received from shell");
    close(sockfd);
    kill(pid, SIGTERM);
    exit(0);
  }
}
