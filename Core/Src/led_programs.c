/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : led_programs.c
  * @brief          : Definicion de las funciones core
  *  Created on: 11/2025
  *      Author: Dana Gonzalez
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "led.h"
#include "led_programs.h"


#define BLOCK_SIZE 3  // tamaño de un cuadrado (en píxeles)
// dimensiones del tablero
#define BOARD_WIDTH  (WIDTH / BLOCK_SIZE)
#define BOARD_HEIGHT (HEIGHT / BLOCK_SIZE)

// Tablero lógico: cada celda guarda color (black = vacío)
extern RGB_t static_frame[]; // ya existe en tu proyecto como pixel buffer
static RGB_t board[BOARD_HEIGHT][BOARD_WIDTH]; // inicializar a black

// Helper: color vacío
static const RGB_t COLOR_EMPTY = {0,0,0};

extern RGB_t static_frame[WIDTH * HEIGHT];
extern char current_tetromino;
extern uint8_t rotacion;
extern float x_offset, y_offset;
extern RGB_t color_actual;
extern uint32_t score;


//---------------------------------------------------------------------------------------------------------------------------------------------------------------
//																		TETRIS
//---------------------------------------------------------------------------------------------------------------------------------------------------------------


// Funciones porteadas del proyecto Tetris Master ========================================================================

//Limpiar el tablero
void board_clear(void) {
    for (int by = 0; by < BOARD_HEIGHT; ++by)
        for (int bx = 0; bx < BOARD_WIDTH; ++bx)
            board[by][bx] = COLOR_EMPTY;
}


// Dibuja una celda bloque (bx,by en bloques) dentro de frame (buffer de pixels)
static inline void draw_block_pixel(RGB_t *frame, RGB_t color, int bx, int by) {
    int px = bx * BLOCK_SIZE;
    int py = by * BLOCK_SIZE;
    for (int yy = py; yy < py + BLOCK_SIZE; ++yy) {
        for (int xx = px; xx < px + BLOCK_SIZE; ++xx) {
            frame[yy * WIDTH + xx] = color;
        }
    }
}

// Renderiza al static frame
void render_board_to_frame(RGB_t *frame) {
    // asume frame ya fue llenado con black o lo sobreescribe
    for (int by = 0; by < BOARD_HEIGHT; ++by) {
        for (int bx = 0; bx < BOARD_WIDTH; ++bx) {
            RGB_t c = board[by][bx];
            if (!(c.R==0 && c.G==0 && c.B==0)) {
                draw_block_pixel(frame, c, bx, by);
            }
        }
    }
}


// Nueva implementacion de logica de colision: x,y en bloques (posición del tetromino). Retorna true si colisiona o fuera de bordes.
bool check_collision_block(char tetromino, uint8_t rotation, int x, int y) {
    uint32_t t_width  = get_tetromino_width(tetromino, rotation);
    uint32_t t_height = get_tetromino_height(tetromino, rotation);

    for (int by = 0; by < (int)t_height; ++by) {
        for (int bx = 0; bx < (int)t_width; ++bx) {
            if (!tetromino_block_active(tetromino, rotation, bx, by)) continue;

            int board_x = x + bx;
            int board_y = y + by;

            // Fuera de límites
            if (board_x < 0 || board_x >= BOARD_WIDTH) return true;
            if (board_y >= BOARD_HEIGHT) return true;  // tocar fondo

            // Si dentro del tablero, chequear ocupación
            if (board_y >= 0) { // si negativo todavía está fuera arriba (no colisiona con board)
                RGB_t c = board[board_y][board_x];
                if (!(c.R==0 && c.G==0 && c.B==0)) return true;
            }
        }
    }
    return false;
}

// Colision horizontal
bool check_collision_horizontal_block(char tetromino, uint8_t rotation, int desired_x, int y) {
    // Simplemente reutilizamos check_collision_block probando la posición (desired_x, y)
    return check_collision_block(tetromino, rotation, desired_x, y);
}


// Tetromino activo -> inactivo => Pasa a ser parte del static frame
void merge_tetromino_to_board(char tetromino, RGB_t color, int x, int y, uint8_t rotation) {
    uint32_t t_width  = get_tetromino_width(tetromino, rotation);
    uint32_t t_height = get_tetromino_height(tetromino, rotation);

    for (int by = 0; by < (int)t_height; ++by) {
        for (int bx = 0; bx < (int)t_width; ++bx) {
            if (!tetromino_block_active(tetromino, rotation, bx, by))
                continue;

            int board_x = x + bx;
            int board_y = y + by;

            if (board_x >= 0 && board_x < BOARD_WIDTH &&
                board_y >= 0 && board_y < BOARD_HEIGHT) {
                board[board_y][board_x] = color;
            }
        }
    }
}



// Nuevo game over
// Detecta si el tetromino llegó al tope superior después de mergearse
bool check_game_over_board_y0(char tetromino, uint8_t rotation, int x_block, int y_block) {
    uint8_t t_height = get_tetromino_height(tetromino, rotation);

    // Game over si alguna parte del tetromino quedó en la fila superior (y = 0)
    // o si directamente está fuera del tablero hacia arriba.
    if (y_block <= 0) {
        for (uint8_t by = 0; by < t_height; by++) {
            for (uint8_t bx = 0; bx < get_tetromino_width(tetromino, rotation); bx++) {
                if (!tetromino_block_active(tetromino, rotation, bx, by))
                    continue;

                int board_y = y_block + by;
                if (board_y <= 0) {
                    return true;
                }
            }
        }
    }

    return false;
}




//Borrar linea (condicion de avance)

void board_clear_full_lines_and_score(void) {
    for (int by = BOARD_HEIGHT - 1; by >= 0; --by) {
        bool full = true;
        for (int bx = 0; bx < BOARD_WIDTH; ++bx) {
            RGB_t c = board[by][bx];
            if (c.R==0 && c.G==0 && c.B==0) { full = false; break; }
        }
        if (full) {
            // Desplazar todo hacia arriba una fila
            for (int y2 = by; y2 > 0; --y2) {
                for (int bx = 0; bx < BOARD_WIDTH; ++bx)
                    board[y2][bx] = board[y2-1][bx];
            }
            // top row cleared
            for (int bx = 0; bx < BOARD_WIDTH; ++bx) board[0][bx] = COLOR_EMPTY;
            score += 100; // ejemplo de puntos
            ++by; // volver a chequear misma fila pos actual porque se trajo otra fila
        }
    }
}


