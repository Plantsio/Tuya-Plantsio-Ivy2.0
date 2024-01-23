
FILE(GLOB_RECURSE IVY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.*)

set(IVY_INCLUDES
        "anim"
        "anim/video"
        "anim/video/libs"
        "interface"
)

idf_component_register(
        SRCS ${IVY_SOURCES}
        INCLUDE_DIRS ${IVY_INCLUDES}
        REQUIRES TFT_eSPI audio arduino
        PRIV_REQUIRES
)