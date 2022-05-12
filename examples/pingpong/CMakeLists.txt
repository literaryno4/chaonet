add_executable(client client.cpp)
target_link_libraries(client chao_net)

add_executable(server server.cpp)
target_link_libraries(server chao_net)

add_executable(bench bench.cpp)
target_link_libraries(bench chao_net)