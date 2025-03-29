idf_component_register(
    SRCS
        main.cpp
        heater/Heater.cpp
        maestro/Maestro.cpp
        maestro/Bridge.cpp
    INCLUDE_DIRS
        "."
    PRIV_REQUIRES
        # ESP-IDF components
        bt
        esp_driver_gpio
        esp_driver_uart
        nvs_flash
        # Custom components
        etk
        wxs
        xf
        # Third-party libraries
        nanopb
)

find_package(Nanopb REQUIRED)

file(GLOB_RECURSE PROTO_FILES ../../shared/proto/*.proto)

nanopb_generate_cpp(
    TARGET proto
    RELPATH ../../shared
    ${PROTO_FILES}
)

target_link_libraries(${COMPONENT_LIB} PRIVATE proto)
