################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ti-cgt-arm_16.9.6.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./can_message.obj" \
"./cli.obj" \
"./mss_main.obj" \
"./sleep.obj" \
"./sys_intvecs.obj" \
"./sys_startup.obj" \
"./osal_nonos/DebugP_nonos.obj" \
"./osal_nonos/Event_nonos.obj" \
"./osal_nonos/HwiP_nonos.obj" \
"./osal_nonos/MemoryP_nonos.obj" \
"./osal_nonos/SemaphoreP_nonos.obj" \
"./osal_nonos/sys_vim.obj" \
"../mss_mmw_linker.cmd" \
$(GEN_CMDS__FLAG) \
-llibesm_xwr18xx.aer4f \
-llibgpio_xwr18xx.aer4f \
-llibsoc_xwr18xx.aer4f \
-llibmailbox_xwr18xx.aer4f \
-llibpinmux_xwr18xx.aer4f \
-llibcrc_xwr18xx.aer4f \
-llibDMA_xwr18xx.aer4f \
-llibuart_xwr18xx.aer4f \
-llibmmwavelink_xwr18xx.aer4f \
-llibcli_xwr18xx.aer4f \
-lrtsv7R4_T_le_v3D16_eabi.lib \
-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include osal_nonos/subdir_vars.mk
-include subdir_rules.mk
-include osal_nonos/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
AWR1843_MMR_mss_version_1.xer4f \

EXE_OUTPUTS__QUOTED += \
"AWR1843_MMR_mss_version_1.xer4f" \

BIN_OUTPUTS += \
AWR1843_MMR_mss_version_1.hex \

BIN_OUTPUTS__QUOTED += \
"AWR1843_MMR_mss_version_1.hex" \


# All Target
all:
	@$(MAKE) --no-print-directory pre-build
	@$(MAKE) --no-print-directory main-build

# Main-build Target
main-build: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "AWR1843_MMR_mss_version_1.xer4f"

# Tool invocations
AWR1843_MMR_mss_version_1.xer4f: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: ARM Linker'
	"C:/ti/ti-cgt-arm_16.9.6.LTS/bin/armcl" -mv7R4 --code_state=16 --float_support=VFPv3D16 -me -Ooff --define=MMWAVE_SHMEM_TCMA_NUM_BANK=0 --define=MMWAVE_SHMEM_TCMB_NUM_BANK=0 --define=MMWAVE_SDK_SHMEM_ALLOC=0x0100005 --define=MMWAVE_L3RAM_NUM_BANK=8 --define=MMWAVE_SHMEM_BANK_SIZE=0x20000 --define=_LITTLE_ENDIAN --define=SOC_XWR18XX --define=SUBSYS_MSS --define=DOWNLOAD_FROM_CCS --define=DebugP_ASSERT_ENABLED -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --enum_type=int --abi=eabi -z -m"AWR1843_MMR_mss_version_1.map" --heap_size=0x800 --stack_size=0x800 -i"C:/ti/ti-cgt-arm_16.9.6.LTS/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/control/mmwavelink/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/crc/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/esm/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/gpio/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/soc/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/pinmux/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/mailbox/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/dma/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/drivers/uart/lib" -i"C:/ti/mmwave_sdk_03_05_00_04/packages/ti/utils/cli/lib" -i"C:/Users/lorenzo/git/Radar/CCS/AWR1843_MMR_mss/osal_nonos" -i"C:/Users/lorenzo/git/Radar/CCS/AWR1843_MMR_mss/" -i"C:/ti/ti-cgt-arm_16.9.6.LTS/include" --reread_libs --disable_auto_rts --diag_warning=225 --diag_wrap=off --display_error_number --warn_sections --xml_link_info="AWR1843_MRR_version1.xml" --rom_model --unused_section_elimination=on -o "AWR1843_MMR_mss_version_1.xer4f" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

AWR1843_MMR_mss_version_1.hex: $(EXE_OUTPUTS)
	@echo 'Building secondary target: "$@"'
	@echo 'Invoking: ARM Hex Utility'
	"C:/ti/ti-cgt-arm_16.9.6.LTS/bin/armhex" --diag_wrap=off -o "AWR1843_MMR_mss_version_1.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building secondary target: "$@"'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "can_message.obj" "cli.obj" "mss_main.obj" "sleep.obj" "sys_intvecs.obj" "sys_startup.obj" "osal_nonos\DebugP_nonos.obj" "osal_nonos\Event_nonos.obj" "osal_nonos\HwiP_nonos.obj" "osal_nonos\MemoryP_nonos.obj" "osal_nonos\SemaphoreP_nonos.obj" "osal_nonos\sys_vim.obj" 
	-$(RM) "can_message.d" "cli.d" "mss_main.d" "sleep.d" "osal_nonos\DebugP_nonos.d" "osal_nonos\Event_nonos.d" "osal_nonos\HwiP_nonos.d" "osal_nonos\MemoryP_nonos.d" "osal_nonos\SemaphoreP_nonos.d" "osal_nonos\sys_vim.d" 
	-$(RM) "sys_intvecs.d" "sys_startup.d" 
	-@echo 'Finished clean'
	-@echo ' '

pre-build:
	-if exist "C:/Users/lorenzo/git/Radar/CCS/AWR1843_MMR_mss/AWR1843_MMR_dss_version_1.bin" del /f "C:/Users/lorenzo/git/Radar/CCS/AWR1843_MMR_mss/AWR1843_MMR_dss_version_1.bin"
	-@echo ' '

post-build:
	-C:/ti/mmwave_sdk_03_05_00_04/packages/scripts/ImageCreator/out2rprc/out2rprc.exe AWR1843_MMR_mss_version_1.xer4f AWR1843_MMR_mss_version_1.bin
	-C:/ti/mmwave_sdk_03_05_00_04/packages/scripts/ImageCreator/multicore_image_generator/MulticoreImageGen.exe LE 37 0x01000005 AWR1843_MMR_version_1.bin 0x35510000 AWR1843_MMR_mss_version_1.bin 0xb5510000 C:/ti/mmwave_sdk_03_05_00_04/firmware/radarss/xwr18xx_radarss_rprc.bin 0xd5510000 D:\VITAL_SIGNS\vitalSigns_rawData_v0\CCS/AWR1843_MMR_dss/Debug/AWR1843_MMR_dss_version_1.bin
	-C:/ti/mmwave_sdk_03_05_00_04/packages/scripts/ImageCreator/crc_multicore_image/crc_multicore_image.exe AWR1843_MMR_version_1.bin  AWR1843_MMR_version_1.bin.tmp
	-C:/ti/mmwave_sdk_03_05_00_04/packages/scripts/ImageCreator/append_bin_crc/gen_bincrc32.exe AWR1843_MMR_version_1.bin
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

