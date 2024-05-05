#include "knn.h"

// Makefile included in starter:
//    To compile:               make
//    To decompress dataset:    make datasets
//
// Example of running validation (K = 3, 8 processes):
//    ./classifier 3 datasets/training_data.bin datasets/testing_data.bin 8

/*****************************************************************************/
/* This file should only contain code for the parent process. Any code for   */
/*      the child process should go in `knn.c`. You've been warned!          */
/*****************************************************************************/

/**
 * main() takes in 4 command line arguments:
 *   - K:  The K value for kNN
 *   - training_data: A binary file containing training image / label data
 *   - testing_data: A binary file containing testing image / label data
 *   - num_procs: The number of processes to be used in validation
 * 
 * You need to do the following:
 *   - Parse the command line arguments, call `load_dataset()` appropriately.
 *   - Create the pipes to communicate to and from children
 *   - Fork and create children, close ends of pipes as needed
 *   - All child processes should call `child_handler()`, and exit after.
 *   - Parent distributes the testing set among childred by writing:
 *        (1) start_idx: The index of the image the child should start at
 *        (2)    N:      Number of images to process (starting at start_idx)
 *     Each child should gets N = ceil(test_set_size / num_procs) images
 *      (The last child might get fewer if the numbers don't divide perfectly)
 *   - Parent waits for children to exit, reads results through pipes and keeps
 *      the total sum.
 *   - Print out (only) one integer to stdout representing the number of test 
 *      images that were correctly classified by all children.
 *   - Free all the data allocated and exit.
 */
int main(int argc, char *argv[]) {
  // TODO: Handle command line arguments
  
  if (argc != 5) {
    fprintf(stderr, "Invalid argument count!\n"); 
    exit(1);
  }

  // argv[1] is k
  int k = (int)strtol(argv[1], NULL, 10);
  if(k == 0) {
    fprintf(stderr, "Invalid k value!\n"); 
    exit(1);
  }

  // argv[2] is the training name list 
  Dataset *training_ds = load_dataset(argv[2]);
  if (training_ds == NULL ) {
    fprintf(stderr, "Failed to open training name list!\n"); 
    exit(1);
  }

  // argv[3] is the testing name list 
  Dataset *testing_ds = load_dataset(argv[3]);
  if (testing_ds == NULL) {
    fprintf(stderr, "Failed to open testing name list!\n"); 
    exit(1);
  }

  // argv[4] is the number of children
  int num_of_children = (int)strtol(argv[4], NULL, 10);
  if (num_of_children <= 0) {
    fprintf(stderr, "invalid process number!\n"); 
    exit(1);
  }


  int starting_index;
  int n = ceil(((double)testing_ds->num_items) / num_of_children);
  int remainder = 0;
  int remainder_child = 0;

  if (n != 0) {
    remainder = testing_ds->num_items % n;
    // remainder_child = child_NO_of_that_child - 1
    // if divide evenly, remainder_child will be bigger than i - 1
    // (means will never get to remainder_child in the for loop)
    remainder_child = testing_ds->num_items / n;
  }
  int image_after_remainder = 0;


  int fd_pipe1[num_of_children][2];
  int fd_pipe2[num_of_children][2];

  for (int i = 0; i < num_of_children; i++) {
    // two pipes per child, parent write to second pipe and read from first pipe
    // child write to first pipe and read from second pipe
    if (pipe(fd_pipe1[i]) == -1) {
      perror("Fail to open pipe! ");
      exit(1);
    }
    if (pipe(fd_pipe2[i]) == -1) {
      perror("Fail to open pipe! ");
      exit(1);
    }

    int result = fork();
    if (result < 0) {
      perror("Fail to fork! ");
      exit(1);
    } else if (result == 0) { // child process

      // close the unneccessary fds
      if(close(fd_pipe1[i][0]) == -1) {
        perror("Can't close!");
      }
      if(close(fd_pipe2[i][1]) == -1) {
        perror("Can't close!");
      }
      for(int child_no = 0; child_no < i; child_no++) {
        if(close(fd_pipe1[child_no][0]) == -1) {
          perror("Can't close! fd_pipe1 read");
        }
      }

      // child read from second pipe and write to parent to first pipe
      child_handler(training_ds, testing_ds, k, fd_pipe2[i][0], fd_pipe1[i][1]);
      if(close(fd_pipe2[i][0]) == -1) {
        perror("Can't close!");
      }
      if(close(fd_pipe1[i][1]) == -1) {
        perror("Can't close!");
      }

      // free malloc stuff in children
      free_dataset(training_ds);
      free_dataset(testing_ds);
      
      // i tells us which child exited, which us useful when 
      // parent calls wait later to choose which fd_pipe1 to read from
      exit(i);

    } else { // parent process

      // close the unneccessary fds
      if(close(fd_pipe1[i][1]) == -1) {
        perror("Can't close!");
      }
      if(close(fd_pipe2[i][0]) == -1) {
        perror("Can't close!");
      }

      // parent write to second pipe
      starting_index =  i * n;
      
      if (write(fd_pipe2[i][1], &starting_index, sizeof(int)) != sizeof(int)) {
        perror("bad write to child on starting index: ");
      }

      if (n == 0 || i > remainder_child) {
        if (write(fd_pipe2[i][1], 
          &image_after_remainder, sizeof(int)) != sizeof(int)) {
        perror("bad write to child on N: ");
        }
      } else if (i == remainder_child) {
        // if it's the last children and there is remainder 
        if (write(fd_pipe2[i][1], &remainder, sizeof(int)) != sizeof(int)) {
          perror("bad write to child on remainder: ");
        }
      } else {
        if (write(fd_pipe2[i][1], &n, sizeof(int)) != sizeof(int)) {
          perror("bad write to child on N: ");
        }
      }

      if(close(fd_pipe2[i][1]) == -1) {
        perror("Can't close!");
      }
    }

  }

  int child_correct = 0;
  int total_correct = 0;
  for (int i = 0; i < num_of_children; i++) {
    // get result from child (which is validate_out)
    int status;
    wait(&status);
    int child_num;
    if (WIFEXITED(status)) {
      child_num = WEXITSTATUS(status);
      if (read(fd_pipe1[child_num][0], &child_correct, sizeof(int)) != sizeof(int)) {
        perror("bad read to parent on child correct: ");
      }

      total_correct += child_correct;
      if(close(fd_pipe1[child_num][0]) == -1) {
        perror("Can't close!");
      }
    }
  }

  // Print out answer
  printf("%d\n", total_correct);

  free_dataset(training_ds);
  free_dataset(testing_ds);

  return 0;
}
