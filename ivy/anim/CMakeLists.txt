
FILE(GLOB_RECURSE anim_src ${CMAKE_CURRENT_LIST_DIR}/*.*)

set(anim_inc
        "."
        "video"
        "video/libs"
)

idf_component_register(
        SRCS ${anim_src}
        INCLUDE_DIRS ${anim_inc}
        REQUIRES TFT_eSPI interface audio
        PRIV_REQUIRES
)