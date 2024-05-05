#include "knn.h"

typedef struct {
    double distance;      // distance of this Image with the testing Image
    int label;            // the label for this image (from 0-9)
} Distance_info;
/****************************************************************************/
/* For all the remaining functions you may assume all the images are of the */
/*     same size, you do not need to perform checks to ensure this.         */
/****************************************************************************/

/**************************** A1 code ****************************************/

/* Same as A1, you can reuse your code if you want! */
double distance(Image *a, Image *b) {
  int length = ((int)(a->sx)) * ((int)(a->sy));
  double buffer = 0;
  double result = 0;

  for(int ind = 0; ind < length; ind++) {
    // (int)((b->data)[ind]) is data for b at index ind
    buffer += pow(((int)((b->data)[ind])) - ((int)((a->data)[ind])), 2);
  }
  result = sqrt(buffer);

  return result; 
}

int most_occurred_labels(Distance_info *di, int di_length) {

  // each index represent label 0-9, the element represent occurrence
  int occurrence[10] = {0,0,0,0,0,0,0,0,0,0};

  // calculate the occurrences for all labels
  for (int ind = 0; ind < di_length; ind++) {
    (occurrence[(di[ind]).label])++;
  }

  // find the index with the most occurrence
  int max_occurrence = occurrence[0];
  int correct_label = 0;
  for (int ind = 1; ind < 10; ind++) {
    if (occurrence[ind] > max_occurrence) {
      max_occurrence = occurrence[ind];
      correct_label = ind;
    }
  }

  return correct_label;
}

/* Same as A1, you can reuse your code if you want! */
int knn_predict(Dataset *data, Image *input, int K) {
  // array for storing nearest k image distance infoad
  // don't need malloc here bc we don't need it after function exits
  Distance_info di_array[K];
  int di_array_ind = 0;

  for(int image_ind = 0; image_ind < (int)(data->num_items); image_ind++) {
    // calculate distance between a training image and input image
    double d = distance(&((data->images)[image_ind]), input);


    // populate di_array if not full, and sort from biggest to smallest
    if (di_array_ind < K) {
      di_array[di_array_ind].distance = d;
      di_array[di_array_ind].label = ((data->labels)[image_ind]);
      di_array_ind++;
    } else {
      // If di_array's full, check if current distance can replace 
      //any of the elements in it(if it's smaller than any element)
      int ind_to_replace = 0;
      // true is 1, false is 0
      int d_can_replace = 0;
      for(int ind = 0; ind < K; ind++) {
        // if current d is smaller than one of the element
        if ((di_array[ind].distance > d) && d_can_replace == 0) {
          ind_to_replace = ind;
          d_can_replace = 1;
        } else if ((di_array[ind].distance > 
              di_array[ind_to_replace].distance) && d_can_replace == 1) {
            ind_to_replace = ind;
        } 
      }
      // replace d with the appropriate element if possible
      if (d_can_replace == 1) {
        di_array[ind_to_replace].distance = d;
        di_array[ind_to_replace].label = ((data->labels)[image_ind]);
      }
    }
  }
  // find correct label
  int result_label = most_occurred_labels(di_array, K);

  return result_label;
}

/**************************** A2 code ****************************************/

/* Same as A2, you can reuse your code if you want! */
Dataset *load_dataset(const char *filename) {
  FILE *file_stream = fopen(filename, "rb");
  if (file_stream == NULL) {
    perror("Failed to open file: ");
    return NULL;
  }

  Dataset *ds = malloc(sizeof(Dataset));
  ds->num_items = 0;

  // first 4 bytes(size of int) represent the number of images in the file
  if (fread(&(ds->num_items), sizeof(int), 1, file_stream) != 1) {
    perror("Failed to read file: ");
    return NULL;
  }
  
  ds->images = malloc(sizeof(Image) * (ds->num_items));
  ds->labels = malloc(sizeof(unsigned char) * (ds->num_items));

  // read a image label (if read correctly will return 1, 
  // else it's end of file or there was an error)
  for(int img_ind = 0; img_ind < ds->num_items; img_ind++) {
    if(fread(&((ds->labels)[img_ind]),
        sizeof(unsigned char), 1, file_stream) != 1) {
      perror("Failed to read file: ");
      return NULL;
    }

    // size of image is always 28*28 which is 784
    ((ds->images)[img_ind]).sx = 28;
    ((ds->images)[img_ind]).sy = 28;
    ((ds->images)[img_ind]).data = malloc(sizeof(unsigned char) * 784);
    // store data into image
    if(fread(((ds->images)[img_ind]).data,
        sizeof(unsigned char), 784, file_stream) != 784){
      perror("Failed to read file: ");
      return NULL;
    }
  }

  if(fclose(file_stream) != 0) {
    // error closing
    perror("Failed to close file: ");
  }

  return ds;
}

/* Same as A2, you can reuse your code if you want! */
void free_dataset(Dataset *data) {
  for (int i = 0; i < data->num_items; i++) {
    free(data->images[i].data);
  }
  free(data->images);
  free(data->labels);
  free(data);
}


/************************** A3 Code below *************************************/

/**
 * NOTE ON AUTOTESTING:
 *    For the purposes of testing your A3 code, the actual KNN stuff doesn't
 *    really matter. We will simply be checking if (i) the number of children
 *    are being spawned correctly, and (ii) if each child is recieving the 
 *    expected parameters / input through the pipe / sending back the correct
 *    result. If your A1 code didn't work, then this is not a problem as long
 *    as your program doesn't crash because of it
 */

/**
 * This function should be called by each child process, and is where the 
 * kNN predictions happen. Along with the training and testing datasets, the
 * function also takes in 
 *    (1) File descriptor for a pipe with input coming from the parent: p_in
 *    (2) File descriptor for a pipe with output going to the parent:  p_out
 * 
 * Once this function is called, the child should do the following:
 *    - Read an integer `start_idx` from the parent (through p_in)
 *    - Read an integer `N` from the parent (through p_in)
 *    - Call `knn_predict()` on testing images `start_idx` to `start_idx+N-1`
 *    - Write an integer representing the number of correct predictions to
 *        the parent (through p_out)
 */
void child_handler(Dataset *training, Dataset *testing, int K, 
                   int p_in, int p_out) {
  // TODO: Compute number of correct predictions from the range of data 
  //      provided by the parent, and write it to the parent through `p_out`.

  int start_ind;
  int n;
  if (read(p_in, &start_ind, sizeof(int)) != sizeof(int)){
    perror("bad read on start index: ");
  }
  if (read(p_in, &n, sizeof(int)) != sizeof(int)){
    perror("bad read on N: ");
  }

  int prediction_label;
  int real_label;
  int amount_correct = 0;

  for (int i = start_ind; i < (start_ind + n); i++) {
    prediction_label = knn_predict(training, &(testing->images[i]), K);
    real_label = testing->labels[i];
    if (prediction_label == real_label) {
      amount_correct++;
    }
  }

  if (write(p_out, &amount_correct, sizeof(int)) != sizeof(int)) {
    perror("bad write on amount_correct: ");
  }
}