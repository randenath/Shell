/* Assignment 3: Shell
 * Author: Rohan H
 * Last Modified: 03/10/2026
 *
 * CS441/541: Project 3
 *
 */
#include "mysh.h"
#include <fcntl.h> //this is for open flags
#include <sys/stat.h> //this is for file permissions


struct bg_job_t *bg_job_list = NULL;
struct history_entry *history_list = NULL;
int history_counter = 1;

char **batch_files = NULL;
int batch_file_count = 0;

int shell_should_exit = 0;

int main(int argc, char * argv[]) {
  int ret;

    /*
     * Parse Command line arguments to check if this is an interactive or batch
     * mode run.
     */
    if( 0 != (ret = parse_args_main(argc, argv)) ) {
        fprintf(stderr, "Error: Invalid command line!\n");
        return -1;
    }

    /*
     * If in batch mode then process all batch files
     */
    if( TRUE == is_batch) {
        if( TRUE == is_debug ) {
            printf("Batch Mode!\n");
        }

        if( 0 != (ret = batch_mode()) ) {
            fprintf(stderr, "Error: Batch mode returned a failure!\n");
        }
    }
    /*
     * Otherwise proceed in interactive mode
     */
    else if( FALSE == is_batch ) {
        if( TRUE == is_debug ) {
            printf("Interactive Mode!\n");
        }

        if( 0 != (ret = interactive_mode()) ) {
            fprintf(stderr, "Error: Interactive mode returned a failure!\n");
        }
    }
    /*
     * This should never happen, but otherwise unknown mode
     */
    else {
        fprintf(stderr, "Error: Unknown execution mode!\n");
        return -1;
    }


    /*
     * Display counts
     */
    printf("-------------------------------\n");
    printf("Total number of jobs               = %d\n", total_jobs);
    printf("Total number of jobs in history    = %d\n", total_history);
    printf("Total number of jobs in background = %d\n", total_jobs_bg);

    /*
     * Cleanup
     */


    return 0;
}

int parse_args_main(int argc, char **argv)
{

    /*
     * If no command line arguments were passed then this is an interactive
     * mode run.
     */
	if (argc == 1) {
		is_batch = FALSE;
	} else {
		is_batch = TRUE;
	

    /*
     * If command line arguments were supplied then this is batch mode.
     */

		batch_file_count = argc - 1;
		batch_files = malloc(batch_file_count * sizeof(char*));

		for (int i = 0; i < batch_file_count; i++) {
			batch_files[i] = strdup(argv[i + 1]);
		}
	}

	return 0;
}


/*
 * Helper command to parse each command that gets entered into the command line.
 */
job_t* parseCommand (char *cmdLine) {
	
	job_t *job = malloc(sizeof(job_t));
	if (job == NULL) {
		perror("malloc");
		return NULL;
	}

	job -> full_command = strdup(cmdLine);
	job -> is_background = 0;
	job -> argc = 0;
	job -> input_file = NULL;
	job -> output_file = NULL;
	job -> append_mode = 0;

	int maxArgs = 64;
	job -> argv = malloc(maxArgs * sizeof(char*));
	if (job -> argv == NULL) {
		perror("malloc");
		free(job -> full_command);
		free(job);
		return NULL;
	}

	char *workingCopy = strdup(cmdLine);
	char *token;


	token = strtok(workingCopy, " \t");

	while (token != NULL && job -> argc < maxArgs - 1) { //logic to view and store args.

		if (strcmp(token, "&") == 0) { 
			job -> is_background = 1;

		} else if (strcmp(token, ";") == 0) {

		} else if (strcmp(token, ">") == 0) {
			token = strtok(NULL, " \t");
			if (token != NULL) {
				job -> output_file = strdup(token);
			} else {
				fprintf(stderr, "Error: No file specified for output redirection\n");
			}

		}else if (strcmp(token, "<") == 0) {
			token = strtok(NULL, " \t");
			if (token != NULL) {
				job -> input_file = strdup(token);
			} else {
				fprintf(stderr, "Error: No file specified for input redirection\n");
			}
		
		} else {
			job -> argv[job -> argc] = strdup(token);
			job -> argc++;
		}

		token = strtok(NULL, " \t");
	}

	job -> argv[job -> argc] = NULL;

	if (job -> argc > 0) {
		job -> binary = strdup(job -> argv[0]);
	} else {
		job -> binary = NULL;
	}


	free(workingCopy);

    	return job;
}

/*
 * Another helper function that adds commands to the history struct
 */

void add_to_history(char *command) {
	struct history_entry *new_entry = malloc(sizeof(struct history_entry));
	if (new_entry == NULL) {
		perror("malloc");
		return;
	}

	new_entry -> job_num = history_counter++;
	new_entry -> command = strdup(command);
	new_entry -> next = history_list;
	history_list = new_entry;
}

