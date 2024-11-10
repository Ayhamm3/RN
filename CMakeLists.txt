cmake_minimum_required(VERSION 3.5)
project(praxis1 C)
set(CMAKE_C_STANDARD 11)

add_executable(webserver webserver.c)
set(CPACK_SOURCE_GENERATOR "TGZ") # Abgabe soll als .tar.gz erstellt werden
set(CPACK_SOURCE_IGNORE_FILES ${CMAKE_BINARY_DIR} /\\..*$)
set(CPACK_VERBATIM_VARIABLES YES) # Variablen sollen nicht opt
include(CPack) # Wir nutzen CPack um das Archiv zu erstellen
