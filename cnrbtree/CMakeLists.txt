project(cnrbtree C CXX)

set(CMAKE_C_FLAGS "-g ")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

add_executable(cpptest cpptest.cpp)

add_executable(testtree testtree.c testtree2.c)