/*
 *Helper function to find a background job by its ID
 */

struct bg_job_t *find_job_by_id(int job_id) {
	struct bg_job_t *current = bg_job_list;

	while (current != NULL) {
		if (current -> job_id == job_id && current -> active) {
			return current;
		}
		current = current -> next;
	}

	return NULL; //this means the job is not found.
}

/*
 *Helper function to find the most recent job.
 */

struct bg_job_t *find_most_recent_job(void) {
	struct bg_job_t *current = bg_job_list;

	while (current != NULL) {
		if (current -> active) {
			return current;
		}
		current = current -> next;
	}

	return NULL;
}


/*
 *Helper method to split lines into multiple jobs.
 */

void execute_command_line(char *line) {
    char *line_copy = strdup(line);
    char *job_str;
    char *saveptr;

    // split by ;
    job_str = strtok_r(line_copy, ";", &saveptr);

    while (job_str != NULL) {
        // Trim leading whitespace
        while (*job_str == ' ' || *job_str == '\t') {
            job_str++;
        }

        if (strlen(job_str) > 0) {
            // Check if this job should be background
            int is_background = 0;
            int len = strlen(job_str);
            
            // Check for & at the end of the job string
            if (len > 0 && job_str[len - 1] == '&') {
                is_background = 1;
                job_str[len - 1] = '\0';  // Remove the &
                
                // Trim whitespace after removing &
                len--;
                while (len > 0 && (job_str[len - 1] == ' ' || job_str[len - 1] == '\t')) {
                    job_str[len - 1] = '\0';
                    len--;
                }
            }
            
            //handle possible multiple & within this job segment
            
            char *ampersand_copy = strdup(job_str);
            char *ampersand_str;
            char *ampersand_saveptr;
            
            // Split by & to get individual commands inside a background job
            ampersand_str = strtok_r(ampersand_copy, "&", &ampersand_saveptr);
            
            while (ampersand_str != NULL) {
                // trim whitespace
                while (*ampersand_str == ' ' || *ampersand_str == '\t') {
                    ampersand_str++;
                }
                
                if (strlen(ampersand_str) > 0) {
                    job_t *job = parseCommand(ampersand_str);
                    if (job != NULL) {
                   
                        job->is_background = is_background;
                        
                        add_to_history(ampersand_str);
                        
     			//now its the builtin handling:
                        
                        if (strcmp(job->binary, "exit") == 0) {
                            builtin_exit();

			    total_history++;
                            
                            // Cleanup job
                            for (int i = 0; i < job->argc; i++) {
                                free(job->argv[i]);
                            }
                            free(job->argv);
                            free(job->full_command);
                            free(job->binary);
                            if (job->input_file) {
                                free(job->input_file);
                            }
                            if (job->output_file) {
                                free(job->output_file);
                            }
                            free(job);
                            
                            free(ampersand_copy);
                            free(line_copy);
			    shell_should_exit = 1; //This fixes the issue I was having in my shell demo.
                            return;
                            
                        } else if (strcmp(job->binary, "jobs") == 0) {
                            builtin_jobs();
                            
                            // Cleanup job
                            for (int i = 0; i < job->argc; i++) {
                                free(job->argv[i]);
                            }
                            free(job->argv);
                            free(job->full_command);
                            free(job->binary);
                            if (job->input_file) {
                                free(job->input_file);
                            }
                            if (job->output_file) {
                                free(job->output_file);
                            }
                            free(job);
                            
                        } else if (strcmp(job->binary, "history") == 0) {
                            builtin_history();
                            
                            // Cleanup job
                            for (int i = 0; i < job->argc; i++) {
                                free(job->argv[i]);
                            }
                            free(job->argv);
                            free(job->full_command);
                            free(job->binary);
                            if (job->input_file) {
                                free(job->input_file);
                            }
                            if (job->output_file) {
                                free(job->output_file);
                            }
                            free(job);
                            
                        } else if (strcmp(job->binary, "wait") == 0) {
                            builtin_wait();
                            
                            // Cleanup job
                            for (int i = 0; i < job->argc; i++) {
                                free(job->argv[i]);
                            }
                            free(job->argv);
                            free(job->full_command);
                            free(job->binary);
                            if (job->input_file) {
                                free(job->input_file);
                            }
                            if (job->output_file) {
                                free(job->output_file);
                            }
                            free(job);
                            
                        } else if (strcmp(job->binary, "fg") == 0) {
                            if (job->argc == 1) {
                                builtin_fg();
                            } else {
                                int job_num = atoi(job->argv[1]);
                                builtin_fg_num(job_num);
                            }
                            
                            // Cleanup job
                            for (int i = 0; i < job->argc; i++) {
                                free(job->argv[i]);
                            }
                            free(job->argv);
                            free(job->full_command);
                            free(job->binary);
                            if (job->input_file) {
                                free(job->input_file);
                            }
                            if (job->output_file) {
                                free(job->output_file);
                            }
                            free(job);
                            
                        } else {
                            launch_job(job);
                        }
                    }
                }
                
                ampersand_str = strtok_r(NULL, "&", &ampersand_saveptr);
            }
            
            free(ampersand_copy);
        }

        job_str = strtok_r(NULL, ";", &saveptr);
    }

    free(line_copy);
}


