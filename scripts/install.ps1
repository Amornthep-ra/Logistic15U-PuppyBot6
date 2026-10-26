param(
    [string]$ArduinoCli = 'arduino-cli',
    [string]$ConfigFile,
    [string]$PackageIndexUrl = 'https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$baseIndex = 'https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json'
$configArgs = @()
if ($ConfigFile) { $configArgs = @('--config-file', $ConfigFile) }
function Invoke-Arduino {
    & $ArduinoCli @configArgs @args
    if ($LASTEXITCODE -ne 0) { throw "Arduino CLI failed with code $LASTEXITCODE" }
}
$urls = "$baseIndex,$PackageIndexUrl"
Invoke-Arduino core update-index --additional-urls $urls
Invoke-Arduino core install 'rp2040:rp2040@6.2.0' --additional-urls $urls
Invoke-Arduino core install 'logistic15u:rp2040@0.2.0' --additional-urls $urls
Invoke-Arduino lib update-index
$dependencies = @(
    'Adafruit GFX Library@1.12.6',
    'Adafruit ST7735 and ST7789 Library@1.11.0',
    'Adafruit TCS34725@1.4.4',
    'Adafruit MCP3008@1.3.3',
    'Adafruit BusIO@1.17.4',
    'rp2040-encoder-library@0.2.0',
    'SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library@1.3.2'
)
# Complete compile dependency set; prevent implicit version upgrades.
Invoke-Arduino lib install --no-deps @dependencies
$previousUnsafeInstall = $env:ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL
try {
    $env:ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL = 'true'
    Invoke-Arduino lib install --zip-path (Join-Path $projectRoot 'dist\Logistic15U-PuppyBot6-0.2.0.zip')
} finally {
    $env:ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL = $previousUnsafeInstall
}
Invoke-Arduino compile --clean --fqbn 'logistic15u:rp2040:logistic15u' (Join-Path $projectRoot 'main')
Write-Host 'Installed and compiled. Add both index URLs to Arduino IDE Preferences for future board updates.'