//Nueva funcion de dibujo de tetromino
void draw_tetromino_on_frame(RGB_t *frame, RGB_t color, int bx, int by, uint8_t rotation, char type) {
    int pixel_x = bx * BLOCK_SIZE;
    int pixel_y = by * BLOCK_SIZE;
    LED_tetromino_rotated(frame, color, pixel_x, pixel_y, rotation, type);
}


// =============================|| Tetrominos ||=================================

// Tetromino I (línea vertical)
void LED_tetromino_I(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y) {
    for (uint8_t i = 0; i < 4; i++) {
        LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                      x, x + BLOCK_SIZE - 1);
    }
}

// Tetromino O (cuadrado)
void LED_tetromino_O(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y) {
    LED_rectangle(frame, color, y, y + BLOCK_SIZE*2 - 1,
                  x, x + BLOCK_SIZE*2 - 1);
}

void LED_tetromino_T(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y) {
    // barra inferior
    for (uint8_t i = 0; i < 3; i++) {
        LED_rectangle(frame, color,
                      y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                      x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
    }
    // bloque central arriba
    LED_rectangle(frame, color,
                  y, y + BLOCK_SIZE - 1,
                  x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
}


// Tetromino L
void LED_tetromino_L(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y) {
    for (uint8_t i = 0; i < 3; i++) {
        LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                      x, x + BLOCK_SIZE - 1);
    }
    LED_rectangle(frame, color, y + 2*BLOCK_SIZE, y + 3*BLOCK_SIZE - 1,
                  x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
}

// Tetromino S
void LED_tetromino_S(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y) {
    // fila superior
    for (uint8_t i = 1; i < 3; i++) {
        LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                      x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
    }
    // fila inferior
    for (uint8_t i = 0; i < 2; i++) {
        LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                      x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
    }
}


// =============================|| Tetrominos rotados ||=================================

// Tetromino I - Rotaciones
void LED_tetromino_I_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation) {
    switch(rotation % 4) {
        case 0: // 0° - horizontal
            for (uint8_t i = 0; i < 4; i++) {
                LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            break;
        case 1: // 90° - vertical
            for (uint8_t i = 0; i < 4; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x, x + BLOCK_SIZE - 1);
            }
            break;
        case 2: // 180° - horizontal (invertida)
            for (uint8_t i = 0; i < 4; i++) {
                LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            break;
        case 3: // 270° - vertical (invertida)
            for (uint8_t i = 0; i < 4; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            }
            break;
    }
}

// Tetromino O - No necesita rotación
void LED_tetromino_O_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation) {
    LED_tetromino_O(frame, color, x, y); // El cuadrado es igual en todas las rotaciones
}

// Tetromino T - Rotaciones
void LED_tetromino_T_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation) {
    switch(rotation % 4) {
        case 0: // T normal (arriba)
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                              x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                          x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            break;

        case 1: // T hacia la derecha
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                              x, x + BLOCK_SIZE - 1);
            LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                          x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            break;

        case 2: // T abajo
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                              x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                          x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            break;

        case 3: // T hacia la izquierda
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                              x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                          x, x + BLOCK_SIZE - 1);
            break;
    }
}


// Tetromino L - Rotaciones (corregido)
void LED_tetromino_L_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation) {
    switch(rotation % 4) {

        case 0: // L normal (de pie)
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color,
                              y + i * BLOCK_SIZE, y + (i + 1) * BLOCK_SIZE - 1,
                              x, x + BLOCK_SIZE - 1);
            LED_rectangle(frame, color,
                          y + 2 * BLOCK_SIZE, y + 3 * BLOCK_SIZE - 1,
                          x + BLOCK_SIZE, x + 2 * BLOCK_SIZE - 1);
            break;

        case 1: // L hacia la derecha
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color,
                              y, y + BLOCK_SIZE - 1,
                              x + i * BLOCK_SIZE, x + (i + 1) * BLOCK_SIZE - 1);
            LED_rectangle(frame, color,
                          y + BLOCK_SIZE, y + 2 * BLOCK_SIZE - 1,
                          x + 2 * BLOCK_SIZE, x + 3 * BLOCK_SIZE - 1);
            break;

        case 2: // L invertida (de cabeza)
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color,
                              y + i * BLOCK_SIZE, y + (i + 1) * BLOCK_SIZE - 1,
                              x + BLOCK_SIZE, x + 2 * BLOCK_SIZE - 1);
            LED_rectangle(frame, color,
                          y, y + BLOCK_SIZE - 1,
                          x, x + BLOCK_SIZE - 1);
            break;

        case 3: // ✅ L hacia la izquierda (corregida)
            // Antes dibujaba la barra en la fila superior y el bloque extra a la izquierda.
            // Ahora se invierte verticalmente para coincidir con la definición lógica.
            for (uint8_t i = 0; i < 3; i++)
                LED_rectangle(frame, color,
                              y + BLOCK_SIZE, y + 2 * BLOCK_SIZE - 1,
                              x + i * BLOCK_SIZE, x + (i + 1) * BLOCK_SIZE - 1);
            LED_rectangle(frame, color,
                          y, y + BLOCK_SIZE - 1,
                          x + 2 * BLOCK_SIZE, x + 3 * BLOCK_SIZE - 1);
            break;
    }
}




