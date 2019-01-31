project("LiveBooster-linux" C)
cmake_minimum_required(VERSION 2.8)

# Create various directory in the build folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(EXECUTABLE_NAME Linux_test)
#set(CMAKE_C_FLAGS "-std=c11")

# Lib inclusion

# LiveBooster-C-Library path
set(LIVEBOOSTER_C_LIBRARY_PATH LiveBooster-C-Library)

# Create JSMN library
add_library(jsmn ${LIVEBOOSTER_C_LIBRARY_PATH}/src/jsonParser/jsmn.c)

# Create MQTTPacket library
set(MQTTPACKET_PATH ${LIVEBOOSTER_C_LIBRARY_PATH}/src/mqttClient/MQTTPacket)
file(GLOB MQTTPACKET_SOURCE ${MQTTPACKET_PATH}/*.c ${LIVEBOOSTER_C_LIBRARY_PATH}/src/mqttClient/*.c)
add_library(MQTTPacket ${MQTTPACKET_SOURCE})

# Create HeraclesGSM library
set(HERACLESGSM_PATH ${LIVEBOOSTER_C_LIBRARY_PATH}/src/heraclesGsm)
file(GLOB HERACLESGSM_SOURCE ${HERACLESGSM_PATH}/*.c)
add_library(HeraclesGSM ${HERACLESGSM_SOURCE})

# Create LiveBooster library
set(LIVEBOOSTER_PATH ${LIVEBOOSTER_C_LIBRARY_PATH}/src/liveBooster)
file(GLOB LIVEBOOSTER_SOURCE ${LIVEBOOSTER_PATH}/*.c LIVEBOOSTER_SOURCE ${LIVEBOOSTER_PATH}/liveBoosterPacket/*.c)
add_library(LiveBooster ${LIVEBOOSTER_SOURCE})

# Create Linux implementation library
set(LINUXIMPL_PATH LinuxImpl)
file(GLOB LINUXIMPL_SOURCE ${LINUXIMPL_PATH}/*.c)
add_library(linuxImpl ${LINUXIMPL_SOURCE})

# Common library list
set(COMMON_LIB_LIST LiveBooster jsmn MQTTPacket HeraclesGSM linuxImpl)

# Application
# You can change the name of the c file but don't forget to report the modification here
# Set the executable name and create it
set(LINUXAPP_PATH LinuxApp)
add_executable(${EXECUTABLE_NAME} ${LINUXAPP_PATH}/${EXECUTABLE_NAME}.c)

# Link edition configuration
target_link_libraries(${EXECUTABLE_NAME}
 ${COMMON_LIB_LIST}
)
