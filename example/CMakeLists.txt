add_executable(client client.cc)
add_executable(proxyRun proxyRun.cc)

target_link_libraries(client Tiny_ChatRoom)
target_link_libraries(proxyRun Tiny_ChatRoom)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)