// Tetromino S - Rotaciones
void LED_tetromino_S_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation) {
    switch(rotation % 4) {
        case 0: // S normal
            // fila superior
            for (uint8_t i = 1; i < 3; i++) {
                LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            // fila inferior
            for (uint8_t i = 0; i < 2; i++) {
                LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            break;
        case 1: // 90° - S vertical
            // columna izquierda
            for (uint8_t i = 0; i < 2; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x, x + BLOCK_SIZE - 1);
            }
            // columna derecha
            for (uint8_t i = 1; i < 3; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            }
            break;
        case 2: // 180° - S invertida horizontal
            // fila superior
            for (uint8_t i = 0; i < 2; i++) {
                LED_rectangle(frame, color, y, y + BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            // fila inferior
            for (uint8_t i = 1; i < 3; i++) {
                LED_rectangle(frame, color, y + BLOCK_SIZE, y + 2*BLOCK_SIZE - 1,
                            x + i*BLOCK_SIZE, x + (i+1)*BLOCK_SIZE - 1);
            }
            break;
        case 3: // 270° - S invertida vertical
            // columna izquierda
            for (uint8_t i = 1; i < 3; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x, x + BLOCK_SIZE - 1);
            }
            // columna derecha
            for (uint8_t i = 0; i < 2; i++) {
                LED_rectangle(frame, color, y + i*BLOCK_SIZE, y + (i+1)*BLOCK_SIZE - 1,
                            x + BLOCK_SIZE, x + 2*BLOCK_SIZE - 1);
            }
            break;
    }
}

// Función general para rotar cualquier tetromino
void LED_tetromino_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation, char tetromino_type) {
    switch(tetromino_type) {
        case 'I':
            LED_tetromino_I_rotated(frame, color, x, y, rotation);
            break;
        case 'O':
            LED_tetromino_O_rotated(frame, color, x, y, rotation);
            break;
        case 'T':
            LED_tetromino_T_rotated(frame, color, x, y, rotation);
            break;
        case 'L':
            LED_tetromino_L_rotated(frame, color, x, y, rotation);
            break;
        case 'S':
            LED_tetromino_S_rotated(frame, color, x, y, rotation);
            break;

    }
}


//======================================|| Tetromino desplazamiento ||=======================================

// Función auxiliar para obtener el ancho de cada tetromino según su rotación
uint8_t get_tetromino_width(char tetromino_type, uint8_t rotation) {
    switch(tetromino_type) {
        case 'I':
            return (rotation % 2 == 0) ? 4 : 1; // Horizontal: 4 bloques, Vertical: 1 bloque

        case 'O':
            return 2; // Siempre 2x2

        case 'T':
            switch(rotation % 4) {
                case 0: return 3;// T normal
                case 2: // T invertida
                    return 3; // ancho = 3 bloques
                case 1: return 2;// T 90°
                case 3: // T 270°
                    return 2; // ancho = 2 bloques
            }

        case 'L':
            switch(rotation % 4) {
                case 0: // L normal
                case 2: // L invertida
                    return 2; // ancho más angosto
                case 1: // L rotada horizontal
                case 3: // L rotada horizontal invertida
                    return 3; // ancho completo
            }

        case 'S':
            return (rotation % 2 == 0) ? 3 : 2; // Horizontal: 3x2, Vertical: 2x3

        default:
            return 2;
    }
}


// Función auxiliar para obtener el alto de cada tetromino según su rotación
uint8_t get_tetromino_height(char tetromino_type, uint8_t rotation) {
    switch(tetromino_type) {
        case 'I':
            return (rotation % 2 == 0) ? 1 : 4; // Horizontal: 1 bloque, Vertical: 4 bloques
        case 'O':
            return 2; // Siempre 2x2
        case 'T':
            switch(rotation % 4) {
                case 0: return 2; // T normal
                case 2: return 2; // T invertida
                case 1: return 3;// T 90°
                case 3: return 3; // Altura = 3 bloques
            }
        case 'L':
            switch(rotation % 4) {
                case 0: // L normal, barra vertical + bloque a la derecha abajo
                case 2: // L invertida vertical
                    return 3; // altura completa
                case 1: // L rotada horizontal
                case 3: // L rotada horizontal invertida
                return 2; // altura más baja
            }
        case 'S':
            return (rotation % 2 == 0) ? 2 : 3; // Algunas rotaciones son 3x2, otras 2x3
        default:
            return 2;
    }
}


// -----------------------------------------------------------
// Desplaza un tetromino horizontalmente según un valor normalizado percentage (0.0 - 1.0)
// 0.0 = extremo izquierdo, 1.0 = extremo derecho
// -----------------------------------------------------------
void LED_tetromino_bar_x(RGB_t *frame, RGB_t color,
                         char tetromino_type, uint32_t rotation,
                         uint32_t y, float percentage)
{

    uint32_t tetromino_width_pixels = get_tetromino_width(tetromino_type, rotation) * BLOCK_SIZE;
    uint32_t max_x = WIDTH - tetromino_width_pixels;
    uint32_t new_x = (uint32_t)(percentage * max_x);

    LED_tetromino_rotated(frame, color, new_x, y, rotation, tetromino_type);
}




//================================================|| Colisiones y mecanicas ||=====================================================

// Reinicia tetromino si llega al fondo
void check_bottom() {
    uint32_t height_pixels = get_tetromino_height(current_tetromino, rotacion) * BLOCK_SIZE;
    if (y_offset > HEIGHT - height_pixels) {
        y_offset = 0.0f;
        // Elegir nuevo tetromino aleatorio
        current_tetromino = "ISZLOJT"[rand() % 7];
        rotacion = 0;
    }
}

// Copia el frame (para dejar estaticos a los tetrominos que ya colisionaron)
void LED_copyFrame(RGB_t src[HEIGHT][WIDTH], RGB_t dst[HEIGHT][WIDTH]) {
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) {
            dst[y][x] = src[y][x];
        }
    }
}



//  Copia el tetromino actual al static_frame
void merge_tetromino(RGB_t *static_frame, char tetromino, RGB_t color, int x, int y) {
    switch (tetromino) {
        case 'I':
            LED_tetromino_I(static_frame, color, x, y);
            break;
        case 'O':
            LED_tetromino_O(static_frame, color, x, y);
            break;
        case 'T':
            LED_tetromino_T(static_frame, color, x, y);
            break;
        case 'L':
            LED_tetromino_L(static_frame, color, x, y);
            break;
        case 'S':
            LED_tetromino_S(static_frame, color, x, y);
            break;

    }
}


