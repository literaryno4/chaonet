project(chao_net)

file(GLOB_RECURSE CHAONET_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(${PROJECT_NAME} ${CHAONET_SRCS})
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
