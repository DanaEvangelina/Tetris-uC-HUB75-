#pragma once
#include <stdint.h>
#include "led.h"
#include <stdbool.h>


//////////////////////////////////////////////

// Pares (Posicion, Valores)
typedef struct{
    uint16_t index;
    RGB_t nuevo_valor;
}pixel_diff_t;

// Cada nueva tiene un array de cambios
typedef struct{
    uint16_t cantidad_de_cambios;
    pixel_diff_t * array_cambios_pixeles;
}frame_diff_t;

// Cada video tiene un frame inicial y un arrau de diferencias entre frames
typedef struct{
    uint16_t cantidad_de_frames;
	RGB_t * frame_inicial;
    frame_diff_t * array_diferencias_frames;
    uint16_t duracion_ms;
}video_by_changes_t;





typedef enum {wave=0, plasma, img1, img2, adc, text, slidding_text, falling_sand, video_1, total_programs} programs_t;

void LED_waveEffect(RGB_t *frame);
void LED_plasmaEffect(RGB_t *frame);

void LED_Image(RGB_t *frame, RGB_t *image_to_copy);
void LED_rectangle(RGB_t *frame, RGB_t color, uint8_t min_row, uint8_t max_row, uint8_t min_col, uint8_t max_col);
void LED_analog_bar(RGB_t *frame, RGB_t color_1, RGB_t color_2, float percentage);
void LED_Letter(RGB_t *frame, uint8_t * letter, uint8_t x_i, uint8_t y_i, RGB_t color, uint8_t circular);
//int check_collision(uint32_t x, uint32_t y, char tetromino, uint8_t rotacion, RGB_t static_frame[HEIGHT*WIDTH]);
uint8_t get_tetromino_width(char tetromino_type, uint8_t rotation);
uint8_t get_tetromino_height(char tetromino_type, uint8_t rotation);
bool tetromino_block_active(char type, uint8_t rotation, uint8_t bx, uint8_t by) ;
void static inline draw_block_pixel(RGB_t *frame, RGB_t color, int bx, int by);
void render_board_to_frame(RGB_t *frame) ;
bool check_collision_block(char tetromino, uint8_t rotation, int x, int y);
bool check_collision_horizontal_block(char tetromino, uint8_t rotation, int desired_x, int y);
void merge_tetromino_to_board(char tetromino, RGB_t color, int x, int y, uint8_t rotation);
bool check_game_over_board(char tetromino, uint8_t rotation, int x, int y);
void board_clear_full_lines_and_score(void);
void draw_tetromino_on_frame(RGB_t *frame, RGB_t color, int bx, int by, uint8_t rotation, char type);
void LED_tetromino_rotated(RGB_t *frame, RGB_t color, uint32_t x, uint32_t y, uint8_t rotation, char tetromino_type);
void Screen_Start(RGB_t *frame, uint8_t **font);
void board_clear(void);
bool check_game_over_board_y0(char tetromino, uint8_t rotation, int x_block, int y_block);
void LED_GameOverBlocks(RGB_t *frame);
void LED_DrawNumber(RGB_t *frame, uint32_t number, uint32_t start_x, uint32_t start_y, uint8_t BLOCK);


void LED_Text(
    RGB_t *frame, 
    uint8_t **font, 
    const char *str, 
    uint8_t x_i, 
    uint8_t y_i, 
    RGB_t color, 
    uint8_t enable_line_wrap
); 

void LED_SliddingText(
    RGB_t *frame, 
    uint8_t **font, 
    const char *str, 
    uint8_t x_i, 
    uint8_t y_i, 
    RGB_t color, 
    uint8_t enable_line_wrap,
    uint8_t direction
);

void LED_FallingSand(RGB_t *frame);
void LED_VideoBuffer(RGB_t *nextBuffer, RGB_t** video);


void LED_Video(uint8_t *nextBuffer, RGB_t** video, uint32_t cantidad_frames);
void LED_ApplyChanges(RGB_t *frame, frame_diff_t* frame_difference);
void LED_VideoByFrameChanges(RGB_t *frame, const video_by_changes_t* video_frames);
