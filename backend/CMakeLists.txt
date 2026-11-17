cmake_minimum_required(VERSION 3.15)
project(CreditCardFraudDetection LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific socket & network libraries
if(WIN32)
    add_definitions(-DASIO_STANDALONE)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(PLATFORM_LIBS ws2_32 mswsock)
else()
    set(PLATFORM_LIBS pthread)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

# Core library sources (shared between server and unit tests)
set(CORE_SOURCES
    src/database.cpp
    src/model.cpp
    src/fraud_detector.cpp
    third_party/sqlite/sqlite3.c
)

# -------------------------------------------------------------------
# Backend REST API Server Executable
# -------------------------------------------------------------------
add_executable(fraud_detection_backend
    src/main.cpp
    src/server.cpp
    src/api.cpp
    ${CORE_SOURCES}
)

target_link_libraries(fraud_detection_backend PRIVATE ${PLATFORM_LIBS})

# -------------------------------------------------------------------
# Unit and Integration Test Runner Executable
# -------------------------------------------------------------------
add_executable(test_backend
    tests/test_backend.cpp
    ${CORE_SOURCES}
)

target_link_libraries(test_backend PRIVATE ${PLATFORM_LIBS})
