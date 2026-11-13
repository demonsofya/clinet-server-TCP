#include "socket_send_recieve.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

bool receive_file(int server, const std::string &copy_path, long long file_size) {
    int copy_fd = -1;
    bool success = true;

    try {
        copy_fd = open(copy_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (copy_fd == -1) {
            throw std::runtime_error("Cannot create output file");
        }

        char buffer[4096];
        long long remaining = file_size;

        while (remaining > 0) {
            std::size_t count = std::min<long long>(remaining, sizeof(buffer));

            ssize_t received = recv(server, buffer, count, 0);

            if (received <= 0) {
                throw std::runtime_error("Cannot receive complete file");
            }

            if (write(copy_fd, buffer, received) != received) {
                throw std::runtime_error("Cannot write file");
            }

            remaining -= received;
        }

    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        success = false;
    }

    if (copy_fd != -1 && close(copy_fd) == -1) {
        std::cerr << "Cannot close output file\n";
        success = false;
    }

    return success;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./client_app <server-ip> <file1> [file2 ...]\n";
        return 1;
    }

    int server = -1;
    int result = 0;

    try {
        server = socket(AF_INET, SOCK_STREAM, 0);

        if (server == -1) {
            throw std::runtime_error("Cannot create socket");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);

        if (inet_pton(AF_INET, argv[1], &address.sin_addr) != 1) {
            throw std::runtime_error("Invalid IP address");
        }

        if (connect(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
            throw std::runtime_error("Cannot connect");
        }

        send_all(server, "GET\n", 4);

        int i = 2;

        while (true) {
            std::string file_name = recv_line(server);

            if (file_name == "END") {
                if (i != argc) {
                    throw std::runtime_error("Server sent fewer files than expected");
                }
                break;
            }

            if (file_name.compare(0, 3, "OK ") != 0) {
                throw std::runtime_error("Invalid response");
            }

            if (i >= argc) {
                throw std::runtime_error("Not enough output filenames");
            }

            long long file_size = std::stoll(file_name.substr(3));
            std::string copy_path = "client/" + std::string(argv[i]);

            if (!receive_file(server, copy_path, file_size)) {
                throw std::runtime_error("File transfer failed");
            }

            std::cout << "File received: " << copy_path << '\n';
            ++i;
        }

    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }

    if (server != -1) {
        close(server);
    }

    return result;
}