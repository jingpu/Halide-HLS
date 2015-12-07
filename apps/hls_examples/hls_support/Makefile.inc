AUTOPILOT_ROOT = /home/jingpu/vivado_hls
AUTOPILOT_ROOT ?= /cad/xilinx/vivado/2015.2/Vivado_HLS/2015.2
AUTOPILOT_MACH ?= lnx64

AUTOPILOT_TOOL := ${AUTOPILOT_ROOT}/${AUTOPILOT_MACH}/tools
AUTOPILOT_TECH := ${AUTOPILOT_ROOT}/common/technology

HLS_IFLAG += -I "${AUTOPILOT_ROOT}/include"
#HLS_IFLAG += -I "${AUTOPILOT_ROOT}/include/ap_sysc"
#HLS_IFLAG += -I "${AUTOPILOT_TOOL}/systemc/include"
#HLS_IFLAG += -I "${AUTOPILOT_TOOL}/auto_cc/include"

HLS_IFLAG += -I "../hls_support"

HLS_CXXFLAGS = $(HLS_IFLAG) -DC_TEST
HLS_CXXFLAGS += -Wno-unknown-pragmas -Wno-unused-label