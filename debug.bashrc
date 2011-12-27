#
# Source to set up your environment for debugging and test purposes in 
# a UNIX environment
#

UDI_SRC_ROOT=`pwd`
export UDI_SRC_ROOT

UDI_BUILD_DIR=${UDI_SRC_ROOT}/build
export UDI_BUILD_DIR

UDI_LIB_DIR=${UDI_BUILD_DIR}/libudi
export UDI_LIB_DIR

UDI_RT_LIB_DIR=${UDI_BUILD_DIR}/libudirt
export UDI_RT_LIB_DIR

UDI_LIB_TEST_DIR=${UDI_BUILD_DIR}/tests/udi_tests
export UDI_LIB_TEST_DIR

UDI_RT_LIB_TEST_DIR=${UDI_BUILD_DIR}/tests/udirt_tests
export UDI_RT_LIB_TEST_DIR

if [ ! -z ${LD_LIBRARY_PATH} ]; then
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${UDI_LIB_DIR}:${UDI_RT_LIB_DIR}
else
    export LD_LIBRARY_PATH=${UDI_LIB_DIR}:${UDI_RT_LIB_DIR}
fi
