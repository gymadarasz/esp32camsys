idf_component_register(SRCS "app_main.c"
                    INCLUDE_DIRS ".")

# Import myesp targets 
add_subdirectory(lib/myesp)
target_link_libraries(${COMPONENT_LIB} PUBLIC myesp)