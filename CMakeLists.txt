#-----------------------------------------------------------------
#-----------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)

project(bibot)

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_MULTITHREADED ON)
set(CMAKE_CXX_STANDARD 14)
set(OPENSSL_USE_STATIC_LIBS TRUE)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS -fno-pie)

find_package(Boost 1.72 COMPONENTS program_options system chrono thread filesystem REQUIRED)
find_package(OpenSSL REQUIRED)

set(Python_ADDITIONAL_VERSIONS 3.6) 
find_package(PythonLibs 3 REQUIRED)

include_directories(${PYTHON_INCLUDE_DIRS})
set(EXT_LIBRARIES /usr/lib/x86_64-linux-gnu)

include_directories(${Boost_INCLUDE_DIRS} /usr/include/python3.6m)

set(SOURCE_FILES "application.cpp" "binance.cpp" "bnc_data_types.cpp" "https_client.cpp" "logger.cpp" "main.cpp" "order.cpp" "py_wrapper.cpp" "statistic.cpp" "strategyserver.cpp" "telegram.cpp" "webhook_server.cpp" "wss_client.cpp" "wss_server.cpp")
set(HEADER_FILES "application.h" "binance.h" "bnc_data_types.h" "database.h" "events.h" "general.h" "html_messages.h" "https_client.h" "logger.h" "order.h" "py_wrapper.h" "sock4a.hpp" "statistic.h" "strategyserver.h" "telegram.h" "timer.h" "tkeyboard.h" "webhook_server.h" "wss_client.h" "wss_server.h" "utils.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "build/release")

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS} )
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} libpython3.6m.so)
target_link_libraries(${PROJECT_NAME} dl)
target_link_libraries(${PROJECT_NAME} python3.6m)
target_link_libraries(${PROJECT_NAME} ssl)
target_link_libraries(${PROJECT_NAME} crypto)
target_link_libraries(${PROJECT_NAME} sqlite3)

add_library(log4cplus STATIC IMPORTED)
set_target_properties(log4cplus PROPERTIES IMPORTED_LOCATION ${EXT_LIBRARIES}/liblog4cplus.a)
target_link_libraries(${PROJECT_NAME} log4cplus)

add_library(boost_python3 STATIC IMPORTED)
set_target_properties(boost_python3 PROPERTIES IMPORTED_LOCATION ${EXT_LIBRARIES}/libboost_python3-py36.a)
target_link_libraries(${PROJECT_NAME} boost_python3)









