$ErrorActionPreference = "Stop"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

Write-Host ""
Write-Host "Compilacion lista. Revisa build/ para StudyBoard.exe."
Write-Host "Para compartirlo, usa windeployqt sobre el .exe para copiar las DLL de Qt."
