#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv) {
    int i, j, now, start, condition = 1, seconds = 5;
    double duration;
    FILE *outfile = fopen("/dev/null", "w");

/*
 * process environment variable and command line arguments
 */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-seconds") == 0) {
            seconds = atoi(argv[i + 1]);
            break;
        } else {
            fprintf(stderr, "Illegal flag: `%s'\n", argv[i]);
            exit(1);
        }
    }

    printf("Process: %d - Begining to write to file.\n", getpid());

    start = clock();
    while(condition) {
      //write a line to the file
      for(j = 0; j<100; j++) {
        fprintf(outfile, "A string! ");
      }
      fprintf(outfile, "\n");
      //check if done
      now = clock();
      duration =  (now - start)/(double) CLOCKS_PER_SEC;
      //printf("Duration: %f - Seconds: %d - condition: %d\n", duration, seconds, duration>=seconds);
      if(duration >= seconds) {condition = 0;}
    }
    fclose(outfile);

    printf("Process: %d - Finished.\n", getpid());
    return 0;
}
