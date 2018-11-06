#include "mlshell.h"

char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &mlsh_cd,
  &mlsh_help,
  &mlsh_exit
};

int mlsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int mlsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "mlsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("mlsh");
    }
  }
  return 1;
}

int mlsh_help(char **args)
{
  int i;
  printf("mlsh - Marijus Laucevicius Shell\n");
  printf("Enter program's name and arguments and press enter.\n");
  printf("Possible options:\n");

  for (i = 0; i < mlsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use man for more info.\n");
  return 1;
}

int mlsh_exit(char **args)
{
  return 0;
}

#define mlsh_TOK_BUFSIZE 64
#define mlsh_TOK_DELIM " \t\r\n\a"
char **mlsh_split_line(char *line)
{
  int bufsize = mlsh_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "mlsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, mlsh_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += mlsh_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "mlsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, mlsh_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int mlsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
        // execvp - exec funkcija, kuri priima array'ju string'u
    if (execvp(args[0], args) == -1) {
      // perror printina system errora + programos varda
      perror("mlsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Forkingo error'as
    perror("mlsh");
  } else {
    // Error'u nera, paliekam parent procesa laukianti (waitpid)
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int mlsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < mlsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return mlsh_launch(args);
}


#define mlsh_RL_BUFSIZE 1024
char *mlsh_read_line(void)
{
  int bufsize = mlsh_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "mlsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character - skaitom kaip int'a, nes EOF yra int tipo
    c = getchar();

    // Jei pasiekem EOF, irasom ji kaip nuli ir grazinam bufferi
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    }
    // Jei nepasiekiem irasom ta raide 
    else {
      buffer[position] = c;
    }
    position++;

    // Jei pritruksta vietos, reallocuojam daugiau
    if (position >= bufsize) {
      bufsize += mlsh_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "mlsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


void mlsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    // skaitom inputa
    line = mlsh_read_line();
    // skirstom eilute i argsus
    args = mlsh_split_line(line);
    // executinam komanda
    status = mlsh_execute(args);

    // atlaisvinam kintamuosius
    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  // 1. Loadinam config failus (jei reikia)
  // 2. Run command loop.
  mlsh_loop();
  // 3. Shutdown/cleanup.

  return EXIT_SUCCESS;
}
