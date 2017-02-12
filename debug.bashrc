#
# Source to set up your environment for debugging and test purposes in 
# a UNIX environment
#

export UDI_SRC_ROOT=`pwd`
export UDIRT_LIB_DIR=${UDI_SRC_ROOT}/libudirt/build/src
export UDI_LIB_DIR=${UDI_SRC_ROOT}/libudi-c/target/debug

if [ ! -z ${LD_LIBRARY_PATH} ]; then
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${UDI_LIB_DIR}:${UDIRT_LIB_DIR}
else
    export LD_LIBRARY_PATH=${UDI_LIB_DIR}:${UDIRT_LIB_DIR}
fi
