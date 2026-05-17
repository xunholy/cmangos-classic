#
# CMaNGOS VIP Module
# Located at: src/modules/vip/
#

set(LIBRARY_NAME vip)

# Collect source files
file(GLOB vip_source ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
  source_group("vip" FILES ${vip_source})

# Define base source library
set(LIBRARY_SRCS ${vip_source})

# Define ENABLE_PLAYERBOTS if need
if (BUILD_PLAYERBOTS)
  include_directories(${CMAKE_SOURCE_DIR}/src/modules/PlayerBots)
  include_directories(${CMAKE_SOURCE_DIR}/dep/g3dlite)
  add_definitions(-DENABLE_PLAYERBOTS)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/modules/modules/src)
add_definitions(-DENABLE_MODULES)
add_definitions(-DENABLE_VIP)

# Define Expansion
if ( ${CMAKE_PROJECT_NAME} MATCHES "Classic")
  add_definitions(-DEXPANSION=0)
endif()
if ( ${CMAKE_PROJECT_NAME} MATCHES "TBC")
  add_definitions(-DEXPANSION=1)
endif()
if ( ${CMAKE_PROJECT_NAME} MATCHES "WoTLK")
  add_definitions(-DEXPANSION=2)
endif()

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SRCS})

target_link_libraries(${LIBRARY_NAME}
  modules
  shared
  RecastNavigation::Detour
)

if(UNIX)
  if (NOT (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD"))
    target_link_libraries(${LIBRARY_NAME} dl)
  endif()
endif()

set_target_properties(${LIBRARY_NAME} PROPERTIES PROJECT_LABEL "VIP")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Modules")

# Install config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/vip.conf.dist.in ${CMAKE_CURRENT_BINARY_DIR}/vip.conf.dist)
if (NOT CONF_INSTALL_DIR)
  set(CONF_INSTALL_DIR ${CONF_DIR})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vip.conf.dist DESTINATION ${CONF_INSTALL_DIR})
