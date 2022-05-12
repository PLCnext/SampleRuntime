cmake_minimum_required(VERSION 3.13)

project($(name))

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

################# create target #######################################################

file(GLOB_RECURSE Headers CONFIGURE_DEPENDS src/*.h src/*.hpp src/*.hxx)
file(GLOB_RECURSE Sources CONFIGURE_DEPENDS src/*.cpp)
add_executable(${CMAKE_PROJECT_NAME} ${Headers} ${Sources})

#######################################################################################

################# set install directories #############################################

string(REGEX REPLACE "^.*\\(([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+).*$" "\\1" _ARP_SHORT_DEVICE_VERSION ${ARP_DEVICE_VERSION})
set(BIN_INSTALL_DIR ${ARP_DEVICE}_${_ARP_SHORT_DEVICE_VERSION}/${CMAKE_BUILD_TYPE})

#######################################################################################

################# project include-paths ###############################################

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

#######################################################################################

################# include arp cmake module path #######################################

list(INSERT CMAKE_MODULE_PATH 0 "${ARP_TOOLCHAIN_CMAKE_MODULE_PATH}")

#######################################################################################

################# set link options ####################################################
# WARNING: Without --no-undefined the linker will not check, whether all necessary    #
#          libraries are linked. When a library which is necessary is not linked,     #
#          the firmware will crash and there will be NO indication why it crashed.    #
#######################################################################################

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE LINKER:--no-undefined)

#######################################################################################

################# add link targets ####################################################

find_package(ArpDevice REQUIRED)
find_package(ArpProgramming REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ArpDevice ArpProgramming
                      Arp.System.ModuleLib Arp.System.Module)

#######################################################################################

################# install #############################################################

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION ${BIN_INSTALL_DIR})
unset(_ARP_SHORT_DEVICE_VERSION)

#######################################################################################