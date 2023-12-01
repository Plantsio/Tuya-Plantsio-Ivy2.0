
set(TUYA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TUYA_OS_SDK_INCLUDE_DIR ${TUYA_ROOT_DIR}/include)
set(TUYA_OS_LIBS_DIR ${TUYA_ROOT_DIR}/libs)
set(TUYA_OS_ADAPTER_DIR ${TUYA_ROOT_DIR}/tuyaos_adapter)


FILE(GLOB_RECURSE adapter_sources ${TUYA_OS_ADAPTER_DIR}/*.c)

set(include_dirs


	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/bluetooth/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/dac/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/flash/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/gpio/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/hci/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/i2c/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/i2s/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/init/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/network/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/pinmux/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/pm/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/pwm/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/register/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/rtc/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/security/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/spi/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/system/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/timer/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/uart/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/utilities/include"
	# "${TUYA_OS_SDK_INCLUDE_DIR}/adapter/wakeup/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/watchdog/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/adapter/wifi/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/base/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_system/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_api_wifi/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_wifi/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_devos/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_cJSON/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_system_service/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_system_service/include/utilities"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_api_iot/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_tuya_cloud/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_tuya_cloud/include/iot_dns"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_tuya_cloud/include/http"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_tuya_cloud/include/log_seq"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_tuya_cloud/include/tls"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_network/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_wifi/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/base_event/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/svc_mf_test/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_http/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_websocket/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_transport/include"
#	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_lwip/include"
#	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_lwip/include/lwip"
#	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_lwip/include/lwip/arch"
#	"${TUYA_OS_SDK_INCLUDE_DIR}/components/tal_lwip/include/lwip/prot"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_tls/include"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_tls/include/mbedtls"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/lib_tls/include/psa"
	"${TUYA_OS_SDK_INCLUDE_DIR}/components/base_db/include"

)



#For ESP_IDF_4.4.1
idf_component_register(SRCS "${adapter_sources}"
                    INCLUDE_DIRS "${include_dirs}"
					PRIV_REQUIRES nvs_flash bt esp_adc_cal efuse
					REQUIRES app_update bootloader_support hal esp_wifi esp_event driver esp_netif esp_ringbuf
					)

add_prebuilt_library(tuyaos "${TUYA_OS_LIBS_DIR}/libtuyaos.a" REQUIRES tuya)
target_link_libraries(${COMPONENT_LIB} INTERFACE tuyaos)

				
