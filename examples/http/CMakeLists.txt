set(http_SRCS
        HttpServer.cpp
        HttpResponse.cpp
        HttpContext.cpp
        )

add_library(chao_http ${http_SRCS})
target_link_libraries(chao_http chao_net)

set(HEADERS HttpContext.h
        HttpRequest.h
        HttpResponse.h
        HttpServer.h
        )

add_executable(httpserver_test tests/HttpServer_test.cc)
target_link_libraries(httpserver_test chao_http)