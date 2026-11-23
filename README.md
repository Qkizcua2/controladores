# Prácticas STM32 Nucleo-F411RE — Proyecto Semáforo (DSBM, Lab 2 – Lab 6)

Evolución incremental del sistema embebido **Semáforo** desarrollado en **C (STM32 HAL / CMSIS)** con **STM32CubeMX** (`Semaforo.ioc`) y **PlatformIO** para la placa **STM32 Nucleo-F411RE** (`nucleo_f411re`), dentro de la asignatura *Diseño de Sistemas Basados en Microprocesador* (ESI Ciudad Real – UCLM, curso 2025/2026).

**Autores:** Sergio Alcaide Nieto y Marta Roiloa García.

## Progresión de los laboratorios

| Laboratorio | Funcionalidad y técnicas implementadas |
| :--- | :--- |
| **Lab 2 — Máquina de Estados Finitos** | Máquina de estados (`ESTADO_VERDE`, `ESTADO_ESPERA_4S`, `ESTADO_AMARILLO`, `ESTADO_ROJO_PEATON_VERDE`, `ESTADO_PARPADEO_LENTO`, `ESTADO_PARPADEO_RAPIDO`) con control de LEDs de coches y peatones y lectura de pulsador. |
| **Lab 3 — Interrupciones y Emergencia** | Interrupciones externas (`EXTI10` para botón y `EXTI8` para interruptor de emergencia), modo emergencia con parpadeo en amarillo, tiempo mínimo de seguridad de 5 s, gestión de petición pendiente, `smart_delay()` y envío de estado por `USART2`. |
| **Lab 4 — Detección por Ultrasonidos** | Detección de peatones sin pulsador mediante sensor de ultrasonidos (`<= 20 cm`), medición temporal con `TIM2`, conmutación dinámica entrada/salida del pin, telemetría por `USART2` y verificación con osciloscopio. |
| **Lab 5 — Señalización Acústica (PWM)** | Señales sonoras de accesibilidad mediante PWM (`TIM1`, canal 3) y `Control_Buzzer()` integrado en `smart_delay()`: 500 Hz (0,5 s ON/OFF) en paso peatonal, 2000 Hz (0,2 s ON/OFF) en parpadeo lento y 3000 Hz continuo en parpadeo rápido, validado con osciloscopio. |
| **Lab 6 — Información Visual (LCD RGB I2C)** | Pantalla LCD con retroiluminación RGB por `I2C1` (`LCD_ADDR = 0x7c`, `RGB_ADDR = 0xc4`) y librería `lcd_drv`, mostrando estados (`"Pulse Boton"`, `"Espere Verde"`, `"Pase"`, `"EMERGENCIA"`) y colores asociados optimizando las escrituras al bus en cambios de estado. |

## Mapa de pines y periféricos (STM32F411RE)

* **LEDs coches:** Rojo `PC7` (D9), Amarillo `PB6` (D10), Verde `PA7` (D11)
* **LEDs peatones:** Rojo `PB5` (D4), Verde `PB4` (D5)
* **Entradas con interrupción:** Pulsador `PB10` (D6, `EXTI10`), Interruptor de emergencia `PA8` (D7, `EXTI8`)
* **Ultrasonidos:** Disparo/Eco con `TIM2` (`PA9` / `PA8` o `PA6` según etapa/sensor, con conmutación de dirección en sensor de un solo pin) y depuración serie por `USART2`
* **Zumbador:** Salida PWM en `TIM1` canal 3 (`PA10` / D2)
* **Pantalla LCD RGB:** Bus `I2C1` (`PB9` SDA, `PB8` SCL)

## Estructura del proyecto (`Semaforo/`)

```text
Semaforo/
├── include/          # Cabeceras del proyecto y configuración HAL/IT
├── lib/lcd_drv/      # Driver LCD RGB I2C (Lab 6)
├── src/Src/          # main.c, stm32f4xx_it.c, stm32f4xx_hal_msp.c, etc.
├── test/             # Pruebas
├── platformio.ini    # Entorno nucleo_f411re
└── Semaforo.ioc      # Configuración de pines y periféricos en STM32CubeMX