int batch_mode(void) {

    /*
     * For each file...
     */

        /*
         * Open the batch file
         * If there was an error then print a message and move on to the next file.
         * Otherwise, 
         *   - Read one line at a time.
         *   - strip off new line
         *   - parse and execute
         * Close the file
         */

    /*
     * Cleanup
     */


	for (int file_id = 0; file_id < batch_file_count; file_id++) { //looking at each file 1 by 1.
		
		FILE *batch_file = fopen(batch_files[file_id], "r");
		if(batch_file == NULL) {
			fprintf(stderr, "Error: Cannot open file %s\n", batch_files[file_id]);
			continue; 
		}

		char *line = NULL;
		size_t len = 0;
		ssize_t nread;
		int line_num = 0;

		while ((nread = getline(&line, &len, batch_file)) != -1) {
			line_num++;

			if(line[nread -1] == '\n') {
				line[nread - 1] = '\0';
			}

			if (strlen(line) == 0) { //220 Petullo taumatized me into cheking for all situations. I still have nightmares.
				continue;
			}

			execute_command_line(line);
			
			if (shell_should_exit) {
				break;
			}

		}

		free(line);
		fclose(batch_file);
	}

	int active_count = 0;
	struct bg_job_t *current = bg_job_list;
	while (current != NULL) {
		if (current -> active) {
			active_count++;
		}
		current = current -> next;
	}

	if (active_count > 0) {
		printf("Waiting for %d background job(s) to complete...\n", active_count);
		current = bg_job_list;
		while (current != NULL) {
			if (current -> active) {
				int status;
				waitpid(current -> pid, &status, 0);
				current -> active = 0;
			}
			current = current -> next;
		}
		printf("All background jobs complete\n");
	}
	return 0;
}



int interactive_mode(void)
{

    //do {
        /*
         * Print the prompt
         */
        
        /*
         * Read stdin, break out of loop if Ctrl-D
         */
        

        /* Strip off the newline */
       
	/*
         * Parse and execute the command
         */
       
    //} while( 1/* end condition */);

    /*
     * Cleanup
     */

	char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        while(1) {
                printf(PROMPT);
                fflush(stdout);

                if ((nread = getline(&line, &len, stdin)) == -1) {
                        break;
                }

                if (line[nread - 1] == '\n') {
                        line[nread - 1] = '\0';
                }

                if (strlen(line) == 0) {

                        continue;
                }

		execute_command_line(line);

		if (shell_should_exit) {
			break;
		}

        }


        free(line);
        return 0;
}

/*
 * You will want one or more helper functions for parsing the commands 
 * and then call the following functions to execute them
 */

