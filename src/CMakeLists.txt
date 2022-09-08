add_library(NPP
        network/NetworkManager.cpp
        basic/Bytes.cpp
        basic/Exception.cpp
        basic/Utils.cpp
        basic/Thread.cpp
        basic/Logger.cpp
        task/ElasticExecutor.cpp
        )

target_link_libraries(NPP fmt::fmt tbb -lasan)
target_compile_options(NPP PUBLIC -DDEBUG -fsanitize=address)