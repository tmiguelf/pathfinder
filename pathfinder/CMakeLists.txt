cmake_minimum_required(VERSION 4.2.3)
project(pathfinder)
enable_language(CXX)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	"./src/*.cpp"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	enable_language(RC)
	set(RESOURCE_FILES  "./resources/dllVersion.rc" )
else()
	set(RESOURCE_FILES "")
endif()

add_compile_definitions(_pathfinder_EXPORTS_)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${RESOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

target_link_libraries(${PROJECT_NAME} PUBLIC CoreLib)
target_link_libraries(${PROJECT_NAME} PRIVATE pathfinder_lib version_lib)

quick_default_post_config()
