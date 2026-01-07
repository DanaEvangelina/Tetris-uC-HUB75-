# Tetris-uC-HUB75-
Tetris using a STM32F446 + HUB75 LED Matrix.

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	
		PROYECTO INTEGRADOR
		MATERIA: LABORATORIO III - ELECTRONICA DIGITAL
		INSTITUTO BALSEIRO
		INGENIERIA EN TELECOMUNICACIONES
		GONZALEZ, DANA EVANGELINA
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

# 🧱 Tetris on STM32F446 + HUB75 LED Matrix

Juego **Tetris** implementado en una **STM32F446RE (Nucleo Board)** y animado en una **matriz LED RGB 64×64 HUB75**.  
El sistema está programado íntegramente en **C** utilizando **CMSIS**, **DMA** y **timers**, modelo hibrido con registros y HAL, para un control directo del hardware y un refresco sin parpadeo.


Proyecto base 				-> 	https://github.com/bikefrivolously/led_matrix/tree/master
Proyecto base porteado al IDE		-> 	https://github.com/LorenBataraza/labo_3/tree/main/Leds_Final 
Proyecto base Tetris 			-> 	https://github.com/svedev0/tetris-c/tree/master

---

## 🧩 Características principales

- Control completo del panel HUB75 mediante **DMA** y **temporizadores**.  
- Librería modular-ish y reutilizable.
- Efectos visuales programables (plasma, animaciones, texto, etc.).  
- Lectura analógica ADC de un **potenciómetro** para modificar parámetros en tiempo real.  
- **Interrupción externa (EXTI)** para el pulsador integrado con antirrebote (TIM6) por software.
---

## ⚙️ Arquitectura técnica

| Componente | Descripción |
|-------------|-------------|
| **MCU** | STM32F446RE (180 MHz, ARM Cortex-M4) |
| **Display** | HUB75, 64×64 píxeles, RGB, 1/32 scan |
| **Lenguaje** | C (CMSIS, HAL hibrido) |
| **Build System** | Makefile + gcc-arm-none-eabi |
| **Frecuencia de actualización** | Hasta ~549 Hz (ajustable con `PRESCALE`) |

---

## 🕹️ Funcionamiento del juego

El microcontrolador maneja toda la lógica del **Tetris** (Maquina de Estados):

- Generación y rotación de piezas (I, O, T, L, Z)  
- Detección de colisiones por bloques  
- Movimiento controlado por potenciometro.
- Puntaje.
- Estados del juego: Press Start, Playing, Game Over.
- Eliminacion de filas
- Dificultad progresiva (aumenta por cada fila eliminada)

---

## 🔧 Control del display HUB75

El refresco del display es totalmente automático, gestionado por **DMA** y dos timers sincronizados:

### 🧠 TIM8 — Señal de reloj (CLK)  ------------------------------> MASTER
- **Modo:** PWM (CH1 → PC6, AF3)  
- **Frecuencia:** ~9 MHz  
- **Acción:** En cada flanco descendente genera una solicitud de **DMA** (DMA2_Stream2_Channel7).  
- **DMA:**  
  - Modo **doble buffer circular**  
  - Transferencias **memoria → GPIOC (PC0–PC5)**  
  - Actualiza las líneas RGB de la matriz en tiempo real  

### ⚙️ TIM5 — Latch y Enable --------------------------------------> SLAVE
- **CH1 (PA0):** genera el pulso **LAT** al finalizar cada fila  
- **CH2 (PA1):** controla **OE** mediante PWM para regular brillo  
- **Interrupción:** en cada actualización:  
  1. Ajusta duty cycle de **OE**  
  2. Avanza a la siguiente fila (control A–E en PB0–PB4)  

Con este esquema, el refresco de la matriz se mantiene constante y libre de carga de CPU, permitiendo que el micro controle la lógica del Tetris en paralelo.

---

## ⚙️ Mapeo de pines

### 🟦 HUB75
| Señal | Pin STM32 | Descripción |
|:------|:-----------|:------------|
| R1 | PB0 | Canal rojo, fila superior |
| G1 | PB1 | Canal verde, fila superior |
| B1 | PB5 | Canal azul, fila superior |
| R2 | PB6 | Canal rojo, fila inferior |
| G2 | PB7 | Canal verde, fila inferior |
| B2 | PB8 | Canal azul, fila inferior |
| A | PB9 | Selección de fila A |
| B | PB10 | Selección de fila B |
| C | PB11 | Selección de fila C |
| D | PB12 | Selección de fila D |
| CLK | PB13 | Reloj de datos |
| LAT | PB14 | Latch (almacena datos) |
| OE | PB15 | Output Enable (habilita salida) |


### 🎚️ Potenciómetro
| Señal | Pin STM32 | Descripción |
|:---------|:-----------|:-------------|
| Entrada analógica | **PA4** | Lectura del valor del potenciómetro mediante ADC |
| Pulsador | **PA13** | Entrada digital con interrupción externa (EXTI) |

---

## 🧱 Estructura del proyecto

Core/
├── Inc/
│   ├── colors.h              # Definición de los colores
│   ├── inicializacion.h      # Prototipos de funciones de configuración de periféricos
│   ├── led.h                 # Estructuras de datos y funciones para el manejo del panel LED
│   ├── led_programs.h        # Definiciones de programas, juegos y efectos sobre el panel LED
│   ├── main.h                # Definiciones globales del proyecto
│   ├── stm32f4xx_hal_msp.h   # Prototipos de inicialización MSP (HAL)
│   ├── stm32f4xx_it.h        # Prototipos de manejadores de interrupciones
│   └── systick.h             # Manejo del SysTick y temporización
│
├── Src/
│   ├── inicializacion.c      # Configuración de periféricos (TIM, DMA, ADC, GPIO, EXTI)
│   ├── interrupts.c          # Rutinas de interrupción de usuario (TIM, DMA, ADC, EXTI).
│   │                           Separa la lógica de interrupciones del archivo generado
│   │                           `stm32f4xx_it.c`
│   ├── led.c                 # Funciones de bajo nivel para escribir en la matriz LED HUB75.
│   │                           Maneja los buffers de imagen (`frame`, `static_frame`)
│   │                           y la comunicación con los pines de datos
│   ├── led_programs.c        # Lógica del Tetris (tetrominos, colisiones, estados del juego),
│   │                           efectos visuales y pantallas de Start y Game Over
│   ├── main.c                # Inicialización de hardware y periféricos.
│   │                           Configuración del panel y ejecución del bucle principal
│   │                           (máquina de estados)
│   ├── stm32f4xx_it.c        # Manejo de interrupciones generado por CubeMX.
│   │                           Algunas rutinas redirigen a `interrupts.c`
│   ├── stm32f4xx_hal_msp.c   # Inicialización MSP de HAL: clocks, GPIO alternativos,
│   │                           DMA y prioridades de interrupción
│   ├── systick.c             # Implementación del manejo de SysTick
│   ├── syscalls.c            # Soporte de llamadas al sistema (newlib)
│   ├── sysmem.c              # Gestión de memoria dinámica
│   └── system_stm32f4xx.c    # Configuración del sistema y del clock


---


----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
(Redactado por IA)
