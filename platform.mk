TUYA_PLATFORM_DIR := $(dir $(lastword $(MAKEFILE_LIST)))/../

# tuya os adapter includes
# TUYA_PLATFORM_CFLAGS := -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include
# TUYA_PLATFORM_CFLAGS += -I$(TUYA_PLATFORM_DIR)/tuyaos/tuyaos_adapter/include/hostapd

TUYA_PLATFORM_CFLAGS += -mlongcalls -Wno-frame-address -ffunction-sections -fdata-sections -Wall -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Og -fstrict-volatile-bitfields -Wno-error=unused-but-set-variable -fno-jump-tables -fno-tree-switch-conversion -std=gnu99 -Wno-old-style-declaration


