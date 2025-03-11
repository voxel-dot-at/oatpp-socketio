



#
# test me
#

add_executable(engineio_server engineio_server.cpp)
target_link_libraries(engineio_server oatpp-socketio ${LIBS})

add_executable(sio_server sio_server.cpp)
target_link_libraries(sio_server oatpp-socketio ${LIBS})
