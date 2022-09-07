
add_executable(test_exception test_exception.cpp)
target_link_libraries(test_exception NPP)

add_executable(test_network test_network.cpp)
target_link_libraries(test_network NPP)

add_executable(test_coroutine coroutine/test_coroutine.cpp)
target_link_libraries(test_coroutine NPP)