/* Sophia Fondell
webserv.c
A simple webserver written in C
Conforming to HTTP 1.1x
*/

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

static int const  NOTFOUND = 404;
static int const  NOTIMPL = 501;

// Show error messages
void err_case(int code, int client){
    char *buf = malloc(512);
    
    //standardized messages found here https://developers.google.com/webmaster-tools/search-console-api-original/v3/errors
    switch(code){
        case NOTFOUND:
            //404 case
            //with the stupid markup stuff
            sprintf(buf, "HTTP/1.1 404 Not Found\n");
            write(client, buf, strlen(buf));
            sprintf(buf, "Content-Type: text/html; charset=UTF-8\n\n");
            write(client, buf, strlen(buf));
            sprintf(buf, "<html>\n<head>\n<title>404 Not Found</title>\n</head>\r\n");
            write(client, buf, strlen(buf));
            sprintf(buf, "<body>\n<p>The specified URL could not be located on our server. </p>\r\n</body></html>\r\n");
            write(client, buf, strlen(buf));
            break;
        case NOTIMPL:
            sprintf(buf, "HTTP/1.1 501 Not Implemented\r\n");
            write(client, buf, strlen(buf));
            sprintf(buf, "<html><head>\n<title>501 Not Implemented</title></head>\r\n");
            write(client, buf, strlen(buf));
            sprintf(buf, "<body>The requested operation has not been implemented. \r\n</body></html>\r\n");
            write(client, buf, strlen(buf));
            break;
            
        default:
            sprintf(stderr, "Error finding error message lol\n");
            break;
    }
    free(buf);
    close(client);
    exit(0);           
}

// Prints header for HTML file
void show_html(int client) {
  char *buf = malloc(512);
  sprintf(buf, "HTTP/1.1 200 OK\n");
  write(client, buf, strlen(buf));
  sprintf(buf, "Content Type: text/html; charset=UTF-8\n\n");
  write(client, buf, strlen(buf));
  free(buf);
}

// Prints header for image (type = 0 for jpg/jpeg and 1 for gif)
void show_img(int client, int type, unsigned long fsize) {
  char *buf = malloc(512);
  sprintf(buf, "HTTP/1.1 200 OK\n");
  write(client, buf, strlen(buf));
  // image is jpg/jpeg
  if (!type) {
    sprintf(buf, "Content-Type:image/jpeg\r\n");
  }
  // image is gif
  else { 
    sprintf(buf, "Content-Type:image/gif\r\n");
  } 
  write(client, buf, strlen(buf));
  sprintf(buf, "Content-Length: %li\r\n\r\n", fsize);
  write(client, buf, strlen(buf));
  free(buf);
}

// Prints header for playing audio file
void play_audio(int client, unsigned long fsize) {
	char *buf = malloc(512);
	sprintf(buf, "HTTP/1.1 200 OK\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Length: %li\r\n", fsize);
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Type: audio/mpeg\r\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Disposition: inline\r\n\r\n");
	write(client, buf, strlen(buf));
	free(buf);
}

// VIDEO FILE 200 OK HEADER
void play_video(int client, unsigned long fsize) {
	char *buf = malloc(512);
	sprintf(buf, "HTTP/1.1 200 OK\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Length: %li\r\n", fsize);
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Type: video/mp4\r\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Disposition: inline\r\n\r\n");
	write(client, buf, strlen(buf));
	free(buf);
}

// VIDEO FILE 206 PARTIAL CONTENT HEADER
void part_video(int client, unsigned long start, unsigned long end, unsigned long fsize) {
	char *buf = malloc(512);
	sprintf(buf, "HTTP/1.1 206 Partial Content\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Range: bytes %li-%li/%li\r\n", start, end, fsize);
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Length: %li\r\n", (end - start + 1));
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Type: video/mp4\r\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-Disposition: inline\r\n\r\n");
	write(client, buf, strlen(buf));
	free(buf);
}
	
// Prints header for showing directory listings
void show_text(int client) {
	char *buf = malloc(512);
	sprintf(buf, "HTTP/1.1 200 OK\r\n");
	write(client, buf, strlen(buf));
	sprintf(buf, "Content-type: text/plain; charset=UTF-8\r\n\r\n");
	write(client, buf, strlen(buf));
	free(buf);
}

// Extract the request from the HTTP GET string
char* getRequest(char* data) {
  char* token;
  token = strtok(data, " ");

  while (token != NULL) {
    if (token[0] == '/') {
      return token;
    }
    token = strtok(NULL, " ");
  }
}

