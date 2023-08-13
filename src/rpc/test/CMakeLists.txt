add_executable(callerTest callerTest.cc user.pb.cc)
add_executable(rpcServerTest rpcServerTest.cc user.pb.cc)


target_link_libraries(callerTest Tiny_ChatRoom)
target_link_libraries(rpcServerTest Tiny_ChatRoom)
 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/rpc/test)