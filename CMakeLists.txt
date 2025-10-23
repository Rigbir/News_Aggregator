cmake_minimum_required(VERSION 3.12...3.31)
project(news_aggregator_service CXX)

# --------------------------
# Dependencies
# --------------------------
# ZLIB
set(ZLIB_ROOT "/usr/local/Cellar/zlib/1.3.1")
set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}/include")
set(ZLIB_LIBRARY "${ZLIB_ROOT}/lib/libz.dylib")

# ICU
set(ICU_ROOT "/usr/local/opt/icu4c")
set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
set(ICU_LIB_DIR "${ICU_ROOT}/lib")

# Add linking paths
link_directories(${ICU_LIB_DIR})
include_directories(${ICU_INCLUDE_DIR})

# --------------------------
# Userver
# --------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core QUIET)
if(NOT userver_FOUND)
  download_userver(TRY_DIR third_party/userver)
endif()
userver_setup_environment()

# --------------------------
# Find ICU libraries
# --------------------------
find_library(ICU_DATA_LIB icudata PATHS ${ICU_LIB_DIR} NO_DEFAULT_PATH)
find_library(ICU_UC_LIB icuuc PATHS ${ICU_LIB_DIR} NO_DEFAULT_PATH)
find_library(ICU_I18N_LIB icui18n PATHS ${ICU_LIB_DIR} NO_DEFAULT_PATH)

if(NOT ICU_DATA_LIB OR NOT ICU_UC_LIB OR NOT ICU_I18N_LIB)
  message(FATAL_ERROR "ICU libraries not found in ${ICU_LIB_DIR}")
endif()

# --------------------------
# Main executable (StorageService)
# --------------------------
        add_executable(${PROJECT_NAME} 
            src/storage/main.cpp
            src/storage/postgres_client.cpp
            src/handlers/health_handler.cpp
            src/handlers/news_latest_handler.cpp
            src/handlers/news_add_handler.cpp
        )

target_include_directories(${PROJECT_NAME} PRIVATE /usr/local/include/postgresql@14)
target_link_directories(${PROJECT_NAME} PRIVATE /usr/local/lib/postgresql@14)
target_link_libraries(${PROJECT_NAME} PRIVATE
    userver::core
    ${ICU_DATA_LIB}
    ${ICU_UC_LIB}
    ${ICU_I18N_LIB}
    pq
)
target_compile_definitions(${PROJECT_NAME} PRIVATE USERVER_NAMESPACE=)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/userver)

        # --------------------------
        # CollectorService executable
        # --------------------------
        add_executable(collector_service 
            src/collector/main.cpp
            src/collector/collector_service.cpp
            src/collector/http_client.cpp
            src/collector/rss_parser.cpp
            src/handlers/collector_status_handler.cpp
        )

        target_link_libraries(collector_service PRIVATE
            userver::core
            ${ICU_DATA_LIB}
            ${ICU_UC_LIB}
            ${ICU_I18N_LIB}
            curl
        )
target_compile_definitions(collector_service PRIVATE USERVER_NAMESPACE=)
target_include_directories(collector_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/userver)

# ParserService removed - not used in current implementation

        # --------------------------
        # API Gateway executable
        # --------------------------
        add_executable(api_gateway 
            src/gateway/main.cpp
            src/gateway/redis_client.cpp
            src/gateway/http_client.cpp
            src/handlers/news_handler.cpp
            src/handlers/health_handler.cpp
        )

        target_include_directories(api_gateway PRIVATE /usr/local/include)
        target_link_directories(api_gateway PRIVATE /usr/local/lib)
        target_link_libraries(api_gateway PRIVATE
            userver::core
            ${ICU_DATA_LIB}
            ${ICU_UC_LIB}
            ${ICU_I18N_LIB}
            hiredis
            curl
        )
        target_compile_definitions(api_gateway PRIVATE USERVER_NAMESPACE=)
        target_include_directories(api_gateway PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/userver)

# Simple Collector removed - no longer needed

# --------------------------
# Configs
# --------------------------
file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml
                        ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

# --------------------------
# Установка
# --------------------------
include(GNUInstallDirs)
if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

install(TARGETS ${PROJECT_NAME} collector_service api_gateway DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})
