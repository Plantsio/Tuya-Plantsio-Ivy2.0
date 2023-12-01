
FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

idf_component_register(SRCS "${app_sources}"
        INCLUDE_DIRS . UICustom
        REQUIRES ivy tuyaos_adapter
        PRIV_REQUIRES)
