add_executable(simple simple.cpp)
target_link_libraries(simple ${ZMQ_LIBRARY})

add_executable(bimodal bimodal.cpp)
target_link_libraries(bimodal ${ZMQ_LIBRARY})
