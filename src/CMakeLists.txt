file(
	GLOB_RECURSE _source_list
	LIST_DIRECTORIES false
	"${CMAKE_CURRENT_SOURCE_DIR}/*.c*"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h*"
)

add_executable(FallingSand3D ${_source_list})

foreach(_source IN ITEMS ${_source_list})
	get_filename_component(_source_path "${_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_path}")
	string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
	source_group("${_group_path}" FILES "${_source}")
endforeach()

set_property(TARGET FallingSand3D PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:FallingSand3D>")

target_include_directories(FallingSand3D PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(FallingSand3D vma glm tinyobjloader imgui stb_image)

target_link_libraries(FallingSand3D Vulkan::Vulkan sdl2)

add_dependencies(FallingSand3D Shaders)
