
FILE(GLOB_RECURSE anim_src ${CMAKE_CURRENT_LIST_DIR}/*.*)

set(anim_inc
        "."
        "audio"
        "audio/libs"
        "video"
        "video/libs"
)

idf_component_register(
        SRCS ${anim_src}
        INCLUDE_DIRS ${anim_inc}
        REQUIRES TFT_eSPI interface
        PRIV_REQUIRES
)