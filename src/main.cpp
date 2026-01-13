#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>      // Necesario para snprintf

// LIBRERIAS PROPORCIONADAS POR EL PROFESOR
#include "HD44780.hpp"
#include "libADC.hpp"
#include "uart_buffer.hpp"


// =======================================================
// SECCIÓN 1: CONSTANTES Y VARIABLES
// =======================================================

enum GameState {
  STATE_MENU,
  STATE_OPTIONS,
  STATE_GAME_INIT,
  STATE_GAME_LOOP,
  STATE_GAME_OVER
};
GameState currentState = STATE_MENU;

#define BTN_NONE    0
#define BTN_RIGHT   1
#define BTN_UP      2
#define BTN_DOWN    3
#define BTN_LEFT    4
#define BTN_SELECT  5

#define BTN_RIGHT_ADC_MAX  50
#define BTN_UP_ADC_MAX    200
#define BTN_DOWN_ADC_MAX  400
#define BTN_LEFT_ADC_MAX  600
#define BTN_SELECT_ADC_MAX 800

unsigned int gameDelay_ms = 200;
unsigned int adcValue = 512;
const unsigned int DEBOUNCE_DELAY_MS = 200;

// Variables de Texto
char menu_start[] = "  Start Game    ";
char menu_options[] = "  Options       ";
char menu_cursor[] = "> ";

// Variables de Juego
unsigned int playerScore = 0;
unsigned int player_X = 1;
const int WALK_Y = 1;
const int JUMP_Y = 0;
int obstacle_X = -1;
int bird_X = -1;
bool isJumping = false;
bool isDucking = false;
unsigned int jump_timer = 0;
static int sameObstacleCount = 0;

// Variable para la animación (0 = Piernas A, 1 = Piernas B)
static int anim_frame = 0; 

// Custom chars
typedef unsigned char u8;

// Frame 1: Piernas juntas/rectas (Index 0)
const u8 dino_char[]   = { 0x0E,0x1F,0x15,0x1F,0x0C,0x18,0x18,0x00 }; 
// Frame 2: Piernas separadas/paso (Index 4) - NUEVO
const u8 dino_walk_2[] = { 0x0E,0x1F,0x15,0x1F,0x0C,0x18,0x0C,0x00 }; 

const u8 cactus_char[] = { 0x04,0x0E,0x1F,0x1F,0x0A,0x0A,0x0A,0x00 };
const u8 bird_char[]   = { 0x00,0x00,0x01,0x02,0x04,0x18,0x18,0x00 };
const u8 duck_char[]   = { 0x00,0x00,0x0E,0x1F,0x15,0x0E,0x00,0x00 };

// PROTOTIPOS
void apply_game_speed();
int get_button_press();
void LCD_createChar(u8 index, const u8 *charmap);

void run_menu();
void run_options();
void run_game_init();
void run_game_loop();
void run_game_over();


// =======================================================
// SECCIÓN 2: FUNCIONES AUXILIARES
// =======================================================

int get_button_press() {
    int adc = ADC_conversion();
    if (adc < BTN_RIGHT_ADC_MAX) return BTN_RIGHT;
    if (adc < BTN_UP_ADC_MAX)    return BTN_UP;
    if (adc < BTN_DOWN_ADC_MAX)  return BTN_DOWN;
    if (adc < BTN_LEFT_ADC_MAX)  return BTN_LEFT;
    if (adc < BTN_SELECT_ADC_MAX) return BTN_SELECT;
    return BTN_NONE;
}

void apply_game_speed() {
    gameDelay_ms = 350 - (300 * adcValue / 1023);
    if (gameDelay_ms < 50) gameDelay_ms = 50;
}

void LCD_createChar(u8 index, const u8 *charmap) {
    LCD_WriteCommand(HD44780_CGRAM_SET | (index << 3));
    for (int i = 0; i < 8; i++) LCD_WriteData(charmap[i]);
}


// =======================================================
// SECCIÓN 3: ESTADOS
// =======================================================

void run_menu() {
    static int menuOption = 0;

    LCD_GoTo(0, 0);
    if (menuOption == 0) {
        LCD_WriteText(menu_cursor);
        LCD_GoTo(2, 0);
        LCD_WriteText(menu_start + 2);
    } else LCD_WriteText(menu_start);

    LCD_GoTo(0, 1);
    if (menuOption == 1) {
        LCD_WriteText(menu_cursor);
        LCD_GoTo(2, 1);
        LCD_WriteText(menu_options + 2);
    } else LCD_WriteText(menu_options);

    int btn = get_button_press();
    if (btn != BTN_NONE) {
        if (btn == BTN_UP || btn == BTN_DOWN)
            menuOption = (menuOption == 0) ? 1 : 0;

        if (btn == BTN_RIGHT || btn == BTN_SELECT) {
            LCD_Clear();
            currentState = (menuOption == 0) ? STATE_GAME_INIT : STATE_OPTIONS;
        }
        _delay_ms(DEBOUNCE_DELAY_MS);
    }
}

void run_options() {
    int btn = get_button_press();
    if (btn != BTN_NONE) {
        if (btn == BTN_UP)   adcValue = (adcValue >= 1000) ? 1023 : adcValue + 50;
        if (btn == BTN_DOWN) adcValue = (adcValue <= 50) ? 0 : adcValue - 50;

        if (btn == BTN_LEFT) {
            apply_game_speed();
            currentState = STATE_MENU;
            LCD_Clear();
            return;
        }
        _delay_ms(DEBOUNCE_DELAY_MS);
    }

    LCD_GoTo(0, 0);
    LCD_WriteText("Speed: ");
    int numBlocks = (10 * adcValue) / 1023;
    for (int i = 0; i < 10; i++)
        LCD_WriteData(i < numBlocks ? 0xFF : ' ');

    LCD_GoTo(0, 1);
    LCD_WriteText("Delay: ");

    apply_game_speed();
    char delay_str[8];
    snprintf(delay_str, sizeof(delay_str), "%u", gameDelay_ms);
    LCD_WriteText(delay_str);
    LCD_WriteText("ms   ");
}

