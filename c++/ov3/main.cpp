#include <iostream>
#include <asio.hpp>
#include <thread>
#include <fstream>

class Tcpserver{
    struct httprequest{
        std::string request_method;
        std::string path;
        std::string http_ver;
    };

public:
    asio::awaitable<void> start(){
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::acceptor acceptor(executor, {asio::ip::tcp::v6(), 3000});

        for(;;){
            auto socket = co_await acceptor.async_accept(asio::use_awaitable);
            asio::co_spawn(executor, handle_request(std::move(socket)), asio::detached);
        }
    }

    asio::awaitable<void> handle_request(asio::ip::tcp::socket socket){
        for(;;){
            std::string buf;
            auto bytes_transferred = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf), "\r\n", asio::use_awaitable);

            httprequest req = parse_request(buf);

            if(req.request_method != "GET" || req.http_ver != "HTTP/1.1") {
                co_await asio::async_write(socket, asio::buffer(req.http_ver + " 400 Bad Request"), asio::use_awaitable);
                continue;
            }

            std::ifstream html_file(get_path(req.path));
            std::stringstream html_stream;
            html_stream << html_file.rdbuf();
            std::string html = html_stream.str();
            std::string res;

            res += "HTTP/1.1 200 OK \n";
            res += "content-type: text/html \n";
            res +="content-length: " + std::to_string(html.length()) + "\n\n";
            res+= html;

            bytes_transferred = co_await asio::async_write(socket, asio::buffer(res), asio::use_awaitable);
        }
    }

    httprequest parse_request(const std::string &raw_req){
        std::vector<std::string> split_string;
        httprequest req;
        std::stringstream req_stream(raw_req);std::string req_item;

        while(std::getline(req_stream, req_item, ' ')){
            int newline = req_item.find("\r\n");
            if(newline != -1){
                req_item.erase(newline, req_item.size() - newline);
            }

            split_string.push_back(req_item);
        }

        req.request_method = split_string[0];
        req.path = split_string[1];
        req.http_ver = split_string[2];

        return req;
    }

    static std::string get_path(const std::string &req_path){
        if(req_path == "/"){
            return "../index.html";
        }

        if(req_path == "/page1"){
            return "../page1.html";
        }

        if(req_path == "/page2"){
            return "../page2.html";
        }

        return "../404.html";
    }
};
int main() {
    asio::io_context event_loop(1);
    Tcpserver tcpserver;
    asio::co_spawn(event_loop, tcpserver.start(), asio::detached);
    event_loop.run();

    return 0;
}
