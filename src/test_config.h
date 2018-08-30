
#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <string>

std::string get_web_server();

std::string get_dev_access();

void set_dev_access_servers(const char* servers);

void set_devconn_servers(const char* servers);
void get_devconn_server(char *ip, int* port);

void set_alerts_servers(const char* servers);
std::string get_alerts_server();

void set_file_servers(const char* servers);
std::string get_fileserver();

void set_devmng_servers(const char* servers);
std::string get_devmng_server();

void mark_invalid_server(const char* server);

#endif /* end of include guard: TEST_CONFIG_H */