void run_game_init() {
    LCD_Clear();

    // Cargamos los caracteres en la memoria del LCD
    LCD_createChar(0, dino_char);   // Frame A (Piernas 1)
    LCD_createChar(4, dino_walk_2); // Frame B (Piernas 2) - NUEVO
    LCD_createChar(1, cactus_char);
    LCD_createChar(2, bird_char);
    LCD_createChar(3, duck_char);

    apply_game_speed();
    playerScore = 0;
    obstacle_X = -1;
    bird_X = -1;
    isJumping = false;
    isDucking = false;
    jump_timer = 0;
    sameObstacleCount = 0;
    anim_frame = 0; // Reiniciar animación
}

void run_game_loop() {
    int btn = get_button_press();

    if ((btn == BTN_UP || btn == BTN_SELECT) && jump_timer == 0 && !isDucking) {
        isJumping = true;
        jump_timer = 4;
    }

    if (btn == BTN_DOWN && jump_timer == 0) isDucking = true;
    else isDucking = false;

    if (btn == BTN_LEFT) {
        currentState = STATE_MENU;
        LCD_Clear();
        return;
    }

    if (playerScore > 0 && playerScore % 10 == 0) {
        if (gameDelay_ms > 50) {
            unsigned int new_delay = 350 - (300 * adcValue / 1023) - (playerScore * 2);
            gameDelay_ms = (new_delay < 50) ? 50 : new_delay;
        }
    }

    // Espera del frame
    for (unsigned int i = 0; i < gameDelay_ms; i++) _delay_ms(1);

    // --- ANIMACIÓN ---
    // Alternamos el frame cada ciclo de juego (movimiento de piernas)
    anim_frame = !anim_frame;

    if (jump_timer > 0) {
        jump_timer--;
        isJumping = (jump_timer > 2);
    }

    obstacle_X--;
    bird_X--;

    if (obstacle_X < -1 && bird_X < -1) {
        playerScore++;

        if (sameObstacleCount >= 3) {
            bird_X = 15;
            obstacle_X = -2;
            sameObstacleCount = 0;
        }
        else if (TCNT0 % 2 == 0) {
            obstacle_X = 15;
            bird_X = -2;
            sameObstacleCount++;
        } else {
            bird_X = 15;
            obstacle_X = -2;
            sameObstacleCount = 0;
        }
    }

    if (obstacle_X == player_X && !isJumping) {
        currentState = STATE_GAME_OVER;
        return;
    }

    if (bird_X == player_X && !isDucking) {
        currentState = STATE_GAME_OVER;
        return;
    }

    LCD_Clear();

    // --- DIBUJAR JUGADOR CON ANIMACIÓN ---
    int current_Y = WALK_Y;
    u8 current_char = 0;

    if (isJumping) {
        current_Y = JUMP_Y;
        current_char = 0; // Saltando: Frame fijo
    } else if (isDucking) {
        current_Y = WALK_Y;
        current_char = 3; // Agachado
    } else {
        current_Y = WALK_Y;
        // Caminando: Alternar entre frame 0 y frame 4
        current_char = (anim_frame) ? 4 : 0; 
    }

    LCD_GoTo(player_X, current_Y);
    LCD_WriteData(current_char);

    if (obstacle_X >= 0 && obstacle_X < 16) {
        LCD_GoTo(obstacle_X, WALK_Y);
        LCD_WriteData(1);
    }

    if (bird_X >= 0 && bird_X < 16) {
        LCD_GoTo(bird_X, JUMP_Y);
        LCD_WriteData(2);
    }

    char score_str[8];
    snprintf(score_str, sizeof(score_str), "%u", playerScore);

    LCD_GoTo(10, 0);
    LCD_WriteText("P:");
    LCD_WriteText(score_str);
}

void run_game_over() {
    LCD_Clear();

    LCD_GoTo(3, 0);
    LCD_WriteText("GAME OVER!");

    LCD_GoTo(0, 1);
    LCD_WriteText("Score:");

    char score_str[8];
    snprintf(score_str, sizeof(score_str), "%u", playerScore);
    LCD_WriteText(score_str);

    _delay_ms(2000);

    LCD_GoTo(0, 0);
    LCD_WriteText("PRESS TO START");

    int btn = BTN_NONE;
    while (btn == BTN_NONE) {
        btn = get_button_press();
        _delay_ms(50);
    }

    currentState = STATE_MENU;
    LCD_Clear();
}


// =======================================================
// MAIN
// =======================================================

int main(void) {
    sei();

    LCD_Initalize();
    ADC_Init();

    LCD_Clear();
    LCD_GoTo(2, 0);
    LCD_WriteText("RetroMini UNO");
    _delay_ms(1500);

    currentState = STATE_MENU;
    LCD_Clear();

    while (1) {
        switch (currentState) {
            case STATE_MENU:        run_menu(); break;
            case STATE_OPTIONS:     run_options(); break;
            case STATE_GAME_INIT:   run_game_init(); currentState = STATE_GAME_LOOP; break;
            case STATE_GAME_LOOP:   run_game_loop(); break;
            case STATE_GAME_OVER:   run_game_over(); break;
        }
    }
    return 0;
} 