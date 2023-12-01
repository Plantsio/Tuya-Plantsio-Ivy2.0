############################################################
# config
############################################################
menuconfig:
	@if [ "$(HAS_PYTHON3)" = "" ]; then echo has no python3; exit 1; fi
	@if [ -f ./build/tuya_iot.config ]; then cp -f ./build/tuya_iot.config ./.config; fi
	@python3 -u ./scripts/kconfiglib/menuconfig.py ./build/IoTOSconfig
	@if [ -f ./.config ]; then cp -f ./.config ./build/tuya_iot.config; fi

ifeq ($(IOT_CONFIG_PATH),)
IOT_CONFIG_PATH:=./tmp
endif
ABLITY_JSON_PATH:=./build/tuya_iot_ablity.json

config_choice: pre_config_choice
pre_config_choice:
	@if [ "$(HAS_PYTHON3)" = "" ]; then echo has no python3; exit 1; fi
	@mkdir -p $(IOT_CONFIG_PATH)
	@INPUT_CONFIG=`bash ./scripts/get_config.sh`; \
		echo copy $$INPUT_CONFIG to tuya_iot.config; \
		cp ./build/config/$$INPUT_CONFIG ./build/tuya_iot.config; \
		echo IOT_CONFIG_PATH=$(IOT_CONFIG_PATH); \
		python3 -u ./scripts/kconfiglib/conf2h.py ./build/tuya_iot.config $(IOT_CONFIG_PATH)/config.h
	@cat ./build/tuya_iot_config-h-head > $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define BUILD_DATE           \"$(BUILD_DATE)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define BUILD_TIME           \"$(BUILD_TIME)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define GIT_USER             \"$(GIT_USER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define IOT_SDK_VER          \"$(IOT_SDK_VER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define SDK_BETA_VER         \"$(SDK_BETA_VER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define TARGET_PLATFORM      \"$(TARGET_PLATFORM)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define PROJECT_NAME         \"$(PROJECT_NAME)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define SDK_ID               \"$(SDK_ID)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define KERNEL_VER           \"$(KERNEL_VERSION)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@cat $(IOT_CONFIG_PATH)/config.h >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@cat ./build/tuya_iot_config-h-tail >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@mkdir -p ./include/base/include
	@mv $(IOT_CONFIG_PATH)/tuya_iot_config.h ./include/base/include -f
	@rm $(IOT_CONFIG_PATH) -rf
	@echo "config.h is generated !!"

pre_config:
	@if [ "$(HAS_PYTHON3)" = "" ]; then echo has no python3; exit 1; fi
	@if [ ! -f ./build/tuya_iot.config ]; then \
		CONFIG=`bash ./scripts/get_config.sh`; \
		cp ./build/config/$$CONFIG ./build/tuya_iot.config; \
		fi
	@mkdir -p $(IOT_CONFIG_PATH)
	@echo INPUT_CONFIG=./build/tuya_iot.config
	@echo IOT_CONFIG_PATH=$(IOT_CONFIG_PATH)
	@python3 -u ./scripts/kconfiglib/conf2h.py ./build/tuya_iot.config $(IOT_CONFIG_PATH)/config.h
	@cat ./scripts/tuya_iot_config-h-head > $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define BUILD_DATE           \"$(BUILD_DATE)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define BUILD_TIME           \"$(BUILD_TIME)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define GIT_USER             \"$(GIT_USER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define IOT_SDK_VER          \"$(IOT_SDK_VER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define SDK_BETA_VER         \"$(SDK_BETA_VER)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define TARGET_PLATFORM      \"$(TARGET_PLATFORM)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define PROJECT_NAME         \"$(PROJECT_NAME)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define SDK_ID               \"$(SDK_ID)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@echo "#define KERNEL_VER	        \"$(KERNEL_VERSION)\"" >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@cat $(IOT_CONFIG_PATH)/config.h >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@cat ./scripts/tuya_iot_config-h-tail >> $(IOT_CONFIG_PATH)/tuya_iot_config.h
	@mkdir -p ./include/base/include
	@mv $(IOT_CONFIG_PATH)/tuya_iot_config.h ./include/base/include -f
	@echo "config.h is generated !!"
	@python3 ./scripts/h2ablity.py ./include/base/include/tuya_iot_config.h $(ABLITY_JSON_PATH)
	@echo "$(ABLITY_JSON_PATH) is generated !!"

ERROR_CODE_PATH:=./include/base/include/tuya_error_code.h
error_code_config:
	@if [ -f ./scripts/error_code/release/tuya_error_code.h ]; then \
		cp ./scripts/error_code/release/tuya_error_code.h $(ERROR_CODE_PATH); \
		echo "tuya_error_code.h is generated !"; \
		else \
		echo "not found tuya_error_code.h !"; \
		fi

config: pre_config error_code_config

.PHONY: config pre_config error_code_config menuconfig
