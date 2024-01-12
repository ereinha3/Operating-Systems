/*
* Description: Project 1 header file. Contains function declarations for the commands listed in the project
*			   Description.
*
* Author: Ethan Reinhart
*
* Date: 10/16/2023
*
*/
#include<sys/stat.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<dirent.h>
#include<errno.h>
#include"string_parser.h"

void listDir(){
	/*Function to call ls command
	 * Base on project parameters, this only lists elements of current directory of the project*/
	//
	// Obtain current directory
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	// Error handling if current directory is NULL
	if (cwd == NULL){
		char * err_msg = "No directory inputted.\n";
		write(1, err_msg, strlen(err_msg));
	}
	// Attempt to open directory
	DIR* dir = opendir(cwd);
	// Error handling if directory did not open properly
	if (dir==NULL){
		char * err_msg = "Error opening directory. Check permissions.\n";
		write(1, err_msg, strlen(err_msg));
	}
	struct dirent *dp;
	// Writing folder or file names to standard output
	while ((dp = readdir(dir)) != NULL){
		write(1, dp->d_name, strlen(dp->d_name));
		write(1, "\t", strlen("\t"));
	}
	write(1, "\n", strlen("\n"));
	// Closing directory to ensure no memory loss
	closedir(dir);
}

void showCurrentDir(){
        /*for the pwd command
	 * Prints current directory of the project*/
	char cwd[1024];
	// Obtaining current dictory and printing to standard output
	if (getcwd(cwd, sizeof(cwd))!=NULL){
		write(1, cwd, strlen(cwd));
		write(1, "\n", strlen("\n"));
	}
	// If current directory is not obtained, print error message to standard output
	else {
		char* err_msg = "CWD command failed!\n";
		write(1, err_msg, strlen(err_msg));
	}
}

void makeDir(char *dirName){
	/*for the mkdir command*/
	// Create a directory with the given name
	int dir = mkdir(dirName, 0777);
	// If creating a directory fails, print error message to standard output
	if (dir==-1){
		char* err_msg = "Directory already exists!\n";
		write(1, err_msg, strlen(err_msg));
	}
}

void changeDir(char *dirName){
	/*for the cd command*/
	// Changes the directory to the current working directory
	if (chdir(dirName)==-1){
		// If this command fails, an error message is printed to standard output
		char* err_msg = "CHDIR command failed!\n";
		write(1, err_msg, strlen(err_msg));
	}
}

void copyFile(char *sourcePath, char *destinationPath){
	/*for the cp command
	 * This command allows the destination to be either a directory to which a copy file of the same name is created in
	 * or a file name where the file is copied in the same directory*/
	char contents[1024];
	int fr, fw;
	// Open source file
	fr = open(sourcePath, O_RDONLY);
	// If source file fails to open, print error to standard output
	if (fr==-1){
		char* err_msg = "CP command failed! Failed to open source file!\n";
		write(1, err_msg, strlen(err_msg));
	}
	// Swapping to desired directory if destinationPath is a directory name
	if (chdir(destinationPath)!=-1){
		// Parsing the source file using created function
		// This function creates tokens where the last is NULL
		command_line tokens = str_filler(sourcePath, "/");
		int i = tokens.num_token;
		// Taking the source file name so be the last token after parsing or the first
		if (i > 1){
			sourcePath = tokens.command_list[i-1];
		}
		else {
			sourcePath = tokens.command_list[0];
		}
		// Creating this file in the new directory
		fw = open(sourcePath, O_CREAT | O_WRONLY | O_TRUNC, 0744);
		free_command_line(&tokens);
	}
	// Otherwise it is a filename and creates a file to be copied to in the same directory
	else {
		fw = open(destinationPath, O_CREAT | O_WRONLY | O_TRUNC, 0744);
	}
	// Error messaging on open failure
	if (fw==-1) {
		char* err_msg = "CP command failed! Failed to open destination file!\n";
		write(1, err_msg, strlen(err_msg));
	}
	int n;
	// Iterating over lines of input file and writing to output file
	while ((n=read(fr, contents, sizeof(contents)))>0){
		write(fw, contents, n);
	}
	// Closing both to avoid memory leaks
	close(fr);
	close(fw);
}

void moveFile(char *sourcePath, char *destinationPath){
	/*for the mv command*/
	// Attempts to rename using built-in function
	if (rename(sourcePath, destinationPath)==-1){
		// Error messaging if fail
		char* err_msg = "MV command failed!\n";
		write(1, err_msg, strlen(err_msg));
	}
}

void deleteFile(char *filename){
	/*for the rm command*/
	// Unlinking the file name from the memory and deleting memory
	if (unlink(filename)==-1){
		// Else error messaging
		char* err_msg = "RM command failed! File not found or permission need to be changed!\n";
		write(1, err_msg, strlen(err_msg));
	}
}

void displayFile(char *filename){
	/*for the cat command*/
	int fd_open;
	// Opening filename
	fd_open = open(filename, O_RDONLY);
	char contents[1];
	int n;
	// Error messaging if file open failed
	if (fd_open==-1){
		char* err_msg = "CAT command failed! Failed to open file!\n";
		write(1, err_msg, strlen(err_msg));
	}
	else{
		// Writing contents to standard output line by line
		while((n = read(fd_open, contents, sizeof(contents)))>0){
			write(1, contents, sizeof(contents));
		}
		// Closing file
		close(fd_open);
	}
}

