cmake_minimum_required(VERSION 3.3)

project(MPMCThreadPool)

option(ATTACH_SOURCES "When generating an IDE project, add MPMCThreadPool header files to project sources." OFF)



set(extern_dir ${CMAKE_CURRENT_SOURCE_DIR}/extern)



set(hdr_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(hdr_inline_files
	${hdr_dir}/MPMCThreadPool/inlines/MPMCThreadPool.inl
)
set_source_files_properties(${hdr_inline_files} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "sourcecode.cpp.h")
source_group("MPMCThreadPool\\inlines" FILES ${hdr_inline_files})

set(hdr_main_files
	${hdr_dir}/MPMCThreadPool/MPMCThreadPool.hpp
)
source_group("MPMCThreadPool" FILES ${hdr_main_files})

set(all_hdr ${hdr_inline_files} ${hdr_main_files})



add_library(${PROJECT_NAME} INTERFACE)

set(required_cxx_features
	cxx_alias_templates
	cxx_auto_type
	cxx_constexpr
	cxx_decltype
	cxx_default_function_template_args
	cxx_defaulted_functions
	cxx_defaulted_move_initializers
	cxx_deleted_functions
	cxx_lambdas
	cxx_noexcept
	cxx_nonstatic_member_init
	cxx_nullptr
	cxx_range_for
	cxx_right_angle_brackets
	cxx_rvalue_references
	cxx_static_assert
	cxx_thread_local
	cxx_variadic_templates
)

target_compile_features(${PROJECT_NAME} INTERFACE ${required_cxx_features})

target_include_directories(${PROJECT_NAME} INTERFACE ${hdr_dir} ${extern_dir})

if(ATTACH_SOURCES)
	target_sources(${PROJECT_NAME} INTERFACE ${all_hdr})
endif()