// Devuelve true si el bloque (bx, by) del tetromino está activo según la rotación
bool tetromino_block_active(char type, uint8_t rotation, uint8_t bx, uint8_t by) {
    // bx, by están en rango 0-3 (bloques de 4x4)
    switch(type) {
        case 'I':
            if(rotation % 2 == 0) return by == 0 && bx < 4; // horizontal
            else return bx == 0 && by < 4;               // vertical
        case 'O':
            return bx < 2 && by < 2;
        case 'T':
              	switch(rotation % 4) {
                case 0: // 0° → barra abajo, bloque arriba
                    return (by == 1 && bx < 3) || (by == 0 && bx == 1);
                case 1: // 90° → barra a la izquierda, bloque a la derecha
                    return (bx == 0 && by < 3) || (bx == 1 && by == 1);
                case 2: // 180° → barra arriba, bloque abajo
                    return (by == 0 && bx < 3) || (by == 1 && bx == 1);
                case 3: // 270° → barra a la derecha, bloque a la izquierda
                    return (bx == 1 && by < 3) || (bx == 0 && by == 1);

            }
              	case 'L':
              	    switch(rotation % 4) {
                    case 0: // L normal (vertical)
                        return (bx == 0 && by < 3) || (bx == 1 && by == 2);
                    case 1: // L hacia la derecha
                    	return (by == 0 && bx < 3) || (bx == 2 && by == 1);
                    case 2: // L invertida
                        return (bx == 1 && by < 3) || (bx == 0 && by == 0);
                    case 3: // L hacia la izquierda
                        return (by == 1 && bx < 3) || (bx == 2 && by == 0);
              	    }


        case 'S':
            switch(rotation % 4) {
            case 0: // horizontal normal
                return (by == 0 && (bx == 1 || bx == 2)) ||
                       (by == 1 && (bx == 0 || bx == 1));
            case 1: // vertical normal
                return (bx == 0 && (by == 0 || by == 1)) ||
                       (bx == 1 && (by == 1 || by == 2));
            case 2: // horizontal invertida
                return (by == 0 && (bx == 0 || bx == 1)) ||
                       (by == 1 && (bx == 1 || bx == 2));
            case 3: // vertical invertida (corregida)
                return (bx == 1 && (by == 0 || by == 1)) ||
                       (bx == 0 && (by == 1 || by == 2));
            }
            return false;
    }
}


