Write-Host "Building tinygltf tester on Windows..." -ForegroundColor Green

# Create build directory if it doesn't exist
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}
Set-Location build

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
cmake .. -G "Visual Studio 16 2019" -A x64

# Build the project
Write-Host "Building the project..." -ForegroundColor Yellow
cmake --build . --config Release

Write-Host "Build completed!" -ForegroundColor Green
Write-Host "The tester executables should be in build/tests/ directory" -ForegroundColor Cyan

Read-Host "Press Enter to continue" 