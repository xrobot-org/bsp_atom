#!/bin/bash

# ===============================================
# STM32 Firmware Flash Script (Bash Version)
# Requires: STM32_Programmer_CLI in user directory
# ===============================================

# Serial device and baud rate
SERIAL_PORT="/dev/ttyCH343USB0"
BAUD_RATE=460800

# Path to STM32CubeProgrammer CLI
STM32_CLI="$HOME/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI"

# Path to firmware (can be ELF/BIN/HEX etc.)
ELF_FILE="./build/xrobot.elf"

# ===============================================
# Step 1: Check if STM32 CLI is executable
# ===============================================
if [ ! -x "$STM32_CLI" ]; then
    echo "❌ Error: STM32_Programmer_CLI is not executable! Check the path:"
    echo "   $STM32_CLI"
    exit 1
fi

# ===============================================
# Step 2: Check if serial device exists
# ===============================================
if [ ! -e "$SERIAL_PORT" ]; then
    echo "❌ Error: Serial port not found: $SERIAL_PORT"
    exit 1
fi

# ===============================================
# Step 3: Configure serial port
# ===============================================
stty -F "$SERIAL_PORT" $BAUD_RATE raw -echo

# ===============================================
# Step 4: Enter bootloader mode
# ===============================================
echo "🔄 Sending bootloader entry command to $SERIAL_PORT..."
sleep 1
echo -e "\n" >"$SERIAL_PORT"
sleep 1
echo -e "power bl\n" >"$SERIAL_PORT"
sleep 3  # Wait for device to enter bootloader

# ===============================================
# Step 5: Flash the firmware
# ===============================================
echo "🚀 Flashing firmware: $ELF_FILE"
"$STM32_CLI" -c port="$SERIAL_PORT" br=1000000 -w "$ELF_FILE" -v

if [ $? -ne 0 ]; then
    echo "❌ Error: Firmware flashing failed."
    exit 1
else
    echo "✅ Firmware successfully flashed!"
fi

# ===============================================
# Step 6: Reset and start the STM32
# ===============================================
echo "🔄 Resetting and starting STM32..."
"$STM32_CLI" -c port="$SERIAL_PORT" br=1000000 -g 0x08000200
echo "✅ Done."
