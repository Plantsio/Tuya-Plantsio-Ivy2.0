
idf_component_register(SRCS "TFT_eSPI.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES arduino)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DUSE_HSPI_PORT")