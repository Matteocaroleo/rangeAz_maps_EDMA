################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_16.9.6.LTS/bin/armcl" -mv7R4 --code_state=16 --float_support=VFPv3D16 -me -Ooff --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/common_files" --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/AWR1843_MMR_mss" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS/packages" --include_path="C:/ti/ti-cgt-arm_16.9.6.LTS/include" --define=MMWAVE_SHMEM_TCMA_NUM_BANK=0 --define=MMWAVE_SHMEM_TCMB_NUM_BANK=0 --define=MMWAVE_SDK_SHMEM_ALLOC=0x0100005 --define=MMWAVE_L3RAM_NUM_BANK=8 --define=MMWAVE_SHMEM_BANK_SIZE=0x20000 --define=_LITTLE_ENDIAN --define=SOC_XWR18XX --define=SUBSYS_MSS --define=DOWNLOAD_FROM_CCS --define=DebugP_ASSERT_ENABLED -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --enum_type=int --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

cli.obj: ../cli.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_16.9.6.LTS/bin/armcl" -mv7R4 --code_state=16 --float_support=VFPv3D16 -me -Ooff --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/common_files" --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/AWR1843_MMR_mss" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS/packages" --include_path="C:/ti/ti-cgt-arm_16.9.6.LTS/include" --define=MMWAVE_SHMEM_TCMA_NUM_BANK=0 --define=MMWAVE_SHMEM_TCMB_NUM_BANK=0 --define=MMWAVE_SDK_SHMEM_ALLOC=0x0100005 --define=MMWAVE_L3RAM_NUM_BANK=8 --define=MMWAVE_SHMEM_BANK_SIZE=0x20000 --define=_LITTLE_ENDIAN --define=SOC_XWR18XX --define=SUBSYS_MSS --define=DOWNLOAD_FROM_CCS --define=DebugP_ASSERT_ENABLED -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --enum_type=int --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ti-cgt-arm_16.9.6.LTS/bin/armcl" -mv7R4 --code_state=16 --float_support=VFPv3D16 -me -Ooff --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/common_files" --include_path="C:/Users/matte/Desktop/polito_locale/rers/repo/Radar/CCS/AWR1843_MMR_mss" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS" --include_path="C:/ti/mmwave_sdk_03_06_02_00-LTS/packages" --include_path="C:/ti/ti-cgt-arm_16.9.6.LTS/include" --define=MMWAVE_SHMEM_TCMA_NUM_BANK=0 --define=MMWAVE_SHMEM_TCMB_NUM_BANK=0 --define=MMWAVE_SDK_SHMEM_ALLOC=0x0100005 --define=MMWAVE_L3RAM_NUM_BANK=8 --define=MMWAVE_SHMEM_BANK_SIZE=0x20000 --define=_LITTLE_ENDIAN --define=SOC_XWR18XX --define=SUBSYS_MSS --define=DOWNLOAD_FROM_CCS --define=DebugP_ASSERT_ENABLED -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --enum_type=int --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


