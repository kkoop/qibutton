project(qibutton)
FIND_PACKAGE(Qt5Widgets REQUIRED)

SET(qibutton_SOURCES main.cpp mainwindow.cpp)
SET(qibutton_HEADERS mainwindow.h)
SET(qibutton_FORMS main.ui about.ui)

QT5_WRAP_CPP(qibutton_HEADERS_MOC ${qibutton_HEADERS})
QT5_WRAP_UI(qibutton_FORMS_HEADERS ${qibutton_FORMS})

message(STATUS ${qibutton_FORMS_HEADERS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
include_directories(..)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(qibutton ${qibutton_SOURCES} ${qibutton_HEADERS_MOC} ${qibutton_FORMS_HEADERS}
                  ../ds1922.cpp ../ds9490.cpp)
target_link_libraries(qibutton usb Qt5::Widgets)
