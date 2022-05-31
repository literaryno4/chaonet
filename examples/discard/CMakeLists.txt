add_executable(discard_client client.cpp)
target_link_libraries(discard_client chao_net)

add_executable(discard_server server.cc)
target_link_libraries(discard_server chao_net)