// Extract file extension from the HTTP GET string
char* ftype(char* f){
  char *rest;
  if (rest = strrchr(f, '.' )){
    return rest++;
  }
  return "";
}

// CGI file
int isCGI(char *f){
  return (strcmp(ftype(f), ".cgi") == 0);
}
// HTML file
int isHTML(char *f){
  return (strcmp(ftype(f), ".html") == 0);
}
// GIF
int isGIF(char *f){
  return (strcmp(ftype(f), ".gif") == 0);
}
// JPEG
int isJPG(char *f){
  return ((strcmp(ftype(f), ".jpg") == 0) || (strcmp(ftype(f), ".jpeg") == 0));
}
// txt file
// Works for any sort of plaintext or code file
int isTXT(char *f){
  return (strcmp(ftype(f), ".txt") == 0) ||
  (strcmp(ftype(f), ".c") == 0) ||
  (strcmp(ftype(f), ".h") == 0) ||
  (strcmp(ftype(f), ".py") == 0) ||
  (strcmp(ftype(f), ".java") == 0);
}
// mp3 audio file
int isMP3(char *f){
	return (strcmp(ftype(f), ".mp3") == 0);
}
// mp4 video
int isMP4(char *f){
	return (strcmp(ftype(f), ".mp4") == 0);
}

