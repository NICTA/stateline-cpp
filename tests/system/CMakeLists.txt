find_package(PythonInterp REQUIRED)

add_executable(test_worker test_worker.cpp)
target_link_libraries(test_worker ${ZMQ_LIBRARY})

add_test(NAME system_test
         COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py $<TARGET_FILE:test_worker>
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
