#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define LED_G_PIN 11
#define LED_B_PIN 12
#define LED_R_PIN 13
#define BOTAO_A_PIN 5
#define BOTAO_B_PIN 6
#define WS2812_PIN 7
#define NUM_PIXELS 25
#define IS_RGBW false

// Variáveis globais
volatile int numero_atual = 0;
bool led_buffer[NUM_PIXELS] = {0};

// Protótipos
static void gpio_irq_handler(uint gpio, uint32_t events);
void atualizar_matriz(int numero);
static inline void put_pixel(uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void set_one_led();

int main() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    
    // Configuração dos botões
    gpio_init(BOTAO_A_PIN);
    gpio_set_dir(BOTAO_A_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_A_PIN);
    
    gpio_init(BOTAO_B_PIN);
    gpio_set_dir(BOTAO_B_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_B_PIN);
    
    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(BOTAO_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    // Inicialização da matriz WS2812
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    
    // Loop principal
    while (true) {
        // Faz o LED vermelho piscar 5 vezes por segundo
        gpio_put(LED_R_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_R_PIN, 0);
        sleep_ms(100);
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    static uint32_t ultima_acao = 0;
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    
    if (tempo_atual - ultima_acao < 200) return; // Debouncing via software
    ultima_acao = tempo_atual;
    
    if (gpio == BOTAO_A_PIN) {
        if (numero_atual < 9) numero_atual++;
        atualizar_matriz(numero_atual);
    } else if (gpio == BOTAO_B_PIN) {
        if (numero_atual > 0) numero_atual--;
        atualizar_matriz(numero_atual);
    }
}

void atualizar_matriz(int numero) {
    const bool numeros[10][NUM_PIXELS] = {
        {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}, // 0
        {0,1,1,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,0,0, 0,0,1,0,0}, // 1
        {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}, // 2
        {1,1,1,1,1, 0,0,0,0,1, 1,1,1,0,0, 0,0,0,0,1, 1,1,1,1,1}, // 3
        {1,0,0,0,0, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1}, // 4
        {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}, // 5
        {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}, // 6
        {0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 0,0,0,0,1, 1,1,1,1,1}, // 7
        {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}, // 8
        {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}  // 9
    };
    
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = numeros[numero][i];
    }
    
    set_one_led();
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_one_led() {
    uint32_t color = urgb_u32(255, 255, 255);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}