// Detectar colision con la pantalla y bloques fijos -> A MEJORAR
bool check_collision(char tetromino, uint8_t rotation, int x, int y) {
    uint32_t t_width  = get_tetromino_width(tetromino, rotation);
    uint32_t t_height = get_tetromino_height(tetromino, rotation);

    for (uint32_t by = 0; by < t_height; by++) {
        for (uint32_t bx = 0; bx < t_width; bx++) {
            if (!tetromino_block_active(tetromino, rotation, bx, by)) continue;

            int px = x + bx * BLOCK_SIZE;
            int py = y + by * BLOCK_SIZE;

            // Limites de pantalla
            if (px < 0 || px + BLOCK_SIZE > WIDTH) return true;
            if (py + BLOCK_SIZE > HEIGHT) return true;

            // Chequear si colisiona con bloque fijo
            for (int iy = py; iy < py + BLOCK_SIZE; iy++) {
                for (int ix = px; ix < px + BLOCK_SIZE; ix++) {
                    RGB_t pixel = static_frame[iy * WIDTH + ix];
                    if (pixel.R != 0 || pixel.G != 0 || pixel.B != 0) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


// Devuelve true si el tetromino puede moverse horizontalmente a desired_x
bool check_collision_horizontal(char tetromino, uint8_t rotation, int desired_x, int y) {
    uint32_t t_width  = get_tetromino_width(tetromino, rotation);
    uint32_t t_height = get_tetromino_height(tetromino, rotation);

    for (uint32_t by = 0; by < t_height; by++) {
        for (uint32_t bx = 0; bx < t_width; bx++) {
            if (!tetromino_block_active(tetromino, rotation, bx, by)) continue;

            int px = desired_x + bx * BLOCK_SIZE;
            int py = y + by * BLOCK_SIZE;

            // Limites pantalla
            if (px < 0 || px + BLOCK_SIZE > WIDTH) return true;

            // Solo chequear los bordes horizontales del bloque
            // Reviso el lateral izquierdo y derecho dentro de la misma fila del bloque
            for (int iy = py; iy < py + BLOCK_SIZE; iy++) {

                // se mueve a la derecha, chequear el borde derecho
                int ix_right = px + BLOCK_SIZE - 1;
                RGB_t pix_r = static_frame[iy * WIDTH + ix_right];
                if (pix_r.R != 0 || pix_r.G != 0 || pix_r.B != 0)
                    return true;

                // se mueve a la izquierda, chequear el borde izquierdo
                int ix_left = px;
                RGB_t pix_l = static_frame[iy * WIDTH + ix_left];
                if (pix_l.R != 0 || pix_l.G != 0 || pix_l.B != 0)
                    return true;
            }
        }
    }
    return false;
}



// Detecta si la colision se da en el limite superior
bool check_game_over(RGB_t *static_frame, char tetromino, uint8_t rotation, int x, int y) {
    for (uint8_t by = 0; by < 4; by++) {
        for (uint8_t bx = 0; bx < 4; bx++) {
            if (!tetromino_block_active(tetromino, rotation, bx, by))
                continue;

            int block_x = x + bx * BLOCK_SIZE;
            int block_y = y + by * BLOCK_SIZE;

            // Game over si colisiona con algún bloque ya fijo
            if (block_y >= 0 && block_y < HEIGHT && block_x >= 0 && block_x < WIDTH) {
                RGB_t pixel = static_frame[block_y * WIDTH + block_x];
                if (pixel.R != 0 || pixel.G != 0 || pixel.B != 0)
                    return true;
            }

            // Game over si bloque queda por encima del marco
            if (block_y < 0)
                return true;
        }
    }
    return false;
}





//---------------------------------------------------------------------------------------------------------------------------------------------------------------
//																	EFECTOS VISUALES GENERALES
//---------------------------------------------------------------------------------------------------------------------------------------------------------------






//=============================================================================> Animacion Ondas
void LED_waveEffect(RGB_t *frame) {
    static float time;
    float xx;
    uint8_t r, g, b;

    if(time > 2*M_PI) {
        time = 0.0;
    }

    for(uint8_t y = 0; y < HEIGHT; y++) {
        for(uint8_t x = 0; x < WIDTH; x++) {
            xx = mapf(x, 0, WIDTH-1, 0, 2*M_PI);
            r = 16 + 100 * (bound(sinf(xx + time + 2*M_PI/3), 0.5, -0.5) + 0.5);
            g = 16 + 100 * (bound(sinf(xx + time - 2*M_PI/3), 0.5, -0.5) + 0.5);
            b = 16 + 100 * (bound(sinf(xx + time         ), 0.5, -0.5) + 0.5);
            PIXEL(frame, x, y).R = r;
            PIXEL(frame, x, y).G = g;
            PIXEL(frame, x, y).B = b;
        }
    }
    time += 0.1;
}

//=============================================================================> Animacion Plasma
void LED_plasmaEffect(RGB_t *frame) {
    static float time;
    uint8_t r, g, b;
    float xx, yy;
    float v;
    float delta = 0.025;

    time += 0.025;
    if(time > 12*M_PI) {
        delta *= -1;
    }

    for(uint8_t y = 0; y < HEIGHT; y++) {
        yy = mapf(y, 0, HEIGHT-1, 0, 2*M_PI);
        for(uint8_t x = 0; x < WIDTH; x++) {
            xx = mapf(x, 0, WIDTH-1, 0, 2*M_PI);

            v = sinf(xx + time);
            v += sinf((yy + time) / 2.0);
            v += sinf((xx + yy + time) / 2.0);
            float cx = xx + .5 * sinf(time/5.0);
            float cy = yy + .5 * cosf(time/3.0);
            v += sinf(sqrtf((cx*cx+cy*cy)+1)+time);
            v /= 2.0;
            r = 255 * (sinf(v * M_PI) + 1) / 2;
            g = 100 * (cosf(v * M_PI) + 1) / 2;
            b = 100 * (sinf(v * M_PI + 2*M_PI/3) + 1) / 2;
            PIXEL(frame, x, y).R = r;
            PIXEL(frame, x, y).G = g;
            PIXEL(frame, x, y).B = b;
        }
    }
}


//=============================================================================> Imagenes
void LED_Image(RGB_t *frame, RGB_t *image_to_copy){
    for (uint8_t row = 0; row <= WIDTH; row++) {
        for (uint8_t col = 0; col <= HEIGHT; col++) {
            // Calculamos la posición en el frame
            uint32_t index = row * WIDTH + col;
            // Establecemos el color en el frame
            frame[index] = image_to_copy[index];
        };
    };
}

//=============================================================================> Rectangulo (base de los tetrominos)
void LED_rectangle(RGB_t *frame, RGB_t color, uint8_t min_row, uint8_t max_row, uint8_t min_col, uint8_t max_col){
 // Limitar los valores a los rangos del marco
    if (min_col >= WIDTH) min_col = WIDTH - 1;
    if (max_col >= WIDTH) max_col = WIDTH - 1;
    if (min_row >= HEIGHT) min_row = HEIGHT - 1;
    if (max_row >= HEIGHT) max_row = HEIGHT - 1;

    for (uint8_t row = min_row; row <= max_row; row++) {
        for (uint8_t col = min_col; col <= max_col; col++) {
            // Calculamos la posición en el frame
            uint32_t index = row * WIDTH + col;
            // Establecemos el color en el frame
            frame[index].R = color.R;
            frame[index].G = color.G;
            frame[index].B = color.B;
        };
    };
}

//=============================================================================> Barra analogica (Base para el movimiento de los tetrominos)
void LED_analog_bar(RGB_t *frame, RGB_t color_1, RGB_t color_2, float percentage) {
    // Asegurarnos de que 'percentage' está en el rango 0.0 - 1.0
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 1.0f) percentage = 1.0f;

    // Calcular la línea de separación
    uint32_t separation_line = (uint32_t)((1.0f - percentage) * HEIGHT);

    // Llenar con color_1 desde separation_line hasta la parte superior
    if (separation_line < HEIGHT) {
        LED_rectangle(frame, color_1, separation_line, HEIGHT - 1, 0, WIDTH - 1);
    }

    // Llenar con color_2 desde la parte inferior hasta separation_line
    if (separation_line > 0) {
        LED_rectangle(frame, color_2, 0, separation_line - 1, 0, WIDTH - 1);
    }
}


//=============================================================================> Letras simples
void LED_Letter(RGB_t *frame, uint8_t *letter, uint8_t x_i, uint8_t y_i, RGB_t color, uint8_t circular) {
    // Iteramos por cada píxel de la letra
    for (uint8_t x = 0; x < TEXT_WIDTH; x++) {
        for (uint8_t y = 0; y < TEXT_HEIGHT; y++) {
            int16_t frame_x = (circular)? (x+x_i)%WIDTH: x + x_i; // Posición x en el marco
            int16_t frame_y = y + y_i; // Posición y en el marco
            uint32_t letter_index = x + y * TEXT_WIDTH;

            // Si el píxel está dentro del marco, lo dibujamos
            if (frame_x >= 0 && frame_x < WIDTH && frame_y >= 0 && frame_y < HEIGHT) {
                uint32_t index = frame_x + frame_y * WIDTH;

                // Dibujamos solo si el píxel de la letra está activo
                if (letter[letter_index]) {
                    frame[index] = color; 
                } else {
                    frame[index] = (RGB_t){0x00, 0x00, 0x00};
                }
            }
        }
    }
}


//=============================================================================> Texto simple
void LED_Text(
    RGB_t *frame, 
    uint8_t **font, 
    const char *str, 
    uint8_t x_i, 
    uint8_t y_i, 
    RGB_t color, 
    uint8_t enable_line_wrap
) {
    uint8_t cursor_x = x_i;
    uint8_t cursor_y = y_i;

    // Iteramos sobre cada caracter en el string
    for (const char *c = str; *c != '\0'; c++) {
        // Ignoramos caracteres fuera del rango 'a' a 'z'
        if (*c < 'a' || *c > 'z') {
            continue;
        }

        // Accedemos a la matriz correspondiente al carácter desde el font
        uint8_t *letter = font[*c - 'a'];

        // Dibujamos la letra en la posición actual
        LED_Letter(frame, letter, cursor_x, cursor_y, color,0);

        // Movemos el cursor para la próxima letra
        cursor_x += TEXT_WIDTH + 1; // Espacio entre letras

        // Si la letra excede el ancho del frame
        if (cursor_x + TEXT_WIDTH > WIDTH) {
            if (enable_line_wrap) {
                // Salto de línea si está habilitado
                cursor_x = x_i;
                cursor_y += TEXT_HEIGHT + 1; // Espacio entre líneas
            } else {
                // Detener impresión si no se permite el salto de línea
                break;
            }
        }

        // Salimos si excedemos los límites verticales
        if (cursor_y + TEXT_HEIGHT > HEIGHT) {
            break;
        }
    }
}


//=============================================================================> Texto animado deslizante horizontal
void LED_SliddingText(
    RGB_t *frame, 
    uint8_t **font, 
    const char *str, 
    uint8_t x_i, 
    uint8_t y_i, 
    RGB_t color, 
    uint8_t enable_line_wrap,
    uint8_t direction
) {
    static uint32_t contador_frames =0;
    uint32_t frames_por_desplazamiento = 6;
    uint8_t desplazamiento = (uint8_t)(contador_frames/frames_por_desplazamiento)*direction;
    
    uint8_t cursor_x = x_i;
    uint8_t cursor_y = y_i;
    uint8_t cursor_actual;

    // Iteramos sobre cada caracter en el string
    for (const char *c = str; *c != '\0'; c++) {
        // Ignoramos caracteres fuera del rango 'a' a 'z'
        if (*c < 'a' || *c > 'z') {
            continue;
        }

        // Accedemos a la matriz correspondiente al carácter desde el font
        uint8_t *letter = font[*c - 'a'];
        
        // Si el cursor sobresale hay que traerlo devuelta
        cursor_actual = (cursor_x+desplazamiento)%WIDTH;
        // Dibujamos la letra en la posición actual
        LED_Letter(frame, letter, cursor_actual, cursor_y, color,1);

        // Movemos el cursor para la próxima letra
        cursor_x += TEXT_WIDTH + 1; // Espacio entre letras

        // Si la letra excede el ancho del frame
        if (cursor_x + TEXT_WIDTH > WIDTH) {
            if (enable_line_wrap) {
                // Salto de línea si está habilitado
                cursor_x = x_i;
                cursor_y += TEXT_HEIGHT + 1; // Espacio entre líneas
            } else {
                // Detener impresión si no se permite el salto de línea
                break;
            }
        }

        // Salimos si excedemos los límites verticales
        if (cursor_y + TEXT_HEIGHT > HEIGHT) {
            break;
        }
    }
    contador_frames++;
}


//=============================================================================> Texto animado deslizante vertical
void LED_SliddingTextVertical(
    RGB_t *frame,
    uint8_t **font,
    const char *str,
    uint8_t x_i,
    uint8_t y_i,
    RGB_t color,
    uint8_t enable_column_wrap,
    uint8_t direction
) {
    static uint32_t contador_frames = 0;
    uint32_t frames_por_desplazamiento = 6;
    uint8_t desplazamiento = (uint8_t)(contador_frames / frames_por_desplazamiento) * direction;

    uint8_t cursor_x = x_i;
    uint8_t cursor_y = y_i;
    uint8_t cursor_actual;

    // Iteramos sobre cada caracter en el string
    for (const char *c = str; *c != '\0'; c++) {
        // Ignoramos caracteres fuera del rango 'a' a 'z'
        if (*c < 'a' || *c > 'z') {
            continue;
        }

        // Accedemos a la matriz correspondiente al carácter desde el font
        uint8_t *letter = font[*c - 'a'];

        // Calculamos la posición actual en Y, aplicando desplazamiento vertical
        cursor_actual = (cursor_y + desplazamiento) % HEIGHT;

        // Dibujamos la letra en la posición actual
        LED_Letter(frame, letter, cursor_x, cursor_actual, color, 1);

        // Movemos el cursor verticalmente para la próxima letra
        cursor_y += TEXT_HEIGHT + 1; // Espacio entre letras

        // Si la letra excede la altura del frame
        if (cursor_y + TEXT_HEIGHT > HEIGHT) {
            if (enable_column_wrap) {
                // Si está habilitado el salto de columna, reiniciamos en Y
                cursor_y = y_i;
                cursor_x += TEXT_WIDTH + 1;
            } else {
                // Si no se permite, detenemos la impresión
                break;
            }
        }

        // Salimos si excedemos los límites horizontales
        if (cursor_x + TEXT_WIDTH > WIDTH) {
            break;
        }
    }

    contador_frames++;
}


//=============================================================================> Animacion de arena
void LED_FallingSand(RGB_t *frame) {
    RGB_t *frame_auxiliar = (RGB_t *)malloc(WIDTH * HEIGHT * sizeof(RGB_t));

    if (frame_auxiliar == NULL) {
        return;
    }

    memcpy(frame_auxiliar, frame, WIDTH * HEIGHT * sizeof(RGB_t));

    uint16_t estado_actual[WIDTH * HEIGHT];
    int16_t siguiente_estado[WIDTH * HEIGHT];
    uint32_t index;

    memset(estado_actual, 0, sizeof(estado_actual));
    memset(siguiente_estado, -1, sizeof(siguiente_estado));

    for (uint8_t row = 0; row < HEIGHT; row++) {
        for (uint8_t col = 0; col < WIDTH; col++) {
            index = row * WIDTH + col;
            estado_actual[index] = (frame[index].R || frame[index].G || frame[index].B) ? 1 : 0;
        }
    }
    // Inicializar las matrices de estado
    for (uint8_t row = 0; row < HEIGHT; row++) {
        for (uint8_t col = 0; col < WIDTH; col++) {
            index = row * WIDTH + col;
            if (frame[index].R == 0 && frame[index].G == 0 && frame[index].B == 0) {
                estado_actual[index] = 0;  // No hay grano en esta posición
            } else {
                estado_actual[index] = 1;  // Hay un grano en esta posición
            }
            siguiente_estado[index] = -1;  // Inicializamos con -1, indicando que el grano no se ha movido
        }
    }

    // Realiza la simulación
// Inicializar la última fila en `siguiente_estado`
for (int col = 0; col < WIDTH; col++) {
    int index = (HEIGHT - 1) * WIDTH + col;
    if (estado_actual[index] == 1) {
        siguiente_estado[index] = index; // Mantener el grano en la última fila
    }
}

// Fase 2: Calcular el siguiente estado
for (int row = HEIGHT - 2; row >= 0; row--) {  // Recorrer de abajo hacia arriba
    for (int col = 0; col < WIDTH; col++) {
        int current_index = row * WIDTH + col;

        // Si el grano de arena está presente en el estado_actual
        if (estado_actual[current_index] == 1) {
            // Verificar si el espacio de abajo está libre
            int abajo_index = (row + 1) * WIDTH + col;
            if (estado_actual[abajo_index] == 0) {
                // Mover el grano de arena hacia abajo
                siguiente_estado[abajo_index] = current_index;
                estado_actual[current_index] = 0;
            } else {
                // Intentar mover hacia abajo a la izquierda o derecha
                int abajoIzquierda_index = (row + 1) * WIDTH + (col - 1);
                if (col > 0 && estado_actual[abajoIzquierda_index] == 0) {
                    siguiente_estado[abajoIzquierda_index] = current_index;
                    estado_actual[current_index] = 0;
                } else {
                    int abajoDerecha_index = (row + 1) * WIDTH + (col + 1);
                    if (col < WIDTH - 1 && estado_actual[abajoDerecha_index] == 0) {
                        siguiente_estado[abajoDerecha_index] = current_index;
                        estado_actual[current_index] = 0;
                    } else {
                        // Si no puede moverse, se queda en su posición actual
                        siguiente_estado[current_index] = current_index;
                    }
                }
            }
        }
    }
}

    // Ahora, calculamos el frame final con la información de siguiente_estado
    for (uint8_t row = 0; row < HEIGHT; row++) {
        for (uint8_t col = 0; col < WIDTH; col++) {
            index = row * WIDTH + col;
            if (siguiente_estado[index] != -1) {
                uint32_t origin_index = siguiente_estado[index];
                frame[index] = frame_auxiliar[origin_index];  // El grano se mueve a la nueva posición
            } else {
                frame[index] = (RGB_t){0x00, 0x00, 0x00};  // Si no se mueve, queda vacío
            }
        }
    }
    free(frame_auxiliar);
}


//=============================================================================> Video generico
void LED_Video(uint8_t *nextBuffer, RGB_t** video, uint32_t cantidad_frames){
    static uint32_t current_frame =0;
    LED_fillBuffer(video[current_frame], nextBuffer);
    current_frame = (current_frame+1)%cantidad_frames;
}

// Aplico un cambio
void LED_ApplyChanges(RGB_t *frame, frame_diff_t *frame_diference){
    // Recorro todos los cambios
    for (size_t i = 0; i < frame_diference->cantidad_de_cambios; i++){
        uint16_t indice_actual = frame_diference->array_cambios_pixeles[i].index;
        frame[indice_actual]=  frame_diference->array_cambios_pixeles[i].nuevo_valor;
    }
    
}

void LED_VideoByFrameChanges(RGB_t *frame, const video_by_changes_t* video){
    static uint32_t current_frame =0;
    if(current_frame==0){
        LED_Image(frame, video->frame_inicial);
    }else{
    	LED_ApplyChanges(frame, &video->array_diferencias_frames[current_frame-1]);
    }
    current_frame = (current_frame+1)%video->cantidad_de_frames;
    HAL_Delay(video->duracion_ms/video->cantidad_de_frames);
}




//---------------------------------------------------------------------------------------------------------------------------------------------------------------
//																	EFECTOS VISUALES TETRIS
//---------------------------------------------------------------------------------------------------------------------------------------------------------------




//=============================================================================> Pantalla de Start
 {
    static uint8_t tick = 0;
    LED_plasmaEffect(frame); // Fondo dinámico
    RGB_t white = {255, 255, 255};
    RGB_t yellow = {255, 180, 0};
    RGB_t red= {0,0,255};

    // Texto central (parpadeante)
    if ((tick++ / 10) % 2 == 0) {
        LED_Text(frame, font, "press start", 4, HEIGHT/2 - 4, white, 0);
    }

}

//=============================================================================> "Fondo" (mejorar)
void LED_TetrisBaseBackground(RGB_t *frame) {
    // Colores típicos de Tetris
    RGB_t colors[5] = {
        {0, 255, 255}, // I - cyan
        {255, 255, 0}, // O - amarillo
        {255, 0, 255}, // T - magenta
        {0, 255, 0},   // L - verde
        {255, 128, 0}  // S - naranja
    };

    static uint8_t tick = 0;
    tick++;

    // Limpiar el fondo (opcional: leve fade para que no sea tan sólido)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            frame[y * WIDTH + x].R /= 2;
            frame[y * WIDTH + x].G /= 2;
            frame[y * WIDTH + x].B /= 2;
        }
    }

    // Determinamos la "altura" de la base de Tetris
    uint32_t base_y = HEIGHT - 6 * BLOCK_SIZE; // 6 filas de bloques

    // Colocamos tetrominos aleatorios en la base
    for (uint32_t y = base_y; y < HEIGHT; y += 2*BLOCK_SIZE) {
        for (uint32_t x = 0; x < WIDTH; x += 4*BLOCK_SIZE) {
            // Tetromino aleatorio
            char types[5] = {'I','O','T','L','S'};
            char t = types[rand() % 5];

            // Rotación aleatoria
            uint8_t rot = rand() % 4;

            // Color aleatorio
            RGB_t c = colors[rand() % 5];

            // Dibujamos
            LED_tetromino_rotated(frame, c, x, y, rot, t);
        }
    }

    // Opcional: cada cierto tick, parpadea un bloque
    if (tick % 20 == 0) {
        uint32_t px = (rand() % (WIDTH / BLOCK_SIZE)) * BLOCK_SIZE;
        uint32_t py = HEIGHT - BLOCK_SIZE - (rand() % 3) * BLOCK_SIZE;
        RGB_t blink = {255, 255, 255};
        LED_rectangle(frame, blink, py, py + BLOCK_SIZE - 1, px, px + BLOCK_SIZE - 1);
    }
}



//=============================================================================> Letras estilo TETRIS
void LED_GameOverBlocks(RGB_t *frame) {
    // Tamaño de cada bloque que formará las letras
    const uint8_t BLOCK_LETTER = 2;

    // Colores posibles (pueden ser aleatorios también)
    RGB_t colors[6] = {
        {255, 0, 0},   // rojo
        {0, 255, 0},   // verde
        {0, 0, 255},   // azul
        {255, 255, 0}, // amarillo
        {255, 0, 255}, // magenta
        {0, 255, 255}  // cyan
    };



    // Posición inicial para centrar "GAME OVER"
    uint32_t start_x = WIDTH/2 - 12*BLOCK_LETTER; // aprox centrar
    uint32_t start_y = HEIGHT/2 - 5*BLOCK_LETTER - 15;

    // Definición de letras usando matriz 5x5
    uint8_t G[5][5] = {
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,0,1,1,1},
        {1,0,0,0,1},
        {1,1,1,1,1}
    };

    uint8_t A[5][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1}
    };

    uint8_t M[5][5] = {
        {1,0,0,0,1},
        {1,1,0,1,1},
        {1,0,1,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1}
    };

    uint8_t E[5][5] = {
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,0},
        {1,1,1,1,1}
    };

    uint8_t O[5][5] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0}
    };

    uint8_t V[5][5] = {
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,1,0,1,0},
        {0,0,1,0,0}
    };

    uint8_t R[5][5] = {
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,0},
        {1,0,1,0,0},
        {1,0,0,1,0}
    };

    // Letras de "GAME OVER" en orden
    uint8_t* letters[9] = { (uint8_t*)G, (uint8_t*)A, (uint8_t*)M, (uint8_t*)E,
                             (uint8_t*)O, (uint8_t*)V, (uint8_t*)E, (uint8_t*)R };

    uint32_t x_offset = start_x;
    uint32_t y_offset = start_y;

    for (int l = 0; l < 8; l++) { // 8 letras
        uint8_t (*letter)[5] = (uint8_t (*)[5])letters[l];

        for (uint8_t row = 0; row < 5; row++) {
            for (uint8_t col = 0; col < 5; col++) {
                if (letter[row][col]) {
                    // Color aleatorio
                    RGB_t c = colors[rand() % 6];

                    // Dibujar rectángulo para este “pixel”
                    LED_rectangle(frame, c,
                        y_offset + row*BLOCK_LETTER,
                        y_offset + (row+1)*BLOCK_LETTER - 1,
                        x_offset + col*BLOCK_LETTER,
                        x_offset + (col+1)*BLOCK_LETTER - 1
                    );
                }
            }
        }

        // Avanzar al próximo carácter
        x_offset += 6*BLOCK_LETTER; // 5 + 1 de espacio
        if (l == 3) { // después de "GAME", saltar línea para "OVER"
            x_offset = start_x + 2*BLOCK_LETTER; // centrado
            y_offset += 6*BLOCK_LETTER;
        }
    }
}






