add_executable(testEventLoop testEventLoop.cpp)
target_link_libraries(testEventLoop chao_net)

add_executable(testRunInLoop testRunInLoop.cpp)
target_link_libraries(testRunInLoop chao_net)

add_executable(testEventLoopThread testEventLoopThread.cpp)
target_link_libraries(testEventLoopThread chao_net)

add_executable(testAcceptor testAcceptor.cpp)
target_link_libraries(testAcceptor chao_net)

add_executable(testTcpServer testTcpServer.cpp)
target_link_libraries(testTcpServer chao_net)

add_executable(testConnector testConnector.cpp)
target_link_libraries(testConnector chao_net)

add_executable(testTcpClient testTcpClient.cpp)
target_link_libraries(testTcpClient chao_net)
