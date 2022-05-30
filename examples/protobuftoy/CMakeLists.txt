add_executable(protobuf_server server.cpp person.pb.h person.pb.cc)
target_link_libraries(protobuf_server  chao_net protobuf)

add_executable(protobuf_client client.cpp person.pb.h person.pb.cc)
target_link_libraries(protobuf_client  chao_net protobuf)

