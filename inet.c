#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

int
try(int success, const char *msg, int fatal)
{
	if(!success)
	{
		perror(msg);
	}
	if(!success && fatal)
	{
		exit(EXIT_FAILURE);
	}
	return success;
}

extern char **environ;

void
dup_lim_exec(int fd, char *cmd, char *argv[])
{
	try(close(STDIN_FILENO) != -1, "close", 0);
	try(close(STDOUT_FILENO) != -1, "close", 0);
	try(dup2(fd, STDIN_FILENO) != -1, "dup2", 1);
	try(dup2(fd, STDOUT_FILENO) != -1, "dup2", 1);
	/*limits?*/
	environ = NULL;
	try(execv(cmd, argv) != -1, "execv", 1);
}

int
handle(int fd, char *cmd, char *argv[])
{
	pid_t pid;
	if(!try((pid = fork()) != -1, "fork", 0))
	{
		return -1;
	}
	else if(pid != 0)
	{
		return 0;
	}
	else
	{
		dup_lim_exec(fd, cmd, argv);
	}
	return -1;
}

int running = 1;

void
sighandler(int signum)
{
	(void)signum;
	running = 0;
}

void
setup_signals(void)
{
	int stopsignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
	int numsignals = sizeof(stopsignals) / sizeof(stopsignals[0]);
	int i;
	for(i = 0; i < numsignals; i++)
	{
		try(signal(stopsignals[i], sighandler) != SIG_ERR, "signal", 1);
	}
	try(signal(SIGCHLD, SIG_IGN) != SIG_ERR, "signal", 1);
}

int
getsocket(char *port)
{
	struct addrinfo hints = {AI_PASSIVE, AF_UNSPEC, SOCK_STREAM, 0, 0, NULL, NULL, NULL};
	struct addrinfo *result;
	int enabled = 1;
	int listenfd;
	try(getaddrinfo(NULL, port, &hints, &result) == 0, "getaddrinfo", 1);
	try((listenfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) != -1, "socket", 1);
	try(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) != -1, "setsockopt", 1);
	try(bind(listenfd, result->ai_addr, result->ai_addrlen) != -1, "bind", 1);
	freeaddrinfo(result);
	try(listen(listenfd, SOMAXCONN) != -1, "listen", 1);
	return listenfd;
}

int
main(int argc, char *argv[])
{
	int listenfd;
	int clientfd;
	if(argc < 3)
	{
		fprintf(stderr, "%s <port> command [ args ... ]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	listenfd = getsocket(argv[1]);
	setup_signals();
	while(running)
	{
		if(try((clientfd = accept(listenfd, NULL, NULL)) != -1, "accept", 0))
		{
			try(handle(clientfd, argv[2], &argv[2]) != -1, "handle", 0);
			try(close(clientfd) != -1, "close", 0);
		}
	}
	close(listenfd);
	exit(EXIT_SUCCESS);
}
