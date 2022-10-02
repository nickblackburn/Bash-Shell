/*
	Author: Nicholas Blackburn in 2019
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

//  Declare background mode
int bgMode;
//  Declare foreground-only mode
int fgOnly = 0;
//  Declare the status initially to 0
int statusVal = 0;
//  Declare pid
pid_t pid = -1;
//  Declare pid array and count index
pid_t pidList[100];
int pidCount = 0;

//  Function - signal handler for SIGINT, changes quitFlag to exit the program cleanly
void sigHandler(int sigNum) {
	//  Signal received
	if (sigNum == SIGINT) {
		//  Print message, and end foreground process
		if (bgMode && pid > 0) {
			char bufferMsg[] = "Foreground process %i is being killed by signal %i.\n";
			char buffer[256];
			memset(buffer, '\0', sizeof(buffer));
			snprintf(buffer, 256, bufferMsg, pid, sigNum);
			write(STDOUT_FILENO, buffer, sizeof(buffer) - 1);
			fflush(stdout);
			statusVal = 2;
		}
	}
}

//  Function - signal handler for SIGTSTP, toggles foreground-only mode
void sigStpHandler(int sigNum) {
	//  Signal received
	if (sigNum == SIGTSTP) {
		//  Wait for current foreground process to finish if one is running
		if (bgMode && pid > 0) {
			waitpid(pid, &statusVal, 0);
		}
		//  Create buffer for message
		char buffer[60];
		memset(buffer, '\0', sizeof(buffer));
		//  Entering foreground-only mode
		if (!fgOnly) {
			fgOnly = 1;
			char bufferMsg[] = "Entering foreground-only mode (& is now ignored)";
			snprintf(buffer, 60, "%s", bufferMsg);
		}
		//  Exiting foreground-only mode
		else {
			fgOnly = 0;
			char bufferMsg[] = "Exiting foreground-only mode";
			snprintf(buffer, 60, "%s", bufferMsg);
		}
		//  Write foreground-only message buffer to output
		write(STDOUT_FILENO, buffer, sizeof(buffer) - 1);
		fflush(stdout);
		statusVal = 15;
	}
}

//  Main Function
int main(int argc, char* argv[]) {
	//  Signal handler info for SIGINT
	struct sigaction sigIntInfo;
	memset(&sigIntInfo, '\0', sizeof(sigIntInfo));
	sigIntInfo.sa_flags = 0;
	sigfillset(&(sigIntInfo.sa_mask));
	sigIntInfo.sa_handler = sigHandler;
	sigaction(SIGINT, &sigIntInfo, NULL);

	//  Signal handler info for SIGTSTP
	struct sigaction sigStpInfo;
	memset(&sigStpInfo, '\0', sizeof(sigStpInfo));
	sigStpInfo.sa_flags = 0;
	sigfillset(&(sigStpInfo.sa_mask));
	sigStpInfo.sa_handler = sigStpHandler;
	sigaction(SIGTSTP, &sigStpInfo, NULL);

	//  Loop until user exit
	while (1) {
		//  Check if any started background processes have completed
	    pid = waitpid(-1, &statusVal, WNOHANG);
		while (pid > 0) {
			//  Background process and exit status update message
            printf("Background process %i is done.\n", pid);
            printf("Exit Status - %i\n", statusVal);
	        fflush(stdout);
	        int y;
	        //  Remove pid from array
	        for (y = 0; y < pidCount; y++) {
	        	if (pid == pidList[y]) {
	        		int x;
	        		//  Shift remaining items in array
	        		for (x = y; x < pidCount - 1; x++) {
	        			pidList[x] = pidList[x + 1];
	        		}
	        		pidCount--;
	        		break;
	        	}
	        }
	        //  Check for any finished background processes
	        pid = waitpid(-1, &statusVal, WNOHANG);
        }

        //  Print terminal operator : to output
		printf(": ");
		fflush(stdout);

		//  Get a line of data from the user
		char* data = NULL;
		size_t dataLength = 0;
		ssize_t dataSize = 0;
		dataSize = getline(&data, &dataLength, stdin);

		//  Data is NULL from cntl+C signal or user pressed enter
		if (data == NULL) {
			//  free data from getline
			free(data);
			data = NULL;
			//  Print newline
			printf("\n");
			fflush(stdout);
			//  Continue program execution from start of while loop
			continue;
		}

		//  Replace last element of data from newline to null
		if (data[dataSize - 1] == '\n' && strcmp(data, "\n\0") != 0) {
			data[dataSize - 1] = '\0';
			dataSize--;
		}

		//  Data copy
		char* dataCopy = data;
		char* dataArray[600];
		int dataArrayLength = 0;
		//  File helpers
		char* fileIn = NULL;
	    char* fileOut = NULL;
	    char* nextInput = NULL;
	    //  Background mode flag
	    bgMode = 1;
	    int i;

	    //  Loop through copied data to store the data arguments
		for (i = 0; i < 600; i++) {
			//  Set pidFlag which gets changed upon a data arg of $$ being found
			int pidFlag = 1;

			//  Utilize strsep to parse data at each space delimeter
        	char* dataVal = strsep(&dataCopy, " ");
        	//  Argument is empty
        	if (dataVal == NULL) {
        		dataArray[i] = NULL;
            	break;
        	}
        	//  Copy the data val to process string expansion for $$ to pid
        	char* dataValCopy = strdup(dataVal);
        	char* dataValCopy2 = malloc(strlen(dataVal)+6);
        	//  Argument contains $$
        	if (strstr(dataVal, "$$") != NULL) {
        		int z;
        		pidFlag = 0;
        		//  Loop data and find $$ replace with %d for snprintf
        		for (z = 0; z < strlen(dataVal); z++) {
        			if (dataValCopy[z] == '$' && dataValCopy[z+1] == '$') {
        				dataValCopy[z] = '%';
        				dataValCopy[z+1] = 'd' ;
        				break;
        			}
        		}
        		//  Store actual pid and replace $$ in buffer
        		snprintf(dataValCopy2, strlen(dataVal)+6, dataValCopy, getpid());
        	}
        	//  Argument matches output >
        	if (strcmp(dataVal, ">") == 0) {
        		//  Set output file to the following argument
	    		fileOut = strsep(&dataCopy, " ");
	    		dataArray[i] = NULL;
	    		break;
	    	}
	    	//  Argument matches input <
	    	if (strcmp(dataVal, "<") == 0) {
	    		//  Set the input file to the next argument
	    		fileIn = strsep(&dataCopy, " ");
	    		//  Check for more arguments
	    		nextInput = strsep(&dataCopy, " ");
	    		//  No more arguments
	    		if (nextInput == NULL) {
	    			dataArray[i] = NULL;
	    			break;
	    		}
	    		//  Next argument is output >
	    		if (strcmp(nextInput, ">") == 0) {
	    			//  Store the file output
	    			fileOut = strsep(&dataCopy, " ");
	    		}
	    		dataArray[i] = NULL;
	    		break;
	    	}
	    	//  Background mode argument &
	    	if (strcmp(dataVal, "&") == 0) {
	    		//  Set the background mode to 0
	    		bgMode = 0;
	    		dataArray[i] = NULL;
	    		break;
	    	}
	    	//  Store the data argument in array, if pidFlag is set utilize data val copy
	    	char* dataValSafeCopy = strdup(dataValCopy2);
	    	if (pidFlag) {
	    		dataArray[i] = dataVal;
	    	}
	    	else {
	    		dataArray[i] = dataValSafeCopy;
	    	}
	    	//  Free malloc'd data copy
	    	free(dataValCopy2);
        	dataValCopy2 = NULL;
        	//  Increment the data array length
        	dataArrayLength++;
    	}

    	//  User exits shell
		if (strcmp(data, "exit") == 0) {
			//  Terminate any running processes
			int s;
			for (s = 0; s < pidCount; s++) {
				//  Kill the process
				kill(pidList[s], SIGKILL);
				printf("Terminated process %i.\n", pidList[s]);
				fflush(stdout);
			}
			//  Exit the shell
			exit(0);
		}

		//  User enters a blank line or comment #
		else if(strcmp(dataArray[0], "\n\0") == 0 || strstr(dataArray[0], "#") != NULL) {
			//  Free data from getline
			free(data);
			data = NULL;
			statusVal = 0;
			//  Continue the program execution loop
			continue;
		}

		//  User changes directory with cd
		else if (strcmp(dataArray[0], "cd") == 0) {
			//  Path is specified
			if (dataArrayLength > 1) {
				chdir(dataArray[1]);
			}
			//  Default to cd home path environment
			else {
				const char *homePath;

				if ((homePath = getenv("HOME")) == NULL) {
				    homePath = getpwuid(getuid())->pw_dir;
				}
				chdir(homePath);
			}
		}

		//  User checks the status
		else if (strcmp(data, "status") == 0) {
			//  Status from signal
			if (statusVal > 1 && statusVal < 16) {
				printf("Terminated By Signal - %i\n", statusVal);
	        	fflush(stdout);
			}
			//  Status output with exit code
			else if (WIFEXITED(statusVal)) {
				printf("Exit Value - %i\n", WEXITSTATUS(statusVal));
	        	fflush(stdout);
	    	}
	    	//  Normal status output
	    	else {
	        	printf("Exit Status - %i\n", statusVal);
	        	fflush(stdout);
	    	}
		}

		//  Data has arguments
		else if (dataArrayLength > 0) {
		    //  Fork the pid to create a child process
		    pid = fork();

		    //  Fork failed
		    if (pid == -1) {
		        printf("Error: Could not create forked child.\n");
		        fflush(stdout);
		        statusVal = 1;
		        free(data);
				data = NULL;
		        break;
		    }

		    //  Child process created
		    else if (pid == 0) {
		    	//  SIGTSTP ignored for child foreground and background processes
		    	sigStpInfo.sa_handler = SIG_IGN;
		    	sigaction(SIGTSTP, &sigStpInfo, NULL);

		    	//  Foreground mode
		    	if (bgMode) {
		    		//  Child foreground process default sigint to end process upon signal
		    		sigIntInfo.sa_handler = SIG_DFL;
					sigaction(SIGINT, &sigIntInfo, NULL);
		    	}
		    	//  Background mode, set output to /dev/null
		    	if(!bgMode){
		    		//  Ignore signal sigint for background jobs
		    		sigIntInfo.sa_handler = SIG_IGN;
					sigaction(SIGINT, &sigIntInfo, NULL);
		    		//  Open /dev/null file
                    int file = open("/dev/null", O_RDONLY);
                    //  Failed to open file
                    if (file == -1) {
                        printf("Error: cannot open /dev/null file.");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Redirect output to /dev/null
                    if (dup2(file, 0) == -1) {
                        printf("Error: dup2 unsuccessful.");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Close the file descriptor
                    close(file);
                }

                //  Input file passed as argument
		    	if (fileIn != NULL) {
		    		//  Open the input file read only
                    int file = open(fileIn, O_RDONLY);
                    //  Failed to open input file
                    if (file == -1) {
                        printf("Error: cannot open input file.\n");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Redirect input from stdin to input file
                    if (dup2(file, 0) == -1) {
                        printf("Error: dup2 unsuccessful for input file.\n");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Close the file
                    close(file);
                }

                //  Output file passed as argument
		    	if (fileOut != NULL) {
		    		//  Open the output file and set permissions
		        	int file = open(fileOut, O_WRONLY | O_CREAT | O_TRUNC, 0755);
                    //  Failed to open output file
                    if(file == -1) {
                        printf("Error: Cannot open output file.\n");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Redirect standard output to the output file
                    if(dup2(file, 1) == -1){
                        printf("Error: dup2 not successful for output file.\n");
                        fflush(stdout);
                        statusVal = 1;
                        free(data);
						data = NULL;
						close(file);
                        exit(1);
                    }
                    //  Set close on exec flag for the output file
					if (fcntl(file, F_SETFD, FD_CLOEXEC) == -1) {
						printf("Error: Cannot set close on exec flag");
						fflush(stdout);
						statusVal = 1;
						free(data);
						data = NULL;
						close(file);
                        exit(1);
					}
					//  Close the output file
					close(file);
				}

				//  Execute command with arguments utilizing execvp
		        if (execvp(dataArray[0], dataArray) < 0) {
		            printf("Cannot execute command.\n");
		            fflush(stdout);
		            //  Bad command, set the status to 1
		            statusVal = 1;
		            free(data);
					data = NULL;
					exit(1);
		        }
		    }

		    //  Child process running
		    else {
		    	//  Foreground mode, wait for process to complete
		        if (bgMode || fgOnly) {
		        	waitpid(pid, &statusVal, 0);
                }
                //  Background mode, process started print pid
                else {
                    printf("Background pid is %i\n", pid);
                    //  Add the pid to the pid array
                    pidList[pidCount] = pid;
                    pidCount++;
                }
		    }
		}

		//  Free the getline data buffer
		free(data);
		data = NULL;
	}

	return 0;
}
