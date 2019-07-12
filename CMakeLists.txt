cmake_minimum_required(VERSION 3.0)

set(PROJ_NAME asio_sync)
project(${PROJ_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF( NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE Release )
ENDIF()

if(WIN32)
    macro(get_WIN32_WINNT version)
        if (WIN32 AND CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REPLACE "." "" ver ${ver})
            string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif()
    endmacro()

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
ENDIF()

if(MSVC)
    set(COMPILER_NAME "Visual Studio")
    add_definitions("-D_CRT_SECURE_NO_DEPRECATE")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE" )
else()
    set(COMPILER_NAME "gcc")
endif()

#############################################################################################
#       This is usefull macro to list files in the given path recurcivly.                   #
#############################################################################################                                                                                        
MACRO( source_files_tree path files)                                                        
    FILE( GLOB all_files ${path}/* )                                                        
                                                                                           
    FOREACH( each ${all_files} )                                                            
        IF( NOT IS_DIRECTORY ${each} )                                                      
            GET_FILENAME_COMPONENT( file_name ${each} NAME )                                
            LIST( APPEND ${files} "${path}/${file_name}" )                                  
        ENDIF()                                                                             
    ENDFOREACH()                                                                            
                                                                                           
    FILE( GLOB directories ${path}/*/ )                                                     
    FOREACH( each ${directories} )                                                          
        IF( IS_DIRECTORY ${each} )                                                          
            GET_FILENAME_COMPONENT( file_name ${each} NAME )                                
            source_files_tree("${path}/${file_name}" ${files})                              
        ENDIF()                                                                             
    ENDFOREACH()                                                                            
ENDMACRO()                                                                                  
                                                                                           
MACRO( source_group_tree )                                                                  
    FILE( GLOB all_files ${ARGV1}/* )                                                       
    SET( files "" )                                                                         
    FOREACH( each ${all_files} )                                                            
        IF( NOT IS_DIRECTORY ${each} )                                                      
            GET_FILENAME_COMPONENT( file_name ${each} NAME )                                
            LIST( APPEND files "${ARGV1}/${file_name}" )                                    
        ENDIF()                                                                             
    ENDFOREACH()                                                                            
    LIST( LENGTH files files_len )                                                          
    IF( ${files_len} GREATER 0 )                                                            
        STRING( REPLACE "/" "\\\\" group_name ${ARGV0} )                                    
        SOURCE_GROUP("${group_name}" FILES ${files})                                        
    ENDIF()                                                                                 
                                                                                           
    FILE( GLOB directories ${ARGV1}/*/ )                                                    
    FOREACH( each ${directories} )                                                          
        IF( IS_DIRECTORY ${each} )                                                          
            GET_FILENAME_COMPONENT( file_name ${each} NAME )                                
            source_group_tree("${ARGV0}/${file_name}" "${ARGV1}/${file_name}")              
        ENDIF()                                                                             
    ENDFOREACH()                                                                            
ENDMACRO()                                                                                                                                                                          


#############################################################################################
#                   Conan package manager corresponding commands                            #
#############################################################################################
if(MSVC)
    execute_process( COMMAND conan install ${CMAKE_SOURCE_DIR} --install-folder ${CMAKE_BINARY_DIR} --build=missing -s arch=x86_64 -s build_type=${CMAKE_BUILD_TYPE} )
endif(MSVC)                
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)                                           
conan_basic_setup()                                                                         
                                                                                           

#file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/main/cpp/${PROJ_NAME}/src/*.cpp)
#file(GLOB HEADER_FILES ${PROJECT_SOURCE_DIR}/src/main/cpp/${PROJ_NAME}/include/*.h ${PROJECT_SOURCE_DIR}/src/main/cpp/${PROJ_NAME}/include/*.hpp)

SET( PROJ_HEADER "" )
SET( PROJ_SOURCE "" )
source_files_tree(include PROJ_HEADER)
source_files_tree(src PROJ_SOURCE)
source_group_tree("Header" include)
source_group_tree("Source" src)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(${PROJ_NAME} ${SRC_FILES} ${PROJ_SOURCE} ${PROJ_HEADER})
target_link_libraries(${PROJ_NAME} ${CONAN_LIBS} )