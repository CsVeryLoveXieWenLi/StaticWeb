/*
 * @Author: CsVeryLoveXieWenLi
 * @Date: 2024-07-24 13:35:16
 * @LastEditTime: 2024-08-08 10:33:07
 * @Description: 主入口
 * @Sign: 有些故事，总是美妙又缥缈，郁郁不得终。
 * Copyright (c) 2024 by CsVeryLoveXieWenLi, All Rights Reserved.
 */

// #define CINATRA_ENABLE_GZIP   1
// #define CINATRA_ENABLE_BROTLI 1

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#define BUFFER_SIZE 1024 * 10


#include <cinatra.hpp>


using namespace cinatra;


// Tools
async_simple::coro::Lazy<void> sends(coro_http_response& response, std::filesystem::path& path) {
    // Type
    std::string      _path     = path.string();
    std::string_view extension = get_extension(_path);
    std::string_view mime      = get_mime_type(extension);

    response.add_header("content-type", mime.data());


    // Open
    std::ifstream file;
    file.open(path, std::ios::binary);

    if (!file.is_open()) co_return;


    // Read And Send
    std::istreambuf_iterator<char> beginf(file);
    std::istreambuf_iterator<char> endf;

    std::string content(beginf, endf);

    // close
    file.close();

    // send
    response.set_status_and_content_view(status_type::ok, std::move(content));
}


// Main
int main(int argc, char* argv[]) {
    // Command
    if (argc != 5) {
        printf("Use: StaticWeb [cpus] [host] [port] [dir]\n");
        return 1;
    }

    // Static
    std::filesystem::path dir(argv[4]);
    std::filesystem::path nfound = dir / "404.html";


    // Create
    coro_http_server server(atoi(argv[1]), atoi(argv[3]), argv[2], true);


    // Default
    server.set_default_handler(
        [&](coro_http_request& request, coro_http_response& response) -> async_simple::coro::Lazy<void> {
            // Is Get Method?
            if (request.get_method() != "GET" || request.is_upgrade()) co_return;


            // Get Path
            std::string   _pathname = code_utils::url_decode(request.get_url()).substr(1);
            std::u8string pathname(_pathname.begin(), _pathname.end());


            // Src File
            std::filesystem::path vpath = dir / pathname;


            // Not Exixts
            if (!std::filesystem::exists(vpath)) vpath = nfound;


            // Not Dir
            if (!std::filesystem::is_directory(vpath)) {
                co_await sends(response, vpath);
                co_return;
            }

            // Is Dir
            vpath /= "index.html";

            std::filesystem::exists(vpath) ? co_await sends(response, vpath)
                                           : response.set_status_and_content_view(status_type::method_not_allowed, "");
        }
    );


    // Run
    return server.sync_start().value();
}