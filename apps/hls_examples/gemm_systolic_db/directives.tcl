//  Catapult Ultra Synthesis 10.0/263344 (Production Release) Sun Jul  3 19:13:39 PDT 2016
//  
//  Copyright (c) Mentor Graphics Corporation, 1996-2016, All Rights Reserved.
//                       UNPUBLISHED, LICENSED SOFTWARE.
//            CONFIDENTIAL AND PROPRIETARY INFORMATION WHICH IS THE
//          PROPERTY OF MENTOR GRAPHICS OR ITS LICENSORS
//  
//  Running on Linux xuany@kiwi 3.13.0-91-generic #138-Ubuntu SMP Fri Jun 24 17:00:34 UTC 2016 x86_64
//  
//  Package information: SIFLIBS v23.0_0.0, HLS_PKGS v23.0_0.0, 
//                       DesignPad v2.78_1.0
//  
solution new -state initial
solution options defaults
solution options set /Architectural/DefaultMemMapThreshold 1024
solution options set /Output/GenerateCycleNetlist false
solution options set /Flows/SCVerify/USE_NCSIM true
solution options set /Flows/SCVerify/USE_VCS true
solution file add ./hls_target.cpp -type C++
solution file add ./tb_hls_target.cpp -type C++ -exclude true
directive set -REGISTER_SHARING_MAX_WIDTH_DIFFERENCE 8
directive set -STALL_FLAG false
directive set -READY_FLAG {}
directive set -CLUSTER_ADDTREE_IN_COUNT_THRESHOLD 0
go new
directive set -DESIGN_GOAL area
directive set -OLD_SCHED false
directive set -SPECULATE true
directive set -MERGEABLE true
directive set -REGISTER_THRESHOLD 256
directive set -FSM_ENCODING none
directive set -REG_MAX_FANOUT 0
directive set -NO_X_ASSIGNMENTS true
directive set -SAFE_FSM false
directive set -REGISTER_SHARING_LIMIT 0
directive set -ASSIGN_OVERHEAD 0
directive set -TIMING_CHECKS true
directive set -MUXPATH true
directive set -REALLOC true
directive set -UNROLL no
directive set -IO_MODE super
directive set -REGISTER_IDLE_SIGNAL false
directive set -IDLE_SIGNAL {}
directive set -TRANSACTION_DONE_SIGNAL true
directive set -DONE_FLAG {}
directive set -START_FLAG {}
directive set -BLOCK_SYNC none
directive set -TRANSACTION_SYNC ready
directive set -DATA_SYNC none
directive set -RESET_CLEARS_ALL_REGS true
directive set -CLOCK_OVERHEAD 20.000000
directive set -OPT_CONST_MULTS simple_one_adder
directive set -CHARACTERIZE_ROM false
directive set -PROTOTYPE_ROM true
directive set -ROM_THRESHOLD 64
directive set -CLUSTER_OPT_CONSTANT_INPUTS true
directive set -CLUSTER_RTL_SYN false
directive set -CLUSTER_FAST_MODE false
directive set -CLUSTER_TYPE combinational
directive set -COMPGRADE fast
directive set -PIPELINE_RAMP_UP true
go analyze
directive set -DESIGN_HIERARCHY hls_target
go compile
directive set -MEM_MAP_THRESHOLD 1024
solution library add mgc_sample-065nm-dw_beh_dc -- -rtlsyntool DesignCompiler -vendor Sample -technology 065nm -Designware Yes
solution library add ram_sample-065nm-dualport_beh_dc
solution library add ram_sample-065nm-separate_beh_dc
solution library add ram_sample-065nm-singleport_beh_dc
go libraries
directive set -CLOCKS {clk {-CLOCK_PERIOD 100.0 -CLOCK_EDGE rising -CLOCK_UNCERTAINTY 0.0 -CLOCK_HIGH_TIME 50.0 -RESET_SYNC_NAME rst -RESET_ASYNC_NAME arst_n -RESET_KIND sync -RESET_SYNC_ACTIVE high -RESET_ASYNC_ACTIVE low -ENABLE_ACTIVE high}}
go assembly
directive set /hls_target/core/COL -UNROLL yes
directive set /hls_target/core/ROW -UNROLL yes
directive set /hls_target/core/main -PIPELINE_INIT_INTERVAL 1
go architect
go extract
