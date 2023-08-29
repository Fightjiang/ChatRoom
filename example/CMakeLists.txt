add_executable(client client.cc)
add_executable(proxyRun proxyRun.cc)
add_executable(userMasterRun userMasterRun.cc)
add_executable(userFollowerRun UserFollowerRun.cc)
add_executable(test2 test.cc)
add_executable(chatFollowerRun ChatFollower.cc)



target_link_libraries(client Tiny_ChatRoom)
target_link_libraries(proxyRun Tiny_ChatRoom)
target_link_libraries(userMasterRun Tiny_ChatRoom)
target_link_libraries(userFollowerRun Tiny_ChatRoom)
target_link_libraries(chatFollowerRun Tiny_ChatRoom)
target_link_libraries(test2 Tiny_ChatRoom)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)