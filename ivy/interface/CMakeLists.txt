
FILE(GLOB_RECURSE if_src ${CMAKE_CURRENT_LIST_DIR}/*.*)

set(if_inc
        .
)

idf_component_register(
        SRCS ${if_src}
        INCLUDE_DIRS ${if_inc}
        REQUIRES arduino TFT_eSPI
        PRIV_REQUIRES
)