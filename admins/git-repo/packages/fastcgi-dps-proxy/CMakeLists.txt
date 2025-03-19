# This file compiles and installs dps-proxy fastcgi library into /usr/lib/fastcgi2/dps-proxy

cmake_minimum_required(VERSION 2.8) 

project(dps-proxy)

set(PROXY_VERSION 1.1.0)
set(PROXY_SOVERSION 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -W -Wall -g -O2 -fPIC -ftemplate-depth-256 -std=c++0x")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0")

INCLUDE(locate_library.cmake)

cmake_minimum_required(VERSION 2.8) 

find_package(Boost 1.46.1 REQUIRED COMPONENTS thread filesystem date_time)
find_package(LibXml2 REQUIRED)

LOCATE_LIBRARY(FASTGCI_DAEMON_2 "fastcgi2/request.h" "fastcgi-daemon2")
LOCATE_LIBRARY(DBPOOL "dbpool3/dbpool/handle_var.h" "mypp3")
LOCATE_LIBRARY(JANSSON "jansson.h" "jansson")
LOCATE_LIBRARY(MSGPACK "msgpack.hpp" "msgpack")

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    "/usr/include/dbpool3"
    ${FASTGCI_DAEMON_2_INCLUDE_DIR})

set(SOURCE_LIB
	src/writers/xml.cpp
	src/writers/json.cpp
	src/writers/msgpack.cpp
	src/handles/index.cpp
    src/handles/documentMetadata.cpp
	src/handles/ping.cpp
    src/mysql_fs_layer.cpp
    src/dsn.cpp
    src/proxy.cpp)

add_library(dps-proxy SHARED ${SOURCE_LIB})

set_target_properties(dps-proxy PROPERTIES VERSION "${PROXY_VERSION}" SOVERSION "${PROXY_SOVERSION}")

target_link_libraries(dps-proxy 
    ${Boost_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${DBPOOL_LIBRARIES}
    "jansson"
    "msgpack"
    "omnithread"
    "COS4"
    "COSDynamic4"
    "omniCodeSets4"
    "omniConnectionMgmt4"
    "omniDynamic4"
    "omniORB4"
    "omnisslTP4"
    "yandex-common-stl"
    ${FASTGCI_DAEMON_2_LIBRARIES})

set(LIB_INSTALL_PATH /usr/lib/fastcgi2/dps-proxy)
install(TARGETS dps-proxy DESTINATION ${LIB_INSTALL_PATH})
