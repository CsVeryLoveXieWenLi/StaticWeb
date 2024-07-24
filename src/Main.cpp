/*
 * @Author: CsVeryLoveXieWenLi
 * @Date: 2024-07-24 13:35:16
 * @LastEditTime: 2024-07-24 21:41:15
 * @Description: 主入口
 * @Sign: 有些故事，总是美妙又缥缈，郁郁不得终。
 * Copyright (c) 2024 by CsVeryLoveXieWenLi, All Rights Reserved.
 */

// #define CINATRA_ENABLE_GZIP   1
// #define CINATRA_ENABLE_BROTLI 1

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#define BUFFER_SIZE 1024 * 100


#include <cinatra.hpp>


using namespace cinatra;


// Tools
async_simple::coro::Lazy<void> sends(coro_http_response& response, std::filesystem::path& path) {
    // Type
    std::string      _path     = path.string();
    std::string_view extension = get_extension(_path);
    std::string_view mime      = get_mime_type(extension);

    response.add_header("content-type", mime.data());


    // Format
    response.set_format_type(format_type::chunked);


    // Open
    coro_io::coro_file file{};
    file.open(_path, std::ios::in);


    // Read And Send
    bool        ok;
    bool        end;
    std::string buffer;

    // reserve
    buffer.reserve(BUFFER_SIZE);

    // start
    ok = co_await response.get_conn()->begin_chunked();
    if (!ok) {
        file.close();
        co_return;
    }

    // read
    while (true) {
        auto [_, size] = co_await file.async_read(buffer.data(), BUFFER_SIZE);

        end = file.eof();
        ok  = co_await response.get_conn()->write_chunked(std::string_view(buffer.data(), size), end);

        if (!ok || end) {
            file.close();
            co_return;
        }
    }
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