# StudyBoard

StudyBoard es una pizarra de escritorio local y multiplataforma inspirada en el flujo de trabajo de las aplicaciones de pizarra digital. Está escrita en C++20 + Qt 6 y apunta a Windows y Linux.

> Estado: **v0.3 en desarrollo**. Esta versión ya incluye la primera interfaz visual propia, galería local, barra flotante y ajustes básicos del lápiz. Todavía faltan funciones antes de considerarla una versión estable.

## Probar en Linux

Dependencias en Arch/CachyOS:

```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-wayland
```

Luego, desde la raíz del repositorio:

```bash
make run
```

Eso configura CMake si hace falta, compila únicamente lo que cambió y abre StudyBoard.

### Comandos Make

```bash
make              # compilar Release
make run          # compilar y ejecutar
make debug        # compilar y ejecutar Debug
make reconfigure  # recrear build/
make clean        # eliminar builds
make help         # mostrar ayuda
```

También se puede usar CMake directamente:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/StudyBoard
```

## Qué incluye v0.3

- Pantalla inicial con biblioteca local y miniaturas.
- Creación local automática de pizarras.
- Barra de herramientas flotante.
- Barra superior con título editable y estado de guardado.
- Zoom flotante.
- Lápiz, resaltador, borrador, notas, texto, imágenes y formas.
- Panel de lápiz con grosor, opacidad, color y estabilización básica.
- Selección rectangular mediante rubber band.
- Pegado de imágenes con `Ctrl+V`.
- `Ctrl+Z`, `Ctrl+Y`/`Ctrl+Shift+Z`, `Ctrl+S`, `Ctrl+O`, `Delete`.
- Guardado automático y formato `.studyboard`.
- Exportación a PNG.
- Cuadrícula opcional.

## Próximos pasos

- Selección mediante lazo libre.
- Handles reales para redimensionar/rotar objetos y selecciones.
- Perfiles independientes para varios lápices.
- Fondo con puntos, gráfico, híbrido, reglas y otros patrones.
- Mejor motor de tinta con presión de stylus.
- Importación de PDF y escritura encima.
- Undo/redo por comandos para reducir el uso de memoria en pizarras grandes.
- Copiar/cortar/duplicar objetos.
- Empaquetado final `.AppImage` y `.exe` portable.
