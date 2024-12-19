#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

// PROTOTYPES
static void exitProgram(char **args, int argcp);
static void cd(char **args, int argcp);
static void pwd(char **args, int argcp);
static void cmdA_ls(char **args, int argc);
static void cmdA_cp(char **args, int argc);
static void cmdA_env(char **args, int argc);

// Helper functions for ls
static void print_file_info(struct stat *fileStat, const struct dirent *dp);
static void format_time(time_t modTime, char *timebuf, size_t bufSize);
static void get_permissions(mode_t mode, char *perm);

/*
 * builtIn
 *
 * Checks if a command is a built-in shell command and executes it if so.
 *
 * Args:
 *   args: array of strings containing command and arguments
 *   argcp: number of elements in args array
 *
 * Returns:
 *   1 if the command was a built-in, 0 otherwise.
 *
 * Built-in commands are executed directly by the shell process rather than
 * being forked to a new process. This function compares the given command
 * to each of the built-ins (exit, pwd, cd, and ls/cp/env or stat/tail/touch
 * depending on group). If a match is found, the corresponding function is called.
 *
 * Hint: Refer to checklist for group specific examples
 */
int builtIn(char **args, int argcp)
{
  if (args == NULL || argcp == 0)
    return 0;

  if (strcmp(args[0], "exit") == 0)
  {
    exitProgram(args, argcp);
    return 1;
  }
  else if (strcmp(args[0], "pwd") == 0)
  {
    pwd(args, argcp);
    return 1;
  }
  else if (strcmp(args[0], "cd") == 0)
  {
    cd(args, argcp);
    return 1;
  }
  else if (strcmp(args[0], "ls") == 0)
  {
    cmdA_ls(args, argcp);
    return 1;
  }
  else if (strcmp(args[0], "cp") == 0)
  {
    cmdA_cp(args, argcp);
    return 1;
  }
  else if (strcmp(args[0], "env") == 0)
  {
    cmdA_env(args, argcp);
    return 1;
  }
  return 0;
}

/*
 * exitProgram
 *
 * Terminates the shell with a given exit status.
 * If no exit status is provided, exits with status 0.
 * This function should use the exit(3) library call.
 *
 * Args:
 *   args: array of strings containing "exit" and optionally an exit status
 *   argcp: number of elements in args array
 */
static void exitProgram(char **args, int argcp)
{
  if (argcp > 2)
  {
    fprintf(stderr, "exit: too many arguments\n");
    return;
  }
  if (argcp == 1)
  {
    exit(0);
  }
  else
  {
    exit(atoi(args[1]));
  }
}

/*
 * pwd
 *
 * Prints the current working directory.
 *
 * Args:
 *   args: array of strings containing "pwd"
 *   argcp: number of elements in args array, should be 1
 *
 * Example Usage:
 *   Command: $ pwd
 *   Output: /some/path/to/directory
 */
static void pwd(char **args, int argcp)
{
  if (argcp != 1 || args == NULL)
  {
    fprintf(stderr, "pwd: no arguments required\n");
    return;
  }

  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    printf("%s\n", cwd);
  }
  else
  {
    perror("getcwd");
  }
}

/*
 * cd
 *
 * Changes the current working directory.
 * When no parameters are provided, changes to the home directory.
 * Supports . (current directory) and .. (parent directory).
 *
 * Args:
 *   args: array of strings containing "cd" and optionally a directory path
 *   argcp: number of elements in args array
 *
 * Example Usage:
 *   Command: $ pwd
 *   Output: /some/path/to/directory
 *   Command: $ cd ..
 *   Command: $ pwd
 *   Output: /some/path/to
 *
 * Hint: Read the man page for chdir(2)
 */
static void cd(char **args, int argcp)
{
  if (argcp > 2)
  {
    fprintf(stderr, "cd: too many arguments\n");
    return;
  }

  const char *path = (argcp == 1) ? getenv("HOME") : args[1];

  if (path == NULL)
  {
    fprintf(stderr, "cd: HOME environment variable not set\n");
    return;
  }

  if (chdir(path) != 0)
  {
    perror("cd");
  }
}

/*
 * cmdA_ls
 *
 * Lists the files and directories in the current directory.
 * If the -l flag is provided, the long format is used.
 *
 * Args:
 *   args: array of strings containing the command and arguments
 *   argc: number of arguments in the args array
 *
 * Note: If the -l flag is provided, the long format is used.
 */
static void cmdA_ls(char **args, int argc)
{
  DIR *dir = opendir("."); // Open current directory
  if (dir == NULL)
  {
    perror("opendir");
    return;
  }

  int long_format = (argc == 2 && strcmp(args[1], "-l") == 0);
  struct dirent *dp;     // Directory entry pointer
  struct stat file_stat; // File status structure
  int total_blocks = 0;

  // First pass: Calculate total blocks for -l format
  if (long_format)
  {
    while ((dp = readdir(dir)) != NULL)
    {
      if (stat(dp->d_name, &file_stat) == 0)
      {
        total_blocks += file_stat.st_blocks;
      }
    }
    printf("total %d\n", total_blocks / 2); // 512-byte block units
    rewinddir(dir);                         // Reset directory stream for the second pass
  }

  // Second pass: Print file details or names
  while ((dp = readdir(dir)) != NULL)
  {
    // Skip `.` and `..` entries
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
    {
      continue;
    }

    if (stat(dp->d_name, &file_stat) < 0) // Get file status
    {
      perror("stat");
      continue;
    }

    if (long_format)
    {
      print_file_info(&file_stat, dp); // Print detailed info
    }
    else
    {
      printf("%s\n", dp->d_name); // Print only the name (if not -l)
    }
  }
  if (closedir(dir) < 0)
  {
    perror("closedir");
  }
}

