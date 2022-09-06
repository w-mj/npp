add_library(NPP
        network/NetworkManager.cpp
        basic/Bytes.cpp
        basic/Exception.cpp
        basic/Utils.cpp
        basic/Thread.cpp
        )

target_link_libraries(NPP fmt::fmt tbb)