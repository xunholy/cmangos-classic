#
# CMaNGOS Autoscale Module
# Located at: src/modules/autoscale/
#

set(LIBRARY_NAME autoscale)

file(GLOB autoscale_source ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
  source_group("autoscale" FILES ${autoscale_source})

set(LIBRARY_SRCS ${autoscale_source})

if (BUILD_PLAYERBOTS)
  include_directories(${CMAKE_SOURCE_DIR}/src/modules/PlayerBots)
  include_directories(${CMAKE_SOURCE_DIR}/dep/g3dlite)
  add_definitions(-DENABLE_PLAYERBOTS)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/modules/modules/src)
add_definitions(-DENABLE_MODULES)
add_definitions(-DENABLE_AUTOSCALE)

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

set_target_properties(${LIBRARY_NAME} PROPERTIES PROJECT_LABEL "Autoscale")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Modules")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/autoscale.conf.dist.in ${CMAKE_CURRENT_BINARY_DIR}/autoscale.conf.dist)
if (NOT CONF_INSTALL_DIR)
  set(CONF_INSTALL_DIR ${CONF_DIR})
endif()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/autoscale.conf.dist DESTINATION ${CONF_INSTALL_DIR})
