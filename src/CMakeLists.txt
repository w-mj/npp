add_library(npc
        network/NetworkManager.cpp
        basic/Bytes.cpp
        basic/Exception.cpp
        basic/Utils.cpp
        )

target_link_libraries(npc fmt::fmt)