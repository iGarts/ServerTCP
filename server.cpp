#include "json.hpp"

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <memory>

#define PORT 9999
#define NEWLINE "\r\n"

using string = std::string;
using json = nlohmann::json;

static int is_running = true;

char   get_choise_flag();
void   print_json(const json &body, uint8_t ident);
void   signal_callback_handler(int signum);
json   get_request_body(const char *buffer);
string set_response_body(json body_json, uint8_t ident);
string build_http_response(string status_code, string body);


int main()
{
    signal(SIGINT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);

    auto auto_close_fd = [](int *fd)
    {
        if (fd != nullptr && *fd != 0)
        {
            close(*fd);
            std::cout <<"closing: " <<*fd <<std::endl;
            delete fd;
        }
    };

    //create socket (IPv4, TCP)
    std::unique_ptr <int, decltype(auto_close_fd)>
            sock_fd(new int(socket(AF_INET, SOCK_STREAM, 0)), auto_close_fd);

    if (*sock_fd == -1)
    {
        std::cerr << "Can't create a socket! Quitting" << std::endl;
        return 1;
    }

    int enable = 1;
    if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        return 1;
    }

    //set socket options
    sockaddr_in sockaddr;
    sockaddr.sin_port = htons(PORT);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    if (int err = bind(*sock_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cout << "Bind to port failed: " << err <<std::endl;
        return 2;
    }

    //start listening
    std::cout << "Server is running.\n\n" << "Start Listening...\n\n";
    if (listen(*sock_fd, SOMAXCONN) < 0)
    {
        std::cout << "Listen on socket failed" << std::endl;
        return 3;
    }

    char buffer[4096];
    socklen_t addrlen = sizeof(sockaddr);

    while (is_running)
    {
        //manually confirm each connection
        //char switch_flag = get_choise_flag();
        //if (switch_flag == 'q')
        //{
        //  std::cout << "See you later!" << std::endl;
        //  return 0;
        //}

        //accepting a connection from queue
        std::cout << "Waiting for connection..." << std::endl;
        int connection = accept(*sock_fd, (struct sockaddr *)&sockaddr, &addrlen);
        if (connection)
        {
            //read from connection
            std::cout << "Сonnection established" << std::endl;
            memset(buffer, 0, sizeof(buffer));
            auto bytes_read = read(connection, buffer, sizeof(buffer));
            if (bytes_read == -1)
            {
                std::cout << "byte read error" << std::endl;
                close(connection);
                break;
            }

            //build a response body JSON
            json request_body_json  = get_request_body(buffer);
            json response_body_json = request_body_json;
            response_body_json["success"] = true;
            response_body_json["id"] = "id_228_1488";
            if  (request_body_json["success"] == false)
                 response_body_json["success"] = false;
            else response_body_json["success"] = true;

            //print request and response data to console
            print_json(request_body_json,  4);
            print_json(response_body_json, 4);

            //build a HTTP response
            string response_http = set_response_body(response_body_json, 4);

            //send a message and close connection
            auto bytes_send = send(connection, response_http.c_str(), response_http.size(), 0);
            if  (bytes_send == -1)
            {
                std::cout << "byte send error" << std::endl;
                close(connection);
                break;
            }
            close(connection);
        }
        else if (!connection)
        {
            std::cout << "connection error" << std::endl;
            break;
        }
    }
}


json get_request_body(const char *buffer)
{
    const char *json_start = strchr(buffer, '{');
    json request_body = json::parse(json_start);
    return request_body;
}

string set_response_body(json body_json, uint8_t ident)
{
    return string(build_http_response("200 OK", body_json.dump(ident)));
}

string build_http_response(string status_code, string body)
{
    return string("HTTP/1.1 " + status_code + NEWLINE
                  "Content-Type: application/json"
                  NEWLINE NEWLINE + body + NEWLINE);
}

void print_json(const json &body, uint8_t ident)
{
    std::cout << body.dump(ident) << std::endl;
}

void signal_callback_handler(int signum)
{
   std::cout << "Caught signal: " << signum << std::endl;
   is_running = false;
   exit(0);
}

char get_choise_flag()
{
    char choise;
    std::cout << "Please, select aviable option!\n"
              << "Press Y to start connection\n"
              << "Press Q to exit" << std::endl;
    std::cin  >> choise;
    while (tolower(choise) != 'y' && tolower(choise) != 'q')
    {
        std::cout << "Please, select aviable option!\n"
                  << "Press Y to start connection\n"
                  << "Press Q to exit" << std::endl;
        std::cin >> choise;
    }
    return choise;
}
