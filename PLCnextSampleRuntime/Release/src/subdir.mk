################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CSampleRuntime.cpp \
../src/PLCnextSampleRuntime.cpp 

OBJS += \
./src/CSampleRuntime.o \
./src/PLCnextSampleRuntime.o 

CPP_DEPS += \
./src/CSampleRuntime.d \
./src/PLCnextSampleRuntime.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-pxc-linux-gnueabi-g++ -I/opt/pxc/2019.0/sysroots/cortexa9t2hf-neon-pxc-linux-gnueabi/usr/include/c++/7.3.0 -I/opt/pxc/2019.0/sysroots/cortexa9t2hf-neon-pxc-linux-gnueabi/usr/include/plcnext -I/opt/pxc/2019.0/sysroots/cortexa9t2hf-neon-pxc-linux-gnueabi/usr/include -I/opt/pxc/2019.0/sysroots/cortexa9t2hf-neon-pxc-linux-gnueabi/usr/include/c++/7.3.0/arm-pxc-linux-gnueabi -O2 -Wall -c -s -fmessage-length=0 -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a9 --sysroot=$SDKTARGETSYSROOT -Wno-unknown-pragmas -Wno-reorder -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


