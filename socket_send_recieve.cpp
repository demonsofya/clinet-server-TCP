#include "socket_send_recieve.hpp"

#include <cerrno>
#include <stdexcept>

#include <sys/socket.h>

// Send all bytes from buffer
void send_all(int fd, const char *data, std::size_t size) {
    while (size > 0) {
        ssize_t n = send(fd, data, size, MSG_NOSIGNAL);

        if (n <= 0) {
            throw std::runtime_error("send returned n <= 0");
        }

        data += n;
        size -= static_cast<std::size_t>(n);
    }
}

// Read string till \n
std::string recv_line(int fd) {
    std::string line;

    while (line.size() < 128) {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);

        if (n <= 0) {
            throw std::runtime_error("recv reterned n <= 0");
        }

        if (ch == '\n') {
            return line;
        }

        line += ch;
    }

    throw std::runtime_error("line is too long");
}