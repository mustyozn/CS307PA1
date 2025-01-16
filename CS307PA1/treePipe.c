#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/wait.h>


/*
Idea:

1) check if root node, if it is input the value

2) check if leaf node, if it is apply the calculations

else:
3) create the left subtree

4)if it is the left child, First create 1 pipe:
this pipe will send the information of the calculated value of num1 back to the parent process

now fork a process to recursively call the function, 

By the way do not forget to redirect stdout to the write end of the pipe(to give the result back to the parent)

5)get the value of num1 from the left subtree

6)pass num1 to the right subtree

7) if it is the right child first create 1 pipe:
this pipe will send the value of num2 back to the parent

8)once the parent get's both num1 and num2, you can create another process to run this execvp function and get the result back via pipe

9)Done!



*/

/* Helper function to create the arrow based on current depth */
void printArrow(int currentDepth) {
    for (int i = 0; i < currentDepth * 3; i++) {
        fprintf(stderr, "-");
    }
    fprintf(stderr, ">");
}


void processTree(int maxDepth, int currentDepth, int lr, int num1){

    // Root node: prompt the user for num1 input
    if (currentDepth == 0) {
        printArrow(currentDepth);
        fprintf(stderr, " Current depth: %d, lr: %d\n", currentDepth, lr);
        printf("Please enter num1 for the root: ");
        scanf("%d", &num1);
        //printf("Root node received num1: %d\n", num1);
    }

    if (currentDepth == maxDepth) {
        // Leaf node reached
        //fprintf(stderr, "Leaf node reached at depth %d (lr: %d)\n", currentDepth, lr);
        //fprintf(stderr, "This is the leaf node and the value of num1 = %d\n", num1);
        int num2 = 1;

        // Create buffer for num1 and num2
        char buffer[50];  // Increased buffer size to 50
        snprintf(buffer, sizeof(buffer), "%d %d", num1, num2);

        // Create pipe for passing input to execvp
        int pipe_for_execvp[2];
        if (pipe(pipe_for_execvp) == -1) {
            fprintf(stderr, "pipe_for_execvp failed\n");
            exit(1);
        }

        // Create pipe for capturing output (stdout) from execvp
        int pipe_for_output[2];
        if (pipe(pipe_for_output) == -1) {
            fprintf(stderr, "pipe_for_output failed\n");
            exit(1);
        }

        pid_t pid_exec = fork();  // Fork a new process for execvp
        if (pid_exec < 0) {
            fprintf(stderr, "fork failed\n");
            exit(1);
        } else if (pid_exec == 0) {
            // Child process (execvp will be executed here)

            // Close unused ends of pipes
            close(pipe_for_execvp[1]);  // Close write end (child will read input)
            close(pipe_for_output[0]);  // Close read end (child will write output)

            // Redirect stdin to the read end of the input pipe
            dup2(pipe_for_execvp[0], STDIN_FILENO);
            close(pipe_for_execvp[0]);  // Close after redirecting

            // Redirect stdout to the write end of the output pipe
            dup2(pipe_for_output[1], STDOUT_FILENO);
            close(pipe_for_output[1]);  // Close after redirecting

            // Now execute the appropriate program
            if (lr == 0) {
                char *args[] = {"./left", NULL};  // Replace with your actual executable
                execvp(args[0], args);  // Execute the left program
            } else {
                char *args[] = {"./right", NULL};  // Replace with your actual executable
                execvp(args[0], args);  // Execute the right program
            }

            // If execvp fails
            fprintf(stderr, "execvp failed\n");
            exit(1);
        } else {
            // Parent process

            // Close unused ends of pipes
            close(pipe_for_execvp[0]);  // Close read end (parent will write input)
            close(pipe_for_output[1]);  // Close write end (parent will read output)

            // Write buffer to child's stdin
            write(pipe_for_execvp[1], buffer, strlen(buffer));
            close(pipe_for_execvp[1]);  // Close the write end after sending the data

            // Read the output from the child process
            char output_buffer[50];  // Set output buffer size to 50
            ssize_t nbytes = read(pipe_for_output[0], output_buffer, sizeof(output_buffer) - 1);
            if (nbytes > 0) {
                output_buffer[nbytes] = '\0';  // Null-terminate the string

                // Convert output to an integer using atoi
                int result_from_execvp = atoi(output_buffer);
                if(maxDepth != 0){
                    printArrow(currentDepth);
                    fprintf(stderr, " Current depth: %d, lr: %d\n", currentDepth, lr);
                }
                
                printArrow(currentDepth);
                fprintf(stderr, " My num1 is: %d\n", num1);
                printArrow(currentDepth);
                fprintf(stderr," My result is: %d\n", result_from_execvp);

                if(maxDepth == 0){
                    fprintf(stderr, "The final result is: %d\n",result_from_execvp );
                }
                //fprintf(stderr, "leaf node calculated the value as integer: %d, lr:%d, current_depth:%d\n", result_from_execvp, lr, currentDepth);
                if(maxDepth !=0 ){
                    printf("%d\n",result_from_execvp );//returning the result back to the parent
                }
                   
            } else {
                fprintf(stderr, "No output received from execvp\n");
            }

            close(pipe_for_output[0]);  // Close the read end after reading
            

            // Wait for the child process to finish
            wait(NULL);
        }
    }
    //fprintf(stderr, "continue the function\n");

    else {
        // Non-leaf node: Create left process
        

        // Step 1: Create pipes for left child communication
        int pipe_left_to_parent[2]; // Left Child -> Parent

        if(pipe(pipe_left_to_parent) == -1) {
            fprintf(stderr, "pipe creation failed\n");
            exit(1);
        }

        pid_t pid_left = fork();  // Fork the left child
        if (pid_left < 0) {
            fprintf(stderr, "fork failed\n");
            exit(1);
        }
        else if(pid_left == 0){
            // We are in the left child process
            /*
            if(currentDepth != 0){
                printArrow(currentDepth);
                fprintf(stderr, " Current depth: %d, lr: %d\n", currentDepth, lr);
                
            }
            */
            

            close(pipe_left_to_parent[0]);  // Close read end (left writes output)

            int pipe_proc[2];  // Pipe between proc and left process

            if (pipe(pipe_proc) == -1) {
                fprintf(stderr, "pipe creation for proc failed\n");
                exit(1);
            }

            pid_t proc = fork();  // Fork a process to handle the recursive call
            if (proc < 0) {
                fprintf(stderr, "fork for proc failed\n");
                exit(1);
            }
            else if(proc == 0){
                // This is the process that handles the recursive call

                close(pipe_proc[0]);  // Close read end, proc will write result

                dup2(pipe_proc[1], STDOUT_FILENO);  // Redirect stdout to pipe
                close(pipe_proc[1]);
                
                // Recursive call for the left subtree (lr = 0 for left)
                processTree(maxDepth, currentDepth + 1, 0, num1);  // Left child recursive call

                exit(0);  // End the proc process
            }


            else {


                // We are still in the left child process

                close(pipe_proc[1]);  // Close write end (proc will write result)

                // Wait for proc to finish and read the result
                wait(NULL);  // Wait for the proc to complete

                char result_buffer[50];  // Buffer to store the string result
                ssize_t nbytes = read(pipe_proc[0], result_buffer, sizeof(result_buffer) - 1);  // Read the string result from proc
                if (nbytes > 0) {
                    result_buffer[nbytes] = '\0';  // Null-terminate the string
                    int result = atoi(result_buffer);  // Convert the string to an integer
                    //fprintf(stderr, "Left child received result from proc, depth %d: result: %d\n", currentDepth, result);

                    // Write the result back to the parent
                    write(pipe_left_to_parent[1], &result, sizeof(result));  // Send result to parent
                } else {
                    fprintf(stderr, "No result received from proc\n");
                }

                close(pipe_proc[0]);  // Close read end after reading
                close(pipe_left_to_parent[1]);  // Close write end after sending the result

                exit(0);  // End the left child process
            }



        }
        else {
            // We are in the parent process
            if(currentDepth != 0){
                printArrow(currentDepth);
                fprintf(stderr, " Current depth: %d, lr: %d\n", currentDepth, lr);
                
            }

            



            close(pipe_left_to_parent[1]);  // Close write end (parent will read result from left child)

            // Wait for the left child to finish and read the result
            wait(NULL);  // Wait for the left child to complete
            int updated_num1;
            printArrow(currentDepth);
            read(pipe_left_to_parent[0], &updated_num1, sizeof(updated_num1));  // Read result from left child
            fprintf(stderr, " My num1 is: %d\n", updated_num1);
            close(pipe_left_to_parent[0]);  // Close read end after reading

            //fprintf(stderr, "Parent received updated num1 from left child: %d\n", updated_num1);

            // Now proceed with the right subtree

            // Step 1: Create pipes for right child communication
            int pipe_right_to_parent[2]; // Right Child -> Parent

            if (pipe(pipe_right_to_parent) == -1) {
                fprintf(stderr, "pipe creation failed\n");
                exit(1);
            }

            pid_t pid_right = fork();  // Fork the right child
            if (pid_right < 0) {
                fprintf(stderr, "fork failed\n");
                exit(1);
            }
            else if (pid_right == 0) {
                // We are in the right child process
                /*
                if(currentDepth != 0){
                    printArrow(currentDepth);
                    fprintf(stderr, " Current depth: %d, lr: %d\n", currentDepth, lr);
                }
                */

                close(pipe_right_to_parent[0]);  // Close read end (right writes output)

                int pipe_proc_right[2];  // Pipe between proc and right process

                if (pipe(pipe_proc_right) == -1) {
                    fprintf(stderr, "pipe creation for proc (right child) failed\n");
                    exit(1);
                }

                pid_t proc_right = fork();  // Fork a process to handle the recursive call
                if (proc_right < 0) {
                    fprintf(stderr, "fork for proc (right child) failed\n");
                    exit(1);
                }
                else if (proc_right == 0) {
                    // This is the process that handles the recursive call for the right subtree

                    close(pipe_proc_right[0]);  // Close read end, proc will write result

                    dup2(pipe_proc_right[1], STDOUT_FILENO);  // Redirect stdout to pipe
                    close(pipe_proc_right[1]);

                    // Recursive call for the right subtree (lr = 1 for right)
                    processTree(maxDepth, currentDepth + 1, 1, updated_num1);  // Right child recursive call

                    exit(0);  // End the proc process
                }
                else {

                    // We are still in the right child process

                    close(pipe_proc_right[1]);  // Close write end (proc will write result)

                    // Wait for proc to finish and read the result
                    wait(NULL);  // Wait for the proc to complete

                    char result_buffer[50];  // Buffer to store the string result
                    ssize_t nbytes = read(pipe_proc_right[0], result_buffer, sizeof(result_buffer) - 1);  // Read the string result from proc
                    if (nbytes > 0) {
                        result_buffer[nbytes] = '\0';  // Null-terminate the string
                        int right_result = atoi(result_buffer);  // Convert the string to an integer
                        //fprintf(stderr, "Right child received result from proc, depth %d: %d\n", currentDepth, right_result);

                        // Write the result back to the parent
                        write(pipe_right_to_parent[1], &right_result, sizeof(right_result));  // Send result to parent
                    } else {
                        fprintf(stderr, "No result received from proc\n");
                    }

                    close(pipe_proc_right[0]);  // Close read end after reading
                    close(pipe_right_to_parent[1]);  // Close write end after sending the result

                    exit(0);  // End the right child process
                }

            }



            else {

                // We are in the parent process

                close(pipe_right_to_parent[1]);  // Close write end (parent will read result from right child)

                // Wait for the right child to finish and read the result
                wait(NULL);  // Wait for the right child to complete
                int updated_num2;
                read(pipe_right_to_parent[0], &updated_num2, sizeof(updated_num2));  // Read result from right child
                close(pipe_right_to_parent[0]);  // Close read end after reading

                //fprintf(stderr, "Parent received updated num2 from right child: %d\n", updated_num2);

                

                // Step 1: Create pipes for execvp input and output communication
                int pipe_for_input[2];     // For passing num1 and num2 to execvp
                int pipe_for_execvp[2];    // For capturing output from execvp

                if (pipe(pipe_for_input) == -1 || pipe(pipe_for_execvp) == -1) {
                    fprintf(stderr, "pipe creation failed\n");
                    exit(1);
                }

                // Step 2: Fork to create a new process for execvp
                pid_t pid_exec = fork();
                if (pid_exec < 0) {
                    fprintf(stderr, "fork failed\n");
                    exit(1);
                } 
                
                else if (pid_exec == 0) {
                    // In the child process for execvp

                    // Close the unused ends of the pipes
                    close(pipe_for_input[1]);  // Close the write end (execvp will read from this pipe)
                    close(pipe_for_execvp[0]);  // Close the read end (execvp will write output to this pipe)

                    // Step 3: Redirect stdin and stdout
                    dup2(pipe_for_input[0], STDIN_FILENO);  // Redirect stdin to the read end of the input pipe
                    close(pipe_for_input[0]);  // Close the read end after redirecting

                    dup2(pipe_for_execvp[1], STDOUT_FILENO);  // Redirect stdout to the write end of the execvp pipe
                    close(pipe_for_execvp[1]);  // Close the write end after redirecting

                    // Now execute the appropriate program (e.g., calculation)
                    if (lr == 0) {
                        char *args[] = {"./left", NULL};
                        execvp(args[0], args);  // Execute the left program
                    } else {
                        char *args[] = {"./right", NULL};
                        execvp(args[0], args);  // Execute the right program
                    }

                    // If execvp fails
                    fprintf(stderr, "execvp failed\n");
                    exit(1);
                } 
                else {
                    // In the parent process

                    close(pipe_for_input[0]);  // Close the read end (parent will write input to execvp)
                    close(pipe_for_execvp[1]);  // Close the write end (parent will read output from execvp)

                    // Step 4: Write num1 and num2 to the child's stdin (through the pipe)
                    char buffer[50];
                    snprintf(buffer, sizeof(buffer), "%d %d", updated_num1, updated_num2);  // Prepare the input for execvp
                    write(pipe_for_input[1], buffer, strlen(buffer));  // Write to execvp's stdin
                    close(pipe_for_input[1]);  // Close the write end after sending the input

                    // Step 5: Read the output from the child process (execvp)
                    char output_buffer[50];  // Buffer to store execvp's output
                    ssize_t nbytes = read(pipe_for_execvp[0], output_buffer, sizeof(output_buffer) - 1);
                    int result_from_execvp;
                    if (nbytes > 0) {
                        output_buffer[nbytes] = '\0';  // Null-terminate the string

                        // Convert the output to an integer
                        result_from_execvp = atoi(output_buffer);
                        //fprintf(stderr, "Parent received output from execvp as integer: %d\n", result_from_execvp);
                    } else {
                        fprintf(stderr, "No output received from execvp\n");
                    }

                    close(pipe_for_execvp[0]);  // Close the read end after reading

                    // Step 6: Decide what to do with the result based on current depth
                    if (currentDepth == 0) {
                        // Root node: Print result to the console
                        printArrow(currentDepth);
                        fprintf(stderr, " Current depth: %d, lr: %d, my num1: %d, my num2: %d\n", currentDepth, lr, updated_num1, updated_num2);
                        printArrow(currentDepth);
                        fprintf(stderr," My result is: %d\n", result_from_execvp);
                        printf("The final result is: %d\n", result_from_execvp);
                    } 
                    else {
                        // Non-root node: Send result to parent via stdout
                        printArrow(currentDepth);
                        fprintf(stderr, " Current depth: %d, lr: %d, my num1: %d, my num2: %d\n", currentDepth, lr, updated_num1, updated_num2);
                        printArrow(currentDepth);
                        fprintf(stderr," My result is: %d\n", result_from_execvp);
                        
                        //fprintf(stderr, "Non-root node at depth %d. Sending result to parent: %d\n", currentDepth, result_from_execvp);
                        printf("%d\n", result_from_execvp);  // Send the result to stdout (which should be piped to parent)
                    }

                    // Wait for the child process to finish
                    wait(NULL);
                }
            }





        }

    }






}


int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: treePipe <current depth> <max depth> <left-right>\n");
        return 1;
    }

    // Parse command line arguments
    int currentDepth = atoi(argv[1]);
    int maxDepth = atoi(argv[2]);
    int lr = atoi(argv[3]);
    int num1 = 0;

    // Call the processTree function with pipes and initial parameters
    processTree(maxDepth, currentDepth, lr, num1);

    return 0;
}