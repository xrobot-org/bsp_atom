# ================================================
# STM32 Firmware Flash Script (PowerShell Version)
# Requires: STM32_Programmer_CLI in system PATH
# ================================================

# Serial port and baudrate
$SerialPort = "COMx"
$BaudRate = 460800

# Firmware path (no extension)
$RealFile = ".\build\Debug\atom"
$ElfFile  = "$RealFile.elf"

# ================================================
# Step 0: Check and prepare firmware file
# ================================================
if (-not (Test-Path $RealFile)) {
    Write-Host "❌ Error: Firmware file not found at $RealFile"
    exit 1
}

# Copy to a temporary file with .elf extension (required by CLI)
Copy-Item $RealFile $ElfFile -Force

# ================================================
# Step 1: Check STM32_Programmer_CLI availability
# ================================================
if (-not (Get-Command STM32_Programmer_CLI -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Error: STM32_Programmer_CLI not found in PATH"
    exit 1
}

if (-not (Test-Path $ElfFile)) {
    Write-Host "❌ Error: Prepared .elf file not found: $ElfFile"
    exit 1
}

# ================================================
# Step 2: Enter Bootloader mode via serial
# ================================================
try {
    $port = New-Object System.IO.Ports.SerialPort $SerialPort, $BaudRate, "None", 8, "One"
    $port.Open()
    Start-Sleep -Milliseconds 200

    Write-Host "🔄 Sending bootloader entry command to $SerialPort..."
    $port.WriteLine("`npower bl`n")
    $port.Close()
    Start-Sleep -Seconds 3
} catch {
    Write-Host "❌ Error: Failed to access serial port $SerialPort"
    Write-Host $_.Exception.Message
    exit 1
}

# ================================================
# Step 3: Flash the firmware
# ================================================
Write-Host "🚀 Flashing firmware file: $ElfFile"
STM32_Programmer_CLI -c port=$SerialPort br=1000000 -w "$ElfFile" -v
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Error: Flashing failed!"
    exit 1
}

# ================================================
# Step 4: Start program execution
# ================================================
Write-Host "🔄 Resetting and starting firmware..."
STM32_Programmer_CLI -c port=$SerialPort br=1000000 -g 0x08000200
if ($LASTEXITCODE -ne 0) {
    Write-Host "⚠️ Warning: Reset failed"
    exit 1
}

Write-Host "✅ Flash complete! Device started successfully."
