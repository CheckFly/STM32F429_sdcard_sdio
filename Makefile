# Portable build for the F429_sdcard_sdio_ecran project.
# Works with the STM32CubeIDE GNU toolchain and with arm-none-eabi-gcc on Linux.
#
# Usage:
#   make -j
#   make flash
#   make clean

TARGET        := F429_sdcard_sdio_ecran
BUILD_DIR     := build
PREFIX        ?= arm-none-eabi-
CC            := $(PREFIX)gcc
AS            := $(PREFIX)gcc
OBJCOPY       := $(PREFIX)objcopy
SIZE          := $(PREFIX)size
PROGRAMMER    ?= STM32_Programmer_CLI.exe
LDSCRIPT      := STM32F429ZITX_FLASH.ld

CPU           := -mcpu=cortex-m4
FPU           := -mfpu=fpv4-sp-d16
FLOAT_ABI     := -mfloat-abi=hard
MCU           := $(CPU) -mthumb $(FPU) $(FLOAT_ABI)

C_DEFS        := -DUSE_HAL_DRIVER -DSTM32F429xx -DDEBUG
C_INCLUDES    := \
	-ICore/Inc \
	-IApp/Inc \
	-IDrivers/STM32F4xx_HAL_Driver/Inc \
	-IDrivers/STM32F4xx_HAL_Driver/Inc/Legacy \
	-IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
	-IDrivers/CMSIS/Include \
	-IMiddlewares/Third_Party/FatFs/src

OPT           := -O0 -g3
CFLAGS        := $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -std=gnu11 -Wall \
	-ffunction-sections -fdata-sections -fstack-usage -MMD -MP
ASFLAGS       := $(MCU) $(OPT) -Wall -ffunction-sections -fdata-sections
LDFLAGS       := $(MCU) -T$(LDSCRIPT) --specs=nosys.specs --specs=nano.specs \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections -static \
	-Wl,--start-group -lc -lm -Wl,--end-group

VPATH         := Core/Src:App/Src:Drivers/STM32F4xx_HAL_Driver/Src:Core/Startup:Middlewares/Third_Party/FatFs/src:Middlewares/Third_Party/FatFs/src/option

C_SOURCES     := $(wildcard Core/Src/*.c) \
                 $(wildcard App/Src/*.c) \
                 $(wildcard Drivers/STM32F4xx_HAL_Driver/Src/*.c) \
                 Middlewares/Third_Party/FatFs/src/ff.c \
                 Middlewares/Third_Party/FatFs/src/ff_gen_drv.c \
                 Middlewares/Third_Party/FatFs/src/diskio.c \
                 Middlewares/Third_Party/FatFs/src/option/unicode.c
ASM_SOURCES   := Core/Startup/startup_stm32f429zitx.s

C_OBJECTS     := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
ASM_OBJECTS   := $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
OBJECTS       := $(C_OBJECTS) $(ASM_OBJECTS)
DEPS          := $(OBJECTS:.o=.d)

MKDIR_BUILD   := mkdir -p $(BUILD_DIR)
CLEAN_BUILD   := rm -rf $(BUILD_DIR)

.PHONY: all clean flash flash-bin size list

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR):
	@$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -x assembler-with-cpp $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD_DIR)/$(TARGET).elf
	$(PROGRAMMER) -c port=SWD -w $< -v -rst

flash-bin: $(BUILD_DIR)/$(TARGET).bin
	$(PROGRAMMER) -c port=SWD -w $< 0x08000000 -v -rst

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

list: $(BUILD_DIR)/$(TARGET).elf
	$(PREFIX)objdump -h -S $< > $(BUILD_DIR)/$(TARGET).list

clean:
	@$(CLEAN_BUILD)

-include $(DEPS)
