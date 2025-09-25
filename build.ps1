# build.ps1
g++ -std=c++20 -static-libgcc -static-libstdc++ $(Get-ChildItem src\*.cpp | ForEach-Object { $_.FullName }) -o trading_app.exe
Write-Host "Build complete."

# Run the executable
./trading_app.exe
