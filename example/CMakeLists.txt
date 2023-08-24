add_executable(client client.cc)
add_executable(proxyRun proxyRun.cc)
add_executable(userMasterRun userMasterRun.cc)
add_executable(UserFollowerRun UserFollowerRun.cc)


target_link_libraries(client Tiny_ChatRoom)
target_link_libraries(proxyRun Tiny_ChatRoom)
target_link_libraries(userMasterRun Tiny_ChatRoom)
target_link_libraries(UserFollowerRun Tiny_ChatRoom)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)