int launch_job(job_t * loc_job) {

    /*
     * Display the job
     */


    /*
     * Launch the job in either the foreground or background
     */

    /*
     * Some accounting
     */


	pid_t c_pid = 0;
	int status = 0;

	//forking a child (rephrase this)
	c_pid = fork();

	/* check for an error */
	if (c_pid < 0) {
		fprintf(stderr, "Error: Fork failed!\n");
		return -1;
	}

	/* Check if child */
	else if (c_pid == 0) {

		if (loc_job -> input_file != NULL) {
			int fd_in = open(loc_job -> input_file, O_RDONLY);
			if (fd_in < 0) {
				perror("Input redirection failed");
				exit(1);
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}

		if (loc_job -> output_file != NULL) {
			int fd_out = open(loc_job -> output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd_out < 0) {
				perror("Output redirection failed");
				exit(1);
			}
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}

		execvp(loc_job -> binary, loc_job -> argv);

		/* If we are here, it means that execvp failed :( Error out! */

		fprintf(stderr, "Error: Exec failed for command: %s!\n", loc_job -> binary);
		exit(-1);
	}

	/* Checking if parent */
	else {
		if (loc_job -> is_background == 0) {
			/* Foreground Job */
			waitpid(c_pid, &status, 0);
		} else {
			
			/* BackGround Job */

			struct bg_job_t *new_job = malloc(sizeof(struct bg_job_t)); //This is where we allocate memory for jobs. (background)
			
			if (new_job == NULL) {
				perror("malloc");
			} else {
				new_job -> job_id = total_jobs + 1;
				new_job -> pid = c_pid;
				new_job -> command = strdup(loc_job -> full_command);
				new_job -> active = 1;
				new_job -> next = bg_job_list;
				bg_job_list = new_job;

				printf("[%d] %d\n", new_job -> job_id, c_pid);
			}
		}


		total_jobs++;
		total_history++;
		if (loc_job -> is_background == 1) {
			total_jobs_bg++;
		}
	}

	/* Good old fasioned cleanup section */
	for (int i = 0; i < loc_job -> argc; i++) {
		free(loc_job -> argv[i]);
	}
	
	free(loc_job -> argv);
	free(loc_job -> full_command);
	free(loc_job -> binary);
	if (loc_job -> input_file) {
        	free (loc_job -> input_file);
	}
	if (loc_job -> output_file) {
		free (loc_job -> output_file);
	}
	free(loc_job);

	return 0;
}

int builtin_exit(void) {
	
	int active_count = 0;
	struct bg_job_t *current = bg_job_list;

	while (current != NULL) {
		if (current -> active) {
			active_count++;
		}
		current = current -> next;
	}

	if (active_count > 0) {
		printf("Waiting for %d background job(s) to complete...\n", active_count);

		current = bg_job_list;
		while (current != NULL) {
			if (current -> active) {
				int status;
				waitpid(current -> pid, &status, 0);
				current -> active = 0;
			}
			current = current -> next;
		}

		printf("All background jobs completed\n");
	}
	return 0;
}

int builtin_jobs(void) {
	
	struct bg_job_t *current = bg_job_list;
	int status;
	int job_count = 0;

	while (current != NULL) {
		if (current -> active) {
			pid_t result = waitpid(current -> pid, &status, WNOHANG); //process running check. 
			if (result == 0) {//means actively running
				printf("[%d] Running %s\n", current -> job_id, current -> command);
				job_count++;
			} else if (result > 0) { //means it is finished
				printf("[%d] Done %s\n", current -> job_id, current -> command);
				current -> active = 0;
			} else if (result == -1) { //means it no longer exits. Reduced to atoms!
                                printf("[%d] Done %s\n", current -> job_id, current -> command);
				current -> active = 0;
			}
		}

		current = current -> next; //getting next job	
	}

	total_history++;
	return 0;
}

int builtin_history(void) {

	int count = 0;
	struct history_entry *current = history_list;
	while (current != NULL) {
		count++;
		current = current -> next;
	}

	if (count == 0) {
		total_history++;
		return 0;
	}

	struct history_entry **entries = malloc(count * sizeof(struct history_entry*));
	if (entries == NULL) {
		perror("malloc");
		return -1;
	}

	current = history_list;
	for (int i = count - 1; i >= 0; i--) { // here is where the array is filled in reverse order since we add to the front.
		entries[i] = current;
		current = current -> next;
	}

	//printing the stuff
	for (int i = 0; i < count; i++) {
		printf("%d %s\n", entries[i] -> job_num, entries[i] -> command);
	}

	free(entries);
		
	total_history++;
	return 0;
}

int builtin_wait(void) {

	struct bg_job_t *current = bg_job_list;
	int status;
	int jobs_waited = 0;

	while (current != NULL) { // first checking for any active background jobs
		if (current -> active) {
			jobs_waited++;
		}

		current = current -> next;
	}

	if (jobs_waited == 0) {
		total_history++;
		return 0;
	}


	current = bg_job_list;//now doing the actual waiting.
	while (current != NULL) {
		if (current -> active) {

			waitpid(current -> pid, &status, 0);//blocking function. Like a giant shield!
			
			current -> active = 0;

		}
		current = current -> next;

	}

	total_history++;
	return 0;
}



int builtin_fg(void) {

	struct bg_job_t *job = find_most_recent_job();

	if (job == NULL) {
		fprintf(stderr, "fg: no current job\n");
		total_history++;
		return -1;
	}

	int status;
	waitpid(job -> pid, &status, 0); //here we wait for the job to be complete

	job -> active = 0;

	total_history++;
	return 0;
}

int builtin_fg_num(int job_num) {
	struct bg_job_t *job = find_job_by_id(job_num);

        if (job == NULL) {
                fprintf(stderr, "fg: job %d: no such job\n", job_num);
                total_history++;
                return -1;
        }

        int status;
        waitpid(job -> pid, &status, 0); //now we are waiting just like regular fg

        job -> active = 0;

	total_history++;
	return 0;
}
