# Copyright (c) 2016 Giorgio Marcias
#
# This software is subject to the simplified BSD license.
#
# Author: Giorgio Marcias
# email: marcias.giorgio@gmail.com



function(set_MPMCThreadPool_source_files_properties)
    get_target_property(files MPMCThreadPool INTERFACE_SOURCES)
    foreach(file IN LISTS files)
        if (file MATCHES "[.]*.inl")
            set_source_files_properties(${file} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "sourcecode.cpp.h")
        endif()
        get_filename_component(file_path_dir ${file} DIRECTORY)
        if (file_path_dir MATCHES "[.]*[/\\]MPMCThreadPool[/\\]inlines")
            source_group("MPMCThreadPool\\inlines" FILES ${file})
        elseif(file_path_dir MATCHES "[.]*[/\\]MPMCThreadPool")
            source_group("MPMCThreadPool" FILES ${file})
        endif()
    endforeach()
endfunction(set_MPMCThreadPool_source_files_properties)
