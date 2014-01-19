project(ibutton)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(..)
add_executable(ibutton main.cpp ../ds1922.cpp ../ds9490.cpp)
target_link_libraries(ibutton usb)