void servConn(int port) {

  int sd, new_sd;
  struct sockaddr_in name, cli_name;
  int sock_opt_val = 1;
  int cli_len;
  // recieve data buffer
  char data[4096];
  
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(servConn): socket() error");
    exit (-1);
  }

  if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
		  sizeof(sock_opt_val)) < 0) {
    perror ("(servConn): Failed to set SO_REUSEADDR on INET socket");
    exit (-1);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if (bind(sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror ("(servConn): bind() error");
    exit (-1);
  }

  listen (sd, 5);

  for(;;) {
      cli_len = sizeof(cli_name);
      new_sd = accept(sd, (struct sockaddr *) &cli_name, &cli_len);
      printf("Assigning new socket descriptor:  %d\n", new_sd);
      
      // Negative socket descriptor returned which means an error occurred
      if (new_sd < 0) {
	     perror("(servConn): accept() error");
	     exit(-1);
      }

      if (fork () == 0) {	/* Child process. */
	     close (sd);
         // will be HTML GET etc.
         // Or just read string from client program
	     read(new_sd, &data, 4096);
         printf("Received string = %s\n", data);

         // Get requested file or dir from GET request
         char* tok = getRequest(data);
         printf("Extracted token: %s\n", tok);

         // Use stat to figure out what token is
         struct stat statbuf;
         int ret = stat(tok, &statbuf);

         // Check to see if token is invalid
         printf("ret: %d\n", ret);
         printf("is reg: %d\n", S_ISREG(statbuf.st_mode));
         printf("is dir: %d\n", S_ISDIR(statbuf.st_mode));
         printf("is cgi: %d\n", isCGI(tok));
         printf("type: %s\n", ftype(tok));

         // Error handling, stat returned -1 so the file does not exist
         if (ret == -1) {
          err_case(NOTFOUND, new_sd);
         }

         // A directory's contents are being requested
         if (S_ISDIR(statbuf.st_mode) == 1) {
          // Concat cmd and dir name and call ls -l
          char buf[4096];
          char lscmd[4096] = "ls -l ";
          strcat(lscmd, tok);
          FILE *out = popen(lscmd, "r");
          show_text(new_sd);

          // Write directory contents to socket fd
          int lines = 0;
          while(fgets(buf, 4096, out)) {
          	printf("%s\n", buf);
            lines++;
            write(new_sd, buf, strlen(buf));
          }

          // Print out footer response (also in case no files in dir)
          char eodbuf[4096];
          if (lines > 1) {
            lines = lines - 1;
          }
          sprintf(eodbuf, "End of directory listing. %d files listed.", lines);
          printf("%s\n", eodbuf);
          write(new_sd, eodbuf, strlen(eodbuf));

          // Close stdout pipe of command
          pclose(out);
         }

         // A specific file is being requested
         else if (S_ISREG(statbuf.st_mode) == 1) {

          // Token is an HTML file
          if (isHTML(tok)) {
            show_html(new_sd);
            char linebuf[4096];
            FILE *ptr = fopen(tok, "r");
            while(fgets(linebuf, sizeof(linebuf), ptr) != NULL) {
              printf("%s\n", linebuf);
              write(new_sd, linebuf, strlen(linebuf));
            }
            fclose(ptr);
          }

          // Token is jpg/jpeg/gif
          else if (isJPG(tok) || isGIF(tok)) {
            FILE *ptr = fopen(tok, "rb");
            fseek(ptr, 0, SEEK_END);
            unsigned long fsize = ftell(ptr);
            unsigned char *rawbuf = (unsigned char *) malloc(fsize + 1);
            fseek(ptr, 0, SEEK_SET);
            fread(rawbuf, fsize, 1, ptr);
            // JPG
            if (isJPG(tok)) {
              show_img(new_sd, 0, fsize);
            }
            // GIF
            else {
              show_img(new_sd, 1, fsize);
            }
            write(new_sd, rawbuf, fsize);
            fclose(ptr);
          }

          // Token is an mp3 audio file
          else if (isMP3(tok)) {
          	FILE *ptr = fopen(tok, "rb");
          	fseek(ptr, 0, SEEK_END);
          	unsigned long fsize = ftell(ptr);
          	unsigned char *rawbuf = (unsigned char *) malloc(fsize + 1);
          	fseek(ptr, 0, SEEK_SET);
          	fread(rawbuf, fsize, 1, ptr);
          	play_audio(new_sd, fsize);
          	write(new_sd, rawbuf, fsize);
          	fclose(ptr);
          }

          /*// NEED TO CHECK FOR CONTENT RANGE VAL IN GET REQUEST
          else if (isMP4(tok)) {
          	// Declare content range string for strstr()
          	const char *range = "Range";

          	// Set up file pointer to get fsize
          	FILE *ptr = fopen(tok, "rb");
          	fseek(ptr, 0, SEEK_END);
          	unsigned long fsize = ftell(ptr);
          	unsigned char *rawbuf = (unsigned char *) malloc(fsize + 1); // does buf need to be this big? may not matter

          	// Declare preliminary start and end vars (change depending on req)
          	unsigned long start = 0;
          	unsigned long end = fsize - 1;

          	// If a content range is requested in HTTP GET request:
          	if (strstr(data, substr)) {
          		// SET START AND END VARS HERE BY INCREMENTING PTR AND CONVERT TO UINT
          		fseek(ptr, start, SEEK_SET);
          		fread(rawbuf, (end - start), 1, ptr);
          		part_video(new_sd, fsize);
          		write(new_sd, rawbuf, fsize);
          		fclose(ptr);
          	}
          	// Content range is not requested in HTTP GET request:
          	else {
          		fseek(ptr, 0, SEEK_SET);
          		fread(rawbuf, fsize, 1, ptr);
          		play_video(new_sd, fsize);
          		write(new_sd, rawbuf, fsize);
          		fclose(ptr);
          	}
          }*/

          // token is an mp4 video file (needs to be fixed, hangs forever)
          else if (isMP4(tok)) {
          	FILE *ptr = fopen(tok, "rb");
          	fseek(ptr, 0, SEEK_END);
          	unsigned long fsize = ftell(ptr);
          	printf("fsize: %li\n", fsize);
          	unsigned char *rawbuf = (unsigned char *) malloc(fsize + 1);
          	fseek(ptr, 0, SEEK_SET);
          	fread(rawbuf, fsize, 1, ptr);
          	play_video(new_sd, fsize);
          	write(new_sd, rawbuf, fsize);
          	fclose(ptr);
          }

          // Token is a cgi file
          else if (isCGI(tok)) {
            char buf[4096];
            char shcmd[4096] = "sh ";
            strcat(shcmd, tok);
            FILE *out = popen(shcmd, "r");
            show_text(new_sd);

            // Write results of shellscript to socket fd
            while(fgets(buf, 4096, out) != NULL) {
              printf("%s\n", buf);
              write(new_sd, buf, strlen(buf));
              printf("henlo\n");
            }
            pclose(out);
          }

          // Token is a txt file
          else if (isTXT(tok)) {
          	char linebuf[4096];
          	show_text(new_sd);
          	FILE *ptr = fopen(tok, "r");
          	while (fgets(linebuf, sizeof(linebuf), ptr) != NULL) {
          		printf("%s\n", linebuf);
          		write(new_sd, linebuf, strlen(linebuf));
          	}
          	fclose(ptr);
          }

          // None of the above cases so the file type has not been implemented
          else {
            err_case(NOTIMPL, new_sd);
          }
         }

         // None of the above cases, requested token type has not been implemented
         else {
         	err_case(NOTIMPL, new_sd);
         }
	     exit(0);
      }
  }
}


int main() {
  // Port number we are running on
  servConn(5050);
  return 0;
}
