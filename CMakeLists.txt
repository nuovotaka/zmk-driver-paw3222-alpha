if(CONFIG_PAW3222)
    zephyr_library()
    zephyr_library_sources(
        src/paw3222.c
        src/events/layer_state_changed.c
    )
    zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
endif()
