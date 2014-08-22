int socket_connect(const char *address);
int socket_server(const char *address);
int socket_accept(int server_fd);
int socket_server_once(const char *address);
void socket_set_timeout(int s, int sec);
