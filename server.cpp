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

bool send_file(int client, const std::string &file_path) {
    int file_fd = -1;
    bool success = true;

    try {
        file_fd = open(file_path.c_str(), O_RDONLY);

        if (file_fd == -1) {
            throw std::runtime_error("Cannot open input file");
        }

        struct stat finfo{};

        if (fstat(file_fd, &finfo) == -1) {
            throw std::runtime_error("Cannot get file information");
        }

        if (!S_ISREG(finfo.st_mode)) {
            throw std::runtime_error("Cannot copy dirs");
        }

        std::string header = "OK " + std::to_string(finfo.st_size) + "\n";

        send_all(client, header.data(), header.size());

        char buffer[4096];
        off_t remaining = finfo.st_size;

        while (remaining > 0) {
            std::size_t count = std::min<off_t>(remaining, sizeof(buffer));

            ssize_t bytes_read = read(file_fd, buffer, count);

            if (bytes_read <= 0) {
                throw std::runtime_error("Cannot read file");
            }

            send_all(client, buffer, bytes_read);
            remaining -= bytes_read;
        }

    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        success = false;
    }

    if (file_fd != -1) {
        close(file_fd);
    }

    return success;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./server_app <file1> [file2 ...]\n";
        return 1;
    }

    int listener = -1;
    int client = -1;
    int result = 0;

    try {
        listener = socket(AF_INET, SOCK_STREAM, 0);

        if (listener == -1) {
            throw std::runtime_error("Cannot create socket");
        }

        int reuse = 1;

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
            throw std::runtime_error("Cannot configure socket");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(PORT);
        address.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
            throw std::runtime_error("Cannot bind socket");
        }

        if (listen(listener, 1) == -1) {
            throw std::runtime_error("Cannot listen");
        }

        std::cout << "Waiting for a client...\n";

        client = accept(listener, nullptr, nullptr);

        if (client == -1) {
            throw std::runtime_error("Cannot accept client");
        }

        if (recv_line(client) != "GET") {
            throw std::runtime_error("Invalid client message");
        }

        for (int i = 1; i < argc; ++i) {
            std::string file_path = "server/" + std::string(argv[i]);

            if (!send_file(client, file_path)) {
                throw std::runtime_error("File transfer failed");
            }

            std::cout << "File sent: " << file_path << '\n';
        }

        send_all(client, "END\n", 4);

    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }

    if (client != -1) {
        close(client);
    }

    if (listener != -1) {
        close(listener);
    }

    return result;
}