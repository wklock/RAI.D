
/*
 * RAID Client I/O
 *
 * Establishes network based connection for read/write with machine given user arguments
 *
 */

void connect_controller(void);

void read_file(int socket);
void write_file(int socket);
