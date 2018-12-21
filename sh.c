#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_SIZE 256
#define MAX_PARAMETERS 128

//#define DEBUG

extern void *_main(void *);

extern int _arp_cache(int, const char **);
extern int _ifconf(int, const char **);
extern int echo_init(int, const char **);
extern int netstat(int, const char **);
extern int http_init(int, const char **);

struct cmd {
    char *name;
    int (*func)(int, const char **);
};

#ifdef SHELL

int _cmd_func(int argc, const char **arg)
{
    int i;

    static struct cmd cmds[] = {{"arp", _arp_cache},
				{"ifconfig", _ifconf},
				{"netstat", netstat},
				{"http", http_init},
				{"echo", echo_init}};

    int count = sizeof(cmds) / sizeof(cmds[(0)]);

    for (i = 0; i < count; i++) {
	if (!(strncmp(arg[0], cmds[i].name, strlen(arg[0])))) {
	    return cmds[i].func(argc, arg);
	}
    }

    return -1;
}

int check_command(char *cmd)
{
    int argc;
    int ret;

    const char *arg[MAX_PARAMETERS] = {};
    char *tmp;

#ifdef DEBUG
    printf("%s\n", cmd);
#endif

    argc = 0;

    arg[argc] = strtok(cmd, " ");

    while (arg[argc] != (NULL)) {
	arg[++argc] = strtok(NULL, " ");

	if (arg[argc] == (NULL)) {
	    argc--;
	    break;
	}

    }

    if (argc == 0 && arg[0] == (NULL)) {
	return 0;
    }

    argc++;

    ret = _cmd_func(argc, arg);

    if (ret < 0) {
	printf("command not found\n");
    }

    return 0;
}

int _char_in(char ch)
{
    static char cmd[MAX_COMMAND_SIZE];
    static int index = 0;

    switch (ch) {
	case 'a' ... 'z':
	case 'A' ... 'Z':
	case 0 ... 9:
	case ' ':
	    cmd[index++] = ch;
//	    putchar(ch);
	break;

	case '\r':
	case '\n':
	    cmd[index++] = '\0';
	    check_command(cmd);
	    index = 0;
	    printf("m-shell> ");
//	    memset(cmd, '\0', MAX_COMMAND_SIZE - 1);
	break;
    }

    return 0;
}

#endif /* SHELL */

int main(int argc, char **args)
{
    int ret;
    char ch;

    pthread_t threadid;

    ret = pthread_create(&threadid, NULL, _main, NULL);

    if (ret != 0) {
	perror("error pthread_create");
	exit(-1);
    }

    while (1) {
#ifdef SHELL
	ch = getchar();

	_char_in(ch);
#endif
    }

    return 0;
}
