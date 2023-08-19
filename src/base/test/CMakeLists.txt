add_executable(redisTest redisTest.cc) 

target_link_libraries(redisTest Tiny_ChatRoom) 
 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/base/test)