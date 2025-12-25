################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/cli.c \
../Src/clock.c \
../Src/controller.c \
../Src/converter.c \
../Src/fpu.c \
../Src/gpio.c \
../Src/interrupts.c \
../Src/main.c \
../Src/pwm.c \
../Src/retarget.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/systick.c \
../Src/terminal.c \
../Src/timer.c \
../Src/uart.c \
../Src/utils.c 

OBJS += \
./Src/cli.o \
./Src/clock.o \
./Src/controller.o \
./Src/converter.o \
./Src/fpu.o \
./Src/gpio.o \
./Src/interrupts.o \
./Src/main.o \
./Src/pwm.o \
./Src/retarget.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/systick.o \
./Src/terminal.o \
./Src/timer.o \
./Src/uart.o \
./Src/utils.o 

C_DEPS += \
./Src/cli.d \
./Src/clock.d \
./Src/controller.d \
./Src/converter.d \
./Src/fpu.d \
./Src/gpio.d \
./Src/interrupts.d \
./Src/main.d \
./Src/pwm.d \
./Src/retarget.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/systick.d \
./Src/terminal.d \
./Src/timer.d \
./Src/uart.d \
./Src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F4 -DSTM32F411RETx -DSTM32F411xE -c -I../Inc -I"C:/Users/hgholab/OneDrive/Documents/Embedded Systems/Modules/Final_Project/chip_headers/CMSIS/Include" -I"C:/Users/hgholab/OneDrive/Documents/Embedded Systems/Modules/Final_Project/chip_headers/CMSIS/Device/ST/STM32F4xx/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/cli.cyclo ./Src/cli.d ./Src/cli.o ./Src/cli.su ./Src/clock.cyclo ./Src/clock.d ./Src/clock.o ./Src/clock.su ./Src/controller.cyclo ./Src/controller.d ./Src/controller.o ./Src/controller.su ./Src/converter.cyclo ./Src/converter.d ./Src/converter.o ./Src/converter.su ./Src/fpu.cyclo ./Src/fpu.d ./Src/fpu.o ./Src/fpu.su ./Src/gpio.cyclo ./Src/gpio.d ./Src/gpio.o ./Src/gpio.su ./Src/interrupts.cyclo ./Src/interrupts.d ./Src/interrupts.o ./Src/interrupts.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/pwm.cyclo ./Src/pwm.d ./Src/pwm.o ./Src/pwm.su ./Src/retarget.cyclo ./Src/retarget.d ./Src/retarget.o ./Src/retarget.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/systick.cyclo ./Src/systick.d ./Src/systick.o ./Src/systick.su ./Src/terminal.cyclo ./Src/terminal.d ./Src/terminal.o ./Src/terminal.su ./Src/timer.cyclo ./Src/timer.d ./Src/timer.o ./Src/timer.su ./Src/uart.cyclo ./Src/uart.d ./Src/uart.o ./Src/uart.su ./Src/utils.cyclo ./Src/utils.d ./Src/utils.o ./Src/utils.su

.PHONY: clean-Src

