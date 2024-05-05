# Description
This program classifies decimal numbers represented as 28x28 pixel blocks, with each pixel value ranging from 0 to 255, using the K-Nearest-Neighbor algorithm and parallel programming techniques involving forking and piping. It utilizes provided training and testing data to compare predicted results with actual results, then prints the number of correct predictions out of 10,000 decimal numbers. Users have the option to specify the number of child processes created during forking.

# How to Run (Require Linux Environment)
- Download the repository onto your Linux environment.
- Navigate to the root directory of the downloaded folder.
- Run "make" to compile the project.
- Execute ./classifier <K value> datasets/training_data.bin datasets/testing_data.bin <Number of child processes>
  - Replace <K value> with your chosen value for KNN, and <Number of child processes> with your desired number of child processes during forking.
  - For example, if K = 3 and you want 8 child processes, type: ./classifier 3 datasets/training_data.bin datasets/testing_data.bin 8.
- Wait for the result. Depending on your processor's power, it may take some time (sometimes 5+ minutes). The result will typically be between 9500 and 10000, indicating the number of correct guesses out of 10000 decimal numbers.

# What I Learned
- Parallel programming with multiple child processes by forking.
- Creates pipes to communicate between children and parents.
- C pointers and dereferencing.
- C dynamic memory management and File IO.

  

