# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(
    Zephyr
    COMPONENTS
    REQUIRED HINTS $ENV{ZEPHYR_BASE}
)

set(CMAKE_MODULE_PATH ${APPLICATION_SOURCE_DIR}/../../cmake)

project(communication_test)

# ######################################################################################################################
# Config
# ######################################################################################################################

set(aoscore_config aoscoreconfig.hpp)
set(aoscore_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_lib_cpp")
set(cert_dir ${CMAKE_CURRENT_BINARY_DIR}/certificates)

# ######################################################################################################################
# Definitions
# ######################################################################################################################

# Aos core configuration
add_definitions(-include ${aoscore_config})
# Certificate dir
add_definitions(-DCERT_DIR="${cert_dir}")
# Root CA cart path
add_definitions(-DCONFIG_AOS_ROOT_CA_PATH="${cert_dir}/ca.pem")

# ######################################################################################################################
# Includes
# ######################################################################################################################

zephyr_include_directories(${aoscore_source_dir}/include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/../../src)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src)
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ######################################################################################################################
#  Generate API
# ######################################################################################################################

find_package(CoreAPI)

set(CORE_API_CXX_FLAGS -I${APPLICATION_SOURCE_DIR}/../../src -I${aoscore_source_dir}/include -include
                       ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/autoconf.h -include ${aoscore_config}
)

core_api_generate(${CMAKE_CURRENT_SOURCE_DIR}/../../../aos_core_api ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts)

# ######################################################################################################################
# Certificates
# ######################################################################################################################

file(MAKE_DIRECTORY ${cert_dir})

foreach(cert_type IN ITEMS "ca" "server" "client")
    execute_process(COMMAND openssl genrsa -out ${cert_dir}/${cert_type}.key 2048)

    execute_process(
        COMMAND openssl req -new -key ${cert_dir}/${cert_type}.key -out ${cert_dir}/${cert_type}.csr -subj
                "/C=XX/ST=State/L=City/O=Organization/OU=Unit/CN=${TYPE}" COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()

execute_process(
    COMMAND openssl x509 -req -days 365 -in ${cert_dir}/ca.csr -signkey ${cert_dir}/ca.key -out ${cert_dir}/ca.pem
            COMMAND_ERROR_IS_FATAL ANY
)

foreach(cert_type IN ITEMS "server" "client")
    execute_process(
        COMMAND openssl x509 -req -days 365 -in ${cert_dir}/${cert_type}.csr -CA ${cert_dir}/ca.pem -CAkey
                ${cert_dir}/ca.key -CAcreateserial -out ${cert_dir}/${cert_type}.cer COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()

# ######################################################################################################################
# mbedTLS
# ######################################################################################################################

# Enable AESNI for posix 32bit
if(${CONFIG_BOARD_NATIVE_POSIX})
    target_compile_options(mbedTLS INTERFACE -mpclmul -msse2 -maes)
endif()

# WA to have get time on zephyr
target_compile_definitions_ifndef(CONFIG_NATIVE_APPLICATION mbedTLS INTERFACE _POSIX_VERSION=200809L)

# Add Aos psa driver

set(mbedtls_source_dir "${APPLICATION_SOURCE_DIR}/../../../modules/crypto/mbedtls")
set(aoscore_source_dir "${APPLICATION_SOURCE_DIR}/../../../aos_core_lib_cpp")

set_source_files_properties(
    ${mbedtls_source_dir}/library/psa_crypto_driver_wrappers_no_static.c ${mbedtls_source_dir}/library/psa_crypto.c
    TARGET_DIRECTORY mbedTLS PROPERTIES HEADER_FILE_ONLY ON
)

target_include_directories(mbedTLSCrypto PRIVATE ${mbedtls_source_dir}/library)

target_sources(
    mbedTLSCrypto
    PRIVATE ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto_driver_wrappers_no_static.c
            ${aoscore_source_dir}/src/common/crypto/mbedtls/drivers/psa_crypto.c
            ${aoscore_source_dir}/src/common/crypto/mbedtls/driverwrapper.cpp ${mbedtls_source_dir}/library/pk_ecc.c
)

target_sources(mbedTLSX509 PRIVATE ${mbedtls_source_dir}/library/x509write.c)

# ######################################################################################################################
# Target
# ######################################################################################################################

target_sources(
    app
    PRIVATE ../../src/communication/cmclient.cpp
            ../../src/communication/communication.cpp
            ../../src/communication/iamserver.cpp
            ../../src/communication/tlschannel.cpp
            ../../src/rootca/rootca.S
            ../../src/utils/checksum.cpp
            src/cmclient.cpp
            src/iamserver.cpp
            src/tlschannel.cpp
            src/utils.cpp
)
