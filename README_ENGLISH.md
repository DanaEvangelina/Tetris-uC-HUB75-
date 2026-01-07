------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        CAPSTONE PROJECT
        COURSE: LABORATORY III – DIGITAL ELECTRONICS
        BALSEIRO INSTITUTE
        TELECOMMUNICATIONS ENGINEERING
        GONZALEZ, DANA EVANGELINA
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

# 🧱 Tetris on STM32F446 + HUB75 LED Matrix

**Tetris** game implemented on an **STM32F446RE (Nucleo Board)** and displayed on a **64×64 RGB HUB75 LED matrix**.  
The system is fully programmed in **C**, using **CMSIS**, **DMA**, and **timers**, following a **hybrid model combining direct register access and HAL**, enabling low-level hardware control and flicker-free refresh.

Base project                 -> https://github.com/bikefrivolously/led_matrix/tree/master  
Base project ported to IDE    -> https://github.com/LorenBataraza/labo_3/tree/main/Leds_Final  
Base Tetris project          -> https://github.com/svedev0/tetris-c/tree/master  

---

## 🧩 Main features

- Full HUB75 panel control using **DMA** and **timers**  
- Modular-ish and reusable library design  
- Programmable visual effects (plasma, animations, text, etc.)  
- **ADC analog input** from a **potentiometer** for real-time parameter control  
- **External interrupt (EXTI)** for the on-board push button, with software debouncing using **TIM6**

---

## ⚙️ Technical architecture

| Component | Description |
|----------|-------------|
| **MCU** | STM32F446RE (180 MHz, ARM Cortex-M4) |
| **Display** | HUB75, 64×64 pixels, RGB, 1/32 scan |
| **Language** | C (CMSIS, hybrid HAL) |
| **Build system** | Makefile + gcc-arm-none-eabi |
| **Refresh rate** | Up to ~549 Hz (adjustable via `PRESCALE`) |

---

## 🕹️ Game operation

The microcontroller handles the complete **Tetris logic** (state machine):

- Piece generation and rotation (I, O, T, L, Z)  
- Block-based collision detection  
- Movement controlled via potentiometer  
- Score tracking  
- Game states: Press Start, Playing, Game Over  
- Line clearing  
- Progressive difficulty (increases with each cleared row)

---

## 🔧 HUB75 display control

The display refresh is fully automatic, managed by **DMA** and two synchronized timers:

### 🧠 TIM8 — Clock signal (CLK)  ------------------------------> MASTER
- **Mode:** PWM (CH1 → PC6, AF3)  
- **Frequency:** ~9 MHz  
- **Action:** On each falling edge, generates a **DMA request** (DMA2_Stream2_Channel7)  
- **DMA:**  
  - **Circular double-buffer** mode  
  - **Memory → GPIOC (PC0–PC5)** transfers  
  - Updates RGB data lines in real time  

### ⚙️ TIM5 — Latch and Enable --------------------------------------> SLAVE
- **CH1 (PA0):** generates the **LAT** pulse at the end of each row  
- **CH2 (PA1):** controls **OE** via PWM to adjust brightness  
- **Interrupt:** on each update:  
  1. Adjusts **OE** duty cycle  
  2. Advances to the next row (A–E control on PB0–PB4)  

With this scheme, the matrix refresh remains constant and CPU-free, allowing the microcontroller to handle the Tetris logic in parallel.

---

## ⚙️ Pin mapping

### 🟦 HUB75

| Signal | STM32 Pin | Description |
|:------|:----------|:------------|
| R1 | PB0 | Red channel, upper half |
| G1 | PB1 | Green channel, upper half |
| B1 | PB5 | Blue channel, upper half |
| R2 | PB6 | Red channel, lower half |
| G2 | PB7 | Green channel, lower half |
| B2 | PB8 | Blue channel, lower half |
| A | PB9 | Row select A |
| B | PB10 | Row select B |
| C | PB11 | Row select C |
| D | PB12 | Row select D |
| CLK | PB13 | Data clock |
| LAT | PB14 | Latch (data storage) |
| OE | PB15 | Output Enable |

### 🎚️ Potentiometer

| Signal | STM32 Pin | Description |
|:-------|:----------|:------------|
| Analog input | **PA4** | Potentiometer value read via ADC |
| Push button | **PA13** | Digital input with external interrupt (EXTI) |

---

## 🧱 Project structure

```
Core/
├── Inc/
│   ├── colors.h              # Color definitions
│   ├── inicializacion.h      # Peripheral configuration functions
│   ├── led.h                 # Data structures and functions for LED panel handling
│   ├── led_programs.h        # Game and visual effect definitions
│   ├── main.h                # Global project definitions
│   ├── stm32f4xx_hal_msp.h   # MSP initialization prototypes (HAL)
│   ├── stm32f4xx_it.h        # Interrupt handler prototypes
│   └── systick.h             # SysTick handling
│
├── Src/
│   ├── inicializacion.c      # Peripheral configuration (TIM, DMA, ADC, GPIO, EXTI)
│   ├── interrupts.c          # User interrupt routines.
│   │                           Separates logic from stm32f4xx_it.c
│   ├── led.c                 # Low-level HUB75 LED matrix driver.
│   │                           Handles frame and static_frame buffers
│   ├── led_programs.c        # Tetris logic, visual effects,
│   │                           Start and Game Over screens
│   ├── main.c                # System initialization and main loop
│   │                           Implements the state machine
│   ├── stm32f4xx_it.c        # CubeMX-generated interrupt handlers
│   ├── stm32f4xx_hal_msp.c   # MSP initialization: clocks, GPIO, DMA, EXTI
│   ├── systick.c             # SysTick implementation
│   ├── syscalls.c            # System call support (newlib)
│   ├── sysmem.c              # Dynamic memory management
│   └── system_stm32f4xx.c    # System and clock configuration
```


------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
(Written with AI assistance)

