project(UI CXX)

message(STATUS "Configure ${PROJECT_NAME}")

# Options
option(${Cryptox_ID}_WX_STATIC  "Build executable with static wxWidgets" ON)

file(GLOB ${PROJECT_NAME}_src *.cpp)
file(GLOB ${PROJECT_NAME}_h *.h)

add_library(${PROJECT_NAME} SHARED
	${${PROJECT_NAME}_src}
	${${PROJECT_NAME}_h}
)

# Define output directories
set_target_properties(${PROJECT_NAME}
    PROPERTIES
	OUTPUT_NAME "cryptox"
	PROJECT_LABEL "Graphical User Interface"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
