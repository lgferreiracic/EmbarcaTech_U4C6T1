#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25

// Configurações dos pinos
const uint led_R = 13; // Red=> GPIO11
const uint led_B = 12; // Blue => GPIO12
const uint led_G = 11; // Green => GPIO13
const uint WS2812_PIN = 7; // WS2812 => GPIO7
const uint button_a = 5; // Botão A = 5
const uint button_b = 6; // Botão B = 6 
const uint luminosity_R = 0; // Luminosidade máxima do LED vermelho do WS2812
const uint luminosity_G = 0; // Luminosidade máxima do LED verde do WS2812
const uint luminosity_B = 50; // Luminosidade máxima do LED azul do WS2812
ssd1306_t ssd; // Inicializa a estrutura do display
char character; // Caractere recebido

static volatile uint number_ws2812 = 0; // Variável para armazenar o número a ser exibido
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
static volatile uint display = 0; // Variável para armazenar o display atual

// Buffers para armazenar quais LEDs estão ligados matriz 5x5
bool number_0[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0
};

bool number_1[NUM_PIXELS] = {
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0
};

bool number_2[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0
};

bool number_3[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0
};

bool number_4[NUM_PIXELS] = {
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0
};

bool number_5[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0
};

bool number_6[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0
};

bool number_7[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 1, 0
};

bool number_8[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0
};

bool number_9[NUM_PIXELS] = {
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0
};

// Função para converter a posição do matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

// Função para mudar o estado de um LED
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função para converter RGB para um valor de 32 bits
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Função para definir a cor de todos os LEDs
void set_ledS(bool number[NUM_PIXELS], uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(luminosity_R, luminosity_G, luminosity_B);
    
    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {   
        int x = i % 5;
        int y = i / 5;
        int index = getIndex(x, y);

        if (number[index])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

// Função para exibir o número no display
void display_number(uint number)
{
    switch (number)
    {
    case 0:
        set_ledS(number_0, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 1:
        set_ledS(number_1, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 2:
        set_ledS(number_2, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 3:
        set_ledS(number_3, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 4:
        set_ledS(number_4, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 5:
        set_ledS(number_5, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 6:
        set_ledS(number_6, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 7:
        set_ledS(number_7, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 8:
        set_ledS(number_8, luminosity_R, luminosity_G, luminosity_B);
        break;
    case 9:
        set_ledS(number_9, luminosity_R, luminosity_G, luminosity_B);
        break;
    default: // Limpa o display
        for(int i = 0; i < NUM_PIXELS; i++)
        {
            put_pixel(0);
        }
        break;
    }
}

void init_display(){
  // Inicializa a comunicação I2C
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define a função do pino GPIO para I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define a função do pino GPIO para I2C
  gpio_pull_up(I2C_SDA); // Ativa o pull up na linha de dados
  gpio_pull_up(I2C_SCL); // Ativa o pull up na linha de clock
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
}

void init_leds(){
    // Inicializa os LEDs como saída 
    gpio_init(led_R);
    gpio_set_dir(led_R, GPIO_OUT);
    gpio_init(led_B);
    gpio_set_dir(led_B, GPIO_OUT);
    gpio_init(led_G);
    gpio_set_dir(led_G, GPIO_OUT);
}

void init_buttons(){
    // Inicializa os botões como entrada
    gpio_init(button_a);
    gpio_set_dir(button_a, GPIO_IN);
    gpio_pull_up(button_a);
    gpio_init(button_b);
    gpio_set_dir(button_b, GPIO_IN);
    gpio_pull_up(button_b);
}

// Função para exibir a tela inicial
void show_display_0(){
    ssd1306_fill(&ssd, true); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, false, true); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    ssd1306_draw_string(&ssd, "Tarefa 1", 35, 30); // Desenha uma string
    ssd1306_draw_string(&ssd, "Lucas Ferreira", 10, 48); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(1000);
}

// Função para exibir a tela  do botão A
void show_display_1(){
    ssd1306_fill(&ssd, true); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, false, true); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "Button A", 35, 20); // Desenha uma string

    if(gpio_get(led_G))
        ssd1306_draw_string(&ssd, "Green LED ON", 15, 28); // Desenha uma string      
    else
        ssd1306_draw_string(&ssd, "Green LED OFF", 15, 28); // Desenha uma string

    ssd1306_send_data(&ssd); // Atualiza o display
}

// Função para exibir a tela do botão B
void show_display_2(){
    ssd1306_fill(&ssd, true); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, false, true); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "Button B", 35, 20); // Desenha uma string

    if(gpio_get(led_B))
        ssd1306_draw_string(&ssd, "Blue LED ON", 20, 28); // Desenha uma string      
    else
        ssd1306_draw_string(&ssd, "Blue LED OFF", 20, 28); // Desenha uma string

    ssd1306_send_data(&ssd); // Atualiza o display
}

char* get_binary(char c){
    static char binary[9];
    binary[0] = '\0';
    for (int i = 128; i > 0; i = i / 2)
    {
        strcat(binary, ((c & i) == i) ? "1" : "0");
    }
    return binary;
}

char* get_hex(char c){
    static char hex[3];
    sprintf(hex, "%02X", c);
    return hex;
}

// Função para exibir a tela dos caracteres
void show_display_3(char character){
    if(character == '\n' || character == '\r' || character == '\t' || character == ' '){
        return;
    }

    char showChar[20];
    char showBin[20];
    char showDec[20];
    char showHex[20];

    ssd1306_fill(&ssd, true); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, false, true); // Desenha um retângulo
    sprintf(showChar, "Char: %c", character);
    ssd1306_draw_string(&ssd, showChar, 8, 10); // Desenha uma string

    if (character <= '9' && character >= '0'){
        number_ws2812 = character - '0';
        display_number(number_ws2812);
    }else{
        display_number(10);
    }

    sprintf(showBin, "Bin: %s", get_binary(character));
    ssd1306_draw_string(&ssd, showBin, 8, 20); // Desenha uma string
    sprintf(showDec, "Dec: %d", character);
    ssd1306_draw_string(&ssd, showDec, 8, 30); // Desenha uma string
    sprintf(showHex, "Hex: %s", get_hex(character));
    ssd1306_draw_string(&ssd, showHex, 8, 40); // Desenha uma string
    ssd1306_send_data(&ssd); // Atualiza o display
}

// Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento

        if(gpio == button_a)
        {
            display = 1;
            gpio_put(led_G, !gpio_get(led_G)); // Inverte o estado do LED verde
            display_number(10); // Limpa o display
            //Limpa o terminal
            if(gpio_get(led_G))
                printf("Button A pressed - Green LED ON\r\n");
            else
                printf("Button A pressed - Green LED OFF\r\n");
            show_display_1();

        }
        else if(gpio == button_b)
        {
            gpio_put(led_B, !gpio_get(led_B)); // Inverte o estado do LED azul
            display_number(10); // Limpa o display
            if(gpio_get(led_B))
                printf("Button B pressed - Blue LED ON\r\n");
            else
                printf("Button B pressed - Blue LED OFF\r\n");
            show_display_2();
        }
    }
}

int main()
{
  // Inicializações
  stdio_init_all();
  init_display();
  init_leds();
  init_buttons();

  //Configurações da PIO
  PIO pio = pio0; 
  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, WS2812_PIN);

  // Configuração da interrupção com callback
  gpio_set_irq_enabled_with_callback(button_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(button_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  show_display_0();

  while (true){
    if(stdio_usb_connected()){
            if(scanf("%c", &character) == 1){
                show_display_3(character);
            }
        }
    }
    return 0;
}
