#pragma once

#include <cstddef>
#include <string>

constexpr int PORT = 3490;

void send_all(int fd, const char *data, std::size_t size);
std::string recv_line(int fd);
