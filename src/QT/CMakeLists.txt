cmake_minimum_required(VERSION 2.8) 
project(QT_files CXX)

# MOC the headers and forms:
file(GLOB FORMS *.ui)
file(GLOB HEADERS *.h)
QT4_WRAP_CPP(HEADERS_MOC ${HEADERS})
QT4_WRAP_UI(FORMS_MOC ${FORMS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
file(GLOB SOURCE *.cpp)

add_library(QT_files ${SOURCE} ${HEADERS_MOC} ${FORMS_MOC} ${simtoi_RESOURCES_RCC})
target_link_libraries(QT_files ${QT_LIBRARIES})
