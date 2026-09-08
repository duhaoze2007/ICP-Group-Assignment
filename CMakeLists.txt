cmake_minimum_required(VERSION 4.3)
project(ICP_Group_Assignment C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(ICP_Group_Assignment
        include/admin.h
        include/auth.h
        include/common.h
        include/facility_officer.h
        include/file_handler.h
        include/instructor.h
        include/manager.h
        include/reports.h
        include/student.h
        include/utils.h
        src/admin.c
        src/auth.c
        src/facility_officer.c
        src/file_handler.c
        src/instructor.c
        src/main.c
        src/manager.c
        src/reports.c
        src/student.c
        src/utils.c)
