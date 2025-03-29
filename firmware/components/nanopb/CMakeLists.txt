if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    include(FetchContent)

    FetchContent_Declare(
        nanopb
        GIT_REPOSITORY https://github.com/nanopb/nanopb.git
        GIT_TAG        0.4.9.1
        GIT_PROGRESS   TRUE
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(nanopb)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${nanopb_SOURCE_DIR}/extra" PARENT_SCOPE)
endif()

idf_component_register()

target_link_libraries(${COMPONENT_LIB} INTERFACE nanopb)