// Helper function for ls: Generate permissions string
static void get_permissions(mode_t mode, char *perm)
{
  perm[0] = S_ISDIR(mode) ? 'd' : '-';
  perm[1] = (mode & S_IRUSR) ? 'r' : '-';
  perm[2] = (mode & S_IWUSR) ? 'w' : '-';
  perm[3] = (mode & S_IXUSR) ? 'x' : '-';
  perm[4] = (mode & S_IRGRP) ? 'r' : '-';
  perm[5] = (mode & S_IWGRP) ? 'w' : '-';
  perm[6] = (mode & S_IXGRP) ? 'x' : '-';
  perm[7] = (mode & S_IROTH) ? 'r' : '-';
  perm[8] = (mode & S_IWOTH) ? 'w' : '-';
  perm[9] = (mode & S_IXOTH) ? 'x' : '-';
  perm[10] = '\0'; // Null-terminate the string
}

// Helper function for ls: Format the modification time
static void format_time(time_t modTime, char *timebuf, size_t bufSize)
{
  strftime(timebuf, bufSize, "%b %d %H:%M", localtime(&modTime));
}

// Helper function for ls: Print file information for -l format
static void print_file_info(struct stat *fileStat, const struct dirent *dp)
{
  char perm[11], timebuf[80];

  get_permissions(fileStat->st_mode, perm);
  format_time(fileStat->st_mtime, timebuf, sizeof(timebuf));

  printf("%s %ld %s %s %5ld %s %s\n",
         perm,
         fileStat->st_nlink,
         getpwuid(fileStat->st_uid)->pw_name,
         getgrgid(fileStat->st_gid)->gr_name,
         fileStat->st_size,
         timebuf,
         dp->d_name);
}

/*
 * cmdA_cp
 *
 * Copies the contents of one file to another.
 *
 * Args:
 *   args: array of strings containing the command and arguments
 *   argc: number of arguments in the args array
 *
 * Note: The cp command requires exactly two arguments.
 */
static void cmdA_cp(char **args, int argc)
{
  if (argc != 3)
  {
    fprintf(stderr, "cp: invalid number of arguments\n");
    return;
  }

  struct stat file_stat;
  if (stat(args[1], &file_stat) < 0)
  {
    perror("stat");
    return;
  }
  if (S_ISDIR(file_stat.st_mode)) // Check if the source is a directory
  {
    fprintf(stderr, "cp: '%s' is a directory\n", args[1]);
    return;
  }

  int src_file = open(args[1], O_RDONLY); // Open source file to read from
  if (src_file == -1)
  {
    perror("open");
    return;
  }

  int dest_file = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open destination file to write to
  if (dest_file == -1)
  {
    perror("open");
    if (close(src_file) == -1)
    {
      perror("close");
    }
    return;
  }

  char buffer[4096];
  ssize_t bytes_read;
  while ((bytes_read = read(src_file, buffer, sizeof(buffer))) > 0)
  {
    if (write(dest_file, buffer, bytes_read) != bytes_read) // Write to destination file
    {
      perror("write");
      if (close(src_file) == -1)
      {
        perror("close");
      }
      if (close(dest_file) == -1)
      {
        perror("close");
      }
      return;
    }
  }

  if (bytes_read == -1)
  {
    perror("read");
  }
  if (close(src_file) == -1)
  {
    perror("close");
  }
  if (close(dest_file) == -1)
  {
    perror("close");
  }
}

/*
 * cmdA_env
 *
 * Prints the environment variables or sets a new environment variable.
 *
 * Args:
 *   args: array of strings containing the command and arguments
 *   argc: number of arguments in the args array
 *
 * Note: If no arguments are provided, all environment variables are printed.
 *       If one argument is provided in the format NAME=VALUE, the environment variable is set.
 */
/*
 * cmdA_env
 *
 * Prints all environment variables if no arguments are passed.
 * If a NAME=VALUE argument is provided, it sets the environment variable.
 *
 * Args:
 *   args: array of strings containing the command and arguments
 *   argc: number of arguments in the args array
 */
static void cmdA_env(char **args, int argc)
{
  if (argc > 2)
  {
    fprintf(stderr, "env: too many arguments\n");
    return;
  }

  // If no arguments are provided, print all environment variables
  if (argc == 1)
  {
    extern char **environ;
    for (char **env = environ; *env != NULL; env++)
    {
      printf("%s\n", *env);
    }
    return;
  }

  // Set environment variable using putenv for NAME=VALUE format
  if (strchr(args[1], '=') == NULL)
  {
    fprintf(stderr, "env: invalid format, expected NAME=VALUE\n");
    return;
  }

  // Attempt to set environment variable
  if (putenv(args[1]) != 0)
  {
    perror("putenv");
  }

  // Print all environment variables after modification
  extern char **environ;
  for (char **env = environ; *env != NULL; env++)
  {
    printf("%s\n", *env);
  }
}
