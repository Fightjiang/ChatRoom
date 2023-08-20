add_executable(redisTest redisTest.cc) 
add_executable(zookeeperTest zookeeperTest.cc) 

target_link_libraries(redisTest Tiny_ChatRoom) 
target_link_libraries(zookeeperTest Tiny_ChatRoom) 
 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/base/test)