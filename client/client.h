
/*
 * RAID Client I/O
 *
 * Establishes network based connection for read/write with machine given user arguments
 *
 */

typedef struct server_info {
    char* ip;
    char* port;
} server_info_t;

int connect_controller(server_info_t* info);

void read_file(int socket);
void write_file(int socket);
