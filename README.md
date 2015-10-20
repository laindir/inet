#inet

##NAME

inet - connect filters to TCP

##SYNOPSIS

	inet port cmd [ args ... ]

##DESCRIPTION

inet opens a tcp socket, binding to port, and listens for connections. When a
new connection is accepted, inet forks, connects the client to stdin and stdout
then calls execve to spawn cmd.

##NOTES

Since PATH is not consulted, the full path to cmd must be specified, e.g.:

	inet 80 $(command -v awk) -f httpd.awk

##SEE ALSO

inetd(8)
