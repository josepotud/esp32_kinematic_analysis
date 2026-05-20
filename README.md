# Analizador de Movimiento Cinemático

Este proyecto usa un `ESP32-C3 SuperMini` con un sensor `BMI160` para transmitir datos inerciales de 6 ejes a una interfaz web en tiempo real. El panel permite conectarse por `USB Serial` o `Bluetooth LE`, visualizar acelerómetro y giroscopio, aplicar filtros DSP y calcular la frecuencia dominante mediante FFT.

## Características

- Adquisición de 6 ejes a `200 Hz`.
- Visualización en tiempo real de acelerómetro, giroscopio, señal fusionada y espectro FFT.
- Modos de análisis `PCA`, `Magnitud 3D` y `Fusión 6D`.
- Filtros configurables: suavizado, `high-pass`, `low-pass`, rango FFT y ponderación.
- Exportación de sesiones a `CSV`.
- Compatibilidad con firmware actual en `CSV` y con firmware antiguo en `JSON`.
- Compatibilidad con BLE binario de `12 bytes` y con formato legado de texto.

## Hardware

- `ESP32-C3 SuperMini`
- `BMI160`
- Cable USB con datos

## Conexiones I2C

| BMI160 | ESP32-C3 |
|---|---|
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SCL` | `GPIO 9` |
| `SDA` | `GPIO 8` |

## Firmware

Archivo: [ESP32-C3 Script.ino](/c:/Users/josel/Documents/Python%20Scripts/esp32_kinematic_analysis/ESP32-C3%20Script.ino)

El firmware actual:

- Inicializa el `BMI160` por I2C.
- Envía por USB una línea `CSV` con timestamp y 6 valores crudos:
  `t_esp,ax,ay,az,gx,gy,gz`
- Envía por BLE un paquete binario de `16 bytes`:
  `uint32 timestamp + 6 int16`
- Emite mensajes de diagnóstico periódicos por serie con prefijo `#`.

### Requisitos en Arduino IDE

1. Instala el core `esp32` de Espressif.
2. Selecciona la placa `ESP32C3 Dev Module`.
3. Instala la librería `DFRobot_BMI160`.
4. **Imprescindible:** Activa la opción **`USB CDC On Boot` -> `Enabled`** en el menú *Herramientas* (Tools). Si no lo haces, la clase `Serial` enviará la salida a los pines de UART de hardware y el puerto USB nativo quedará mudo tras el arranque.
5. Carga `ESP32-C3 Script.ino`.

## Uso del panel web

Archivo: [index.html](/c:/Users/josel/Documents/Python%20Scripts/esp32_kinematic_analysis/index.html)

Abre `index.html` en `Chrome` o `Edge`.

### USB (Vacuna Anti-Secuestro DTR/RTS)

- Pulsa `USB` y selecciona el puerto.
- La web abre el puerto a `115200`.
- **Secuencia de Liberación:** Chrome tiende a abrir el puerto Web Serial forzando `DTR=true` y `RTS=false`, combinación que pone al ESP32-C3 en modo de flasheo perpetuo. Para solucionar esto, el panel ejecuta inmediatamente una secuencia de "vacuna":
  1. Envía `DTR=true, RTS=true` para liberar el pin `IO0`.
  2. Envía un pulso de reset físico de 50ms (`DTR=false, RTS=true`).
  3. Vuelve a `DTR=true, RTS=true` para que el chip reinicie libre y comience la transmisión normal.
- Si por el cambio de descriptores Windows decide asignar un nuevo puerto COM al iniciar el código de usuario, simplemente recarga la web y conéctate al nuevo puerto COM disponible.
- El panel acepta tanto el formato nuevo con timestamp como el formato anterior sin timestamp.

### Uso Local / Offline (Highcharts Local)

- Se incluye una copia local de `highcharts.js` en la raíz del proyecto.
- Esto evita el error de bloqueo de recursos por políticas CORS/origen y caídas de conexión al abrir el archivo directamente como un recurso local (`file://`) en el navegador, permitiendo graficar en tiempo real sin conexión a internet.

### BLE

- Pulsa `BLE` y selecciona el dispositivo `ESP32...`.
- El panel intenta estabilizar la conexión GATT con reintentos.
- Si el firmware emite binario, lo procesa como paquete de `16 bytes` con timestamp o como `12 bytes` si detecta firmware anterior.
- Si el firmware antiguo emite texto, lo interpreta automáticamente.

## Compatibilidad de navegador

- `Windows` y `Android`: pueden funcionar con `Web Serial` y/o `Web Bluetooth` según navegador.
- `iPhone` y `iPad`: los navegadores basados en WebKit no exponen `Web Serial`, y normalmente tampoco `Web Bluetooth` de forma utilizable para este caso.
- En iOS esto no es un fallo del proyecto, sino una limitación del navegador/plataforma.

## Ajustes FFT

En el panel HTML puedes modificar:

- `FFT Mín (Hz)`
- `FFT Máx (Hz)`
- `Picos FFT`

`Picos FFT` controla cuántas frecuencias dominantes se muestran. El valor por defecto se mantiene en `1`, que reproduce el comportamiento anterior.

## Grabación

1. Pulsa `Grabar`.
2. Realiza el movimiento.
3. Pulsa `Stop`.
4. Se descargará un archivo `CSV` con:
   `Timestamp, Accel_X, Accel_Y, Accel_Z, Gyro_X, Gyro_Y, Gyro_Z`

## Notas

- Si el puerto USB muestra mensajes como `ESP-ROM...`, son mensajes de arranque del chip y se ignoran.
- Las líneas de diagnóstico que empiezan por `#` también se ignoran en el panel web.
- Si el sensor no responde, el firmware sigue enviando datos planos para facilitar pruebas de conectividad.
