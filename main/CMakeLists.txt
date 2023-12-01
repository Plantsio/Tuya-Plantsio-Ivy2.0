
set(srcs "main.cpp")

idf_component_register(SRCS "${srcs}"
        REQUIRES arduino nvs_flash tuya)