// //=============================================================================> Numeros TETRIS
void LED_DrawNumber(RGB_t *frame, uint32_t number, uint32_t start_x, uint32_t start_y, uint8_t BLOCK) {
    // Colores posibles
    RGB_t colors[6] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
        {255, 255, 0}, {255, 0, 255}, {0, 255, 255}
    };

    // Definición de dígitos 0–9 en matriz 5x5
    const uint8_t digits[10][5][5] = {
        { {0,1,1,1,0}, {1,0,0,0,1}, {1,0,0,0,1}, {1,0,0,0,1}, {0,1,1,1,0} }, // 0
        { {0,0,1,0,0}, {0,1,1,0,0}, {1,0,1,0,0}, {0,0,1,0,0}, {1,1,1,1,1} }, // 1
        { {0,1,1,1,0}, {1,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {1,1,1,1,1} }, // 2
        { {1,1,1,1,0}, {0,0,0,0,1}, {0,0,1,1,0}, {0,0,0,0,1}, {1,1,1,1,0} }, // 3
        { {1,0,0,1,0}, {1,0,0,1,0}, {1,1,1,1,1}, {0,0,0,1,0}, {0,0,0,1,0} }, // 4
        { {1,1,1,1,1}, {1,0,0,0,0}, {1,1,1,1,0}, {0,0,0,0,1}, {1,1,1,1,0} }, // 5
        { {0,1,1,1,0}, {1,0,0,0,0}, {1,1,1,1,0}, {1,0,0,0,1}, {0,1,1,1,0} }, // 6
        { {1,1,1,1,1}, {0,0,0,0,1}, {0,0,0,1,0}, {0,0,1,0,0}, {0,0,1,0,0} }, // 7
        { {0,1,1,1,0}, {1,0,0,0,1}, {0,1,1,1,0}, {1,0,0,0,1}, {0,1,1,1,0} }, // 8
        { {0,1,1,1,0}, {1,0,0,0,1}, {0,1,1,1,1}, {0,0,0,0,1}, {0,1,1,1,0} }  // 9
    };

    // Convertir el número a string para recorrer sus dígitos
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%u", number);

    uint32_t x_offset = start_x;
    uint32_t y_offset = start_y;

    for (int i = 0; buffer[i] != '\0'; i++) {
        uint8_t d = buffer[i] - '0';

        // Dibujar el dígito
        for (uint8_t row = 0; row < 5; row++) {
            for (uint8_t col = 0; col < 5; col++) {
                if (digits[d][row][col]) {
                    RGB_t c = colors[rand() % 6];
                    LED_rectangle(frame, c,
                        y_offset + row * BLOCK,
                        y_offset + (row + 1) * BLOCK - 1,
                        x_offset + col * BLOCK,
                        x_offset + (col + 1) * BLOCK - 1
                    );
                }
            }
        }

        // Separación entre dígitos
        x_offset += 6 * BLOCK;
    }
}
