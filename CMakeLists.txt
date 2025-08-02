if(CONFIG_PAW3222)
    zephyr_library()
    zephyr_library_sources(
        src/paw3222.c
        src/paw3222_spi.c
        src/paw3222_input.c
        src/paw3222_power.c
    )
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    # Add ZMK app include directory
    if(EXISTS ${CMAKE_SOURCE_DIR}/app/include)
        zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/app/include)
    elseif(EXISTS ${CMAKE_SOURCE_DIR}/../zmk/app/include)
        zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
    endif()
endif()
