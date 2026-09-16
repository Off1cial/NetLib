cmake_minimum_required(VERSION 3.16)

project(netproto C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(_POSIX_C_SOURCE=200809L)
# --------------------------------------------------
# Shared netcode
# --------------------------------------------------

file(GLOB_RECURSE NET_SOURCES
    src/net/*.c
)

file(GLOB_RECURSE SHARED_SOURCES
    src/common/*.c
)

add_library(net STATIC
    ${NET_SOURCES}
    ${SHARED_SOURCES}
)

target_include_directories(net PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# --------------------------------------------------
# Client
# --------------------------------------------------

file(GLOB_RECURSE CLIENT_SOURCES
    src/client/*.c
    #${SHARED_SOURCES}
)

add_executable(client
        ${CLIENT_SOURCES}
)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(client PRIVATE
    net
)

# --------------------------------------------------
# Server
# --------------------------------------------------

file(GLOB_RECURSE SERVER_SOURCES
    src/server/*.c
)

add_executable(server
    ${SERVER_SOURCES}
    #${SHARED_SOURCES}
)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(server PRIVATE
    net
)

# --------------------------------------------------
# Warnings
# --------------------------------------------------

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(net PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(client PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(server PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# --------------------------------------------------
# Debug
# --------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(net PRIVATE
        -g
        -O0
        -fsanitize=address,undefined
    )

    target_compile_options(client PRIVATE
        -g
        -O0
        -fsanitize=address,undefined
    )

    target_compile_options(server PRIVATE
        -g
        -O0
        -fsanitize=address,undefined
    )

    target_link_options(client PRIVATE
        -fsanitize=address,undefined
    )

    target_link_options(server PRIVATE
        -fsanitize=address,undefined
    )
endif()
