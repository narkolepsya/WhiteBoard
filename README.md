# StudyBoard

StudyBoard es una pizarra de escritorio **offline, local y multiplataforma** para Windows y Linux. Está inspirada en el flujo de trabajo de una pizarra digital para estudiar, sin depender de una cuenta o nube.

> Estado actual: prototipo temprano (0.2.0). Ya funciona como pizarra básica. La interfaz y el motor interno se están rediseñando para una experiencia de estudio más completa y eficiente.

Consulta [`ROADMAP.md`](ROADMAP.md) para el plan de desarrollo y [`docs/WHITEBOARD_REFERENCE.md`](docs/WHITEBOARD_REFERENCE.md) para la referencia funcional de interfaz.

## Funciones actuales

- lienzo muy grande con paneo y zoom
- lápiz y resaltador
- borrador por trazo/objeto
- selección y movimiento de objetos
- texto y notas adhesivas
- rectángulos, elipses y líneas
- insertar imágenes desde archivo o portapapeles
- cuadrícula opcional
- deshacer y rehacer
- guardado local en `.studyboard`
- autoguardado
- exportación PNG
- misma base de código para Windows y Linux

## Ejecutables listos para usar

El repositorio incluye GitHub Actions. Al subirlo a GitHub, entra a **Actions → Build StudyBoard** y descarga:

- `StudyBoard-Windows-x64.zip`: descomprimir y abrir `StudyBoard.exe`.
- `StudyBoard-Linux-x86_64`: contiene un `StudyBoard-Linux-x86_64.AppImage` portable.

Para publicar una versión para tus amigos, crea un tag como `v0.2.0`. El workflow adjuntará automáticamente los paquetes a una GitHub Release.

### Linux AppImage

Después de descargarla:

```bash
chmod +x StudyBoard-Linux-x86_64.AppImage
./StudyBoard-Linux-x86_64.AppImage
```

### Windows

Descomprime `StudyBoard-Windows-x64.zip` y ejecuta `StudyBoard.exe`. El paquete incluye las DLL de Qt necesarias.

## Compilar en CachyOS / Arch Linux

```bash
sudo pacman -S --needed base-devel cmake qt6-base
./scripts/build-linux.sh
./build/StudyBoard
```

También se puede hacer manualmente:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/StudyBoard
```

Si aparece:

```text
[100%] Built target StudyBoard
```

la compilación terminó correctamente. Los mensajes que empiezan con `warning:` son advertencias, no errores de compilación.

## Compilar en Windows

Se recomienda Qt 6 + CMake + Visual Studio 2022 o Qt Creator.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Para una copia portable, `windeployqt` debe ejecutarse sobre `StudyBoard.exe`. El workflow de GitHub ya hace esto automáticamente.

## Controles

- rueda del mouse: zoom
- `Espacio`: mover temporalmente el lienzo
- herramienta **Mover**: paneo permanente
- `Ctrl+S`: guardar
- `Ctrl+Z`: deshacer
- `Ctrl+Y`: rehacer
- `Delete`: eliminar objetos seleccionados

## Archivos `.studyboard`

El formato es JSON legible y guarda trazos, figuras, textos e imágenes embebidas. No depende de servidores externos.

## Subir a GitHub

Desde la carpeta del proyecto:

```bash
git init
git add .
git commit -m "Primera version de StudyBoard"
git branch -M main
git remote add origin https://github.com/TU-USUARIO/StudyBoard.git
git push -u origin main
```

Después revisa la pestaña **Actions**. Al terminar el workflow podrás descargar los ejecutables sin compilar manualmente.

## Crear una Release

```bash
git tag v0.2.0
git push origin v0.2.0
```

GitHub compilará Windows y Linux y los adjuntará a la Release correspondiente.

## Licencia

MIT. Se puede usar, modificar y compartir respetando el texto de `LICENSE`.
