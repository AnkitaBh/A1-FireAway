################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/exe_sch.c \
../Core/Src/graph.c \
../Core/Src/lora.c \
../Core/Src/main.c \
../Core/Src/prints.c \
../Core/Src/protocols.c \
../Core/Src/schedule.c \
../Core/Src/standby.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/stop.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/uart.c 

OBJS += \
./Core/Src/exe_sch.o \
./Core/Src/graph.o \
./Core/Src/lora.o \
./Core/Src/main.o \
./Core/Src/prints.o \
./Core/Src/protocols.o \
./Core/Src/schedule.o \
./Core/Src/standby.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/stop.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/uart.o 

C_DEPS += \
./Core/Src/exe_sch.d \
./Core/Src/graph.d \
./Core/Src/lora.d \
./Core/Src/main.d \
./Core/Src/prints.d \
./Core/Src/protocols.d \
./Core/Src/schedule.d \
./Core/Src/standby.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/stop.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/uart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L412xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/exe_sch.d ./Core/Src/exe_sch.o ./Core/Src/exe_sch.su ./Core/Src/graph.d ./Core/Src/graph.o ./Core/Src/graph.su ./Core/Src/lora.d ./Core/Src/lora.o ./Core/Src/lora.su ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/prints.d ./Core/Src/prints.o ./Core/Src/prints.su ./Core/Src/protocols.d ./Core/Src/protocols.o ./Core/Src/protocols.su ./Core/Src/schedule.d ./Core/Src/schedule.o ./Core/Src/schedule.su ./Core/Src/standby.d ./Core/Src/standby.o ./Core/Src/standby.su ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/stop.d ./Core/Src/stop.o ./Core/Src/stop.su ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/uart.d ./Core/Src/uart.o ./Core/Src/uart.su

.PHONY: clean-Core-2f-Src

