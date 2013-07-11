# swatbotics/hubo-logging/src/CMakeLists.txt

add_library(mrdplot mrdplot.c LogWriter.cpp LogReader.cpp)

add_executable(test_cwrite test_cwrite.c)
target_link_libraries(test_cwrite mrdplot m)

add_executable(test_cread test_cread.c)
target_link_libraries(test_cread mrdplot)

add_executable(test_cppwrite test_cppwrite.cpp)
target_link_libraries(test_cppwrite mrdplot)

add_executable(test_cppread test_cppread.cpp)
target_link_libraries(test_cppread mrdplot)

if (HAVE_HUBO_ACH)
add_executable(hubo-logger hubo-logger.cpp)
target_link_libraries(hubo-logger mrdplot huboparams ach)
endif (HAVE_HUBO_ACH)

