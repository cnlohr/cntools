project(cnrbtree C CXX)

add_executable(cpptest cpptest.cpp)

add_executable(testtree testtree.c)
set_property(TARGET testtree PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
set_property(TARGET cpptest PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
