# Referencia funcional de interfaz

Este documento registra el comportamiento observado en las capturas de referencia para que StudyBoard mantenga un flujo de uso coherente sin depender de recursos propietarios.

## Pantalla de inicio

- encabezado simple;
- tarjeta destacada para crear una pizarra;
- cuadrícula de pizarras locales con miniatura;
- cada tarjeta muestra nombre y última fecha de edición;
- menú contextual por tarjeta;
- acceso visible a configuración.

## Lienzo

- barra superior discreta;
- gran área de trabajo sin bordes visibles;
- toolbar principal flotante en la parte inferior;
- herramientas secundarias aparecen encima de la toolbar;
- zoom flotante en la esquina inferior derecha.

## Tinta

- selector de herramientas de tinta desplegable;
- varios lápices visibles simultáneamente;
- pulsar el lápiz activo abre un panel de propiedades;
- propiedades observadas: grosor, opacidad, paleta de color y extremos de línea;
- StudyBoard añadirá además estabilización configurable.

## Selección

- selección por rectángulo transparente;
- selección de varios trazos como grupo;
- caja delimitadora con handles circulares;
- barra contextual encima del objeto/grupo;
- selección de trazos individuales dentro del grupo;
- StudyBoard añadirá también selección por lazo libre.

## Formas e imágenes

- menú flotante con formas básicas, líneas y flechas;
- menú de inserción de imágenes desde la toolbar;
- objetos seleccionados se pueden mover y redimensionar.

## Configuración del fondo

- panel lateral/modal compacto;
- selección de color;
- patrones: sólido, puntos, cuadrado, gráfico, híbrido, rombo, regla ancha, triángulo y regla estrecha.

## Principios para StudyBoard

- recrear funciones e interacciones, no nombres, logos ni recursos gráficos de Microsoft;
- priorizar uso offline y archivos locales;
- evitar componentes web embebidos;
- mantener consumo de memoria razonable incluso con imágenes, PDFs y sesiones largas.
