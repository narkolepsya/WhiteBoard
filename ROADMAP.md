# StudyBoard — Roadmap

StudyBoard busca recrear el flujo de trabajo de una pizarra digital de escritorio, de forma local, liviana y multiplataforma. El objetivo es reproducir las funciones e interacciones útiles para estudio, manteniendo nombre, iconografía y recursos propios.

## Prioridad 0 — Base y rendimiento

Antes de ampliar la interfaz se debe reforzar la arquitectura para pizarras grandes:

- reemplazar snapshots JSON completos por `QUndoStack` y comandos incrementales;
- guardado atómico con `QSaveFile` y recuperación de autosave;
- separar metadatos, contenido y recursos de imagen para no duplicar imágenes en cada historial;
- simplificación de puntos de trazos al terminar de dibujar;
- miniaturas de pizarras generadas bajo demanda y almacenadas en caché;
- formato `.studyboard` versionado y migrable.

## M1 — Inicio / galería local

- pantalla inicial al abrir la aplicación;
- tarjeta `Nueva pizarra`;
- tarjetas con miniatura, nombre y fecha de última edición;
- menú por pizarra: renombrar, duplicar, exportar y eliminar;
- búsqueda/orden opcional;
- todas las pizarras almacenadas localmente;
- configuración general desde la pantalla de inicio.

## M2 — Interfaz del lienzo

- barra superior mínima con Inicio, nombre de la pizarra y configuración;
- barra flotante inferior centrada;
- control de zoom flotante abajo a la derecha;
- paneles emergentes compactos en vez de menús tradicionales;
- modo claro y oscuro en una etapa posterior;
- interfaz adaptable a Windows y Linux.

## M3 — Motor de tinta

- varios lápices configurables e independientes;
- al pulsar de nuevo un lápiz activo se abre su configuración;
- color, grosor y opacidad por lápiz;
- extremos normal, flecha simple y flecha doble cuando aplique;
- resaltador;
- borrador por trazo y borrador parcial en una etapa posterior;
- estabilización/suavizado configurable por lápiz;
- presión de stylus/tablet cuando el dispositivo la entregue;
- suavizado final y simplificación de puntos sin cambiar perceptiblemente el trazo;
- perfiles de lápiz persistentes entre sesiones.

## M4 — Selección

- selector rectangular;
- selector de lazo libre;
- selección múltiple;
- caja delimitadora con handles de redimensionado;
- mover, duplicar y eliminar;
- agrupar/desagrupar;
- rotación cuando el tipo de objeto lo permita;
- barra contextual flotante sobre la selección;
- selección de trazos individuales y grupos de trazos.

## M5 — Objetos

- texto editable en el lienzo;
- notas adhesivas;
- imágenes desde archivo y portapapeles;
- redimensionado conservando proporción;
- formas: rectángulo, círculo, triángulo, rombo, pentágono, paralelogramo y otras;
- líneas sólidas y discontinuas;
- flechas simples y dobles;
- duplicación y orden de capas;
- PDFs como recurso de estudio para escribir encima.

## M6 — Fondo

- color de fondo;
- sólido;
- puntos;
- cuadrado;
- gráfico;
- híbrido;
- rombo;
- regla ancha;
- triángulo;
- regla estrecha;
- parámetros de tamaño/espaciado cuando corresponda.

## M7 — Atajos y ergonomía

- `Ctrl+Z`: deshacer;
- `Ctrl+Y` / `Ctrl+Shift+Z`: rehacer;
- `Ctrl+S`: guardar;
- `Ctrl+C`, `Ctrl+X`, `Ctrl+V`: objetos seleccionados;
- `Ctrl+A`: seleccionar todo;
- `Delete`: eliminar selección;
- `Esc`: cancelar herramienta/selección;
- `Space` + arrastre: mover lienzo temporalmente;
- rueda/pinch: zoom;
- atajos configurables en una etapa posterior.

## M8 — Exportación y distribución

- exportar PNG;
- exportar PDF;
- copia portable Windows (`.exe` + dependencias);
- AppImage Linux;
- GitHub Actions para generar binarios;
- Releases para versiones estables.

## Estrategia de ramas

- `main`: siempre debe compilar y ser usable;
- `feature/dashboard`: pantalla inicial y galería;
- `feature/ink-engine`: tinta y configuración de lápices;
- `feature/selection`: lazo, rectángulo y handles;
- nuevas funciones se desarrollan en ramas `feature/...` y se integran a `main` una vez probadas.
