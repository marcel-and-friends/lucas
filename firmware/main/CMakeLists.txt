idf_component_register(
    SRCS
        main.cpp
        maestro/Maestro.cpp
        maestro/Bridge.cpp
        heater/Task.cpp
        heater/TemperatureSensor.cpp
    INCLUDE_DIRS
        "."
    PRIV_REQUIRES
        # ESP-IDF components
        bt
        esp_adc
        # Custom components
        ads1115
        etk
        wxs
        xf
        # Third-party libraries
        nanopb
)

file(GLOB_RECURSE PROTO_FILES ../../shared/proto/*.proto)

# Generate oneof fields as anonymous unions
set(NANOPB_OPTIONS "-s anonymous_oneof:true")
# Since the files are interconnected through `import` statements, we want changes to one protobuf file to invalidate every generated file.
# See https://github.com/nanopb/nanopb/issues/1083#issuecomment-2824873956 for more context.
set(NANOPB_DEPENDS ${PROTO_FILES})

find_package(Nanopb REQUIRED)

nanopb_generate_cpp(
    TARGET proto
    RELPATH ../../shared
    ${PROTO_FILES}
)

target_link_libraries(${COMPONENT_LIB} PRIVATE proto)
