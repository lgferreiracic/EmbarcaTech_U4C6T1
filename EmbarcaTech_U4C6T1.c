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

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define UART_TX_PIN 0
#define UART_RX_PIN 1

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

static volatile uint number_ws2812 = 0; // Variável para armazenar o número a ser exibido
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
static volatile bool cor = true; // Variável para armazenar a cor atual dos LEDs
static volatile uint display = 0; // Variável para armazenar o display atual
static volatile char character = ' '; // Variável para armazenar o character atual

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
            printf("\033[2J\033[1;1H");
            if(gpio_get(led_G))
                uart_puts(UART_ID, "Button A pressed - LED ON\r\n");
            else
                uart_puts(UART_ID, "Button A pressed - LED OFF\r\n");
        }
        else if(gpio == button_b)
        {
            display = 2;
            gpio_put(led_B, !gpio_get(led_B)); // Inverte o estado do LED azul
            display_number(10); // Limpa o display
            printf("\033[2J\033[1;1H");
            if(gpio_get(led_B))
                uart_puts(UART_ID, "Button B pressed - LED ON\r\n");
            else
                uart_puts(UART_ID, "Button B pressed - LED OFF\r\n");
        }
    }
}

// Função de callback que será chamada quando a interrupção ocorrer 
void on_uart_rx() { 
    // Enquanto houver dados para ler na UART 
    while (uart_is_readable(UART_ID)) { 
        char ch = uart_getc(UART_ID); // Lê um character da UART 
        uart_putc(UART_ID, ch); // Envia o character de volta para a UART 
        uart_puts(UART_ID, "\r\n"); // Envia uma nova linha para a UART
        if (ch >= 32 && ch <= 126) // Verifica se o character é imprimível 
        { 
            character = ch; // Armazena o character 
            display = 4; // Atualiza o display 
            if(ch >= '0' && ch <= '9') // Verifica se o character é um dígito 
            { 
                number_ws2812 = ch - '0'; // Converte o character para um número 
                display_number(number_ws2812); // Exibe o número no display 
            }else{
                display_number(10); // Limpa o display
            }
        }
    }
}

void init_serial_uart(){
    
    uart_init(UART_ID, BAUD_RATE); // Inicializa a UART0 com baud rate de 115200 
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // Configura pino 0 como TX 
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // Configura pino 1 como RX 
    uart_set_fifo_enabled(UART_ID, true); // Habilita o FIFO para evitar sobrecarga de buffer 
 
    // Configura a interrupção para a UART0 
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx); // Define a função de callback para a interrupção de recepção 
    irq_set_enabled(UART0_IRQ, true); // Habilita a interrupção na UART0 
    uart_set_irq_enables(UART_ID, true, false); // Habilita a interrupção de recepção de dados (RX) na UART 
}

void show_display_0(){
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    ssd1306_draw_string(&ssd, "Tarefa 1", 35, 30); // Desenha uma string
    ssd1306_draw_string(&ssd, "Lucas Ferreira", 10, 48); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(1000);
}

void show_display_1(){
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    ssd1306_draw_string(&ssd, "Button A", 35, 30); // Desenha uma string

    if(gpio_get(led_G))
        ssd1306_draw_string(&ssd, "Green LED ON", 15, 48); // Desenha uma string      
    else
        ssd1306_draw_string(&ssd, "Green LED OFF", 15, 48); // Desenha uma string

    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(1000);
}

void show_display_2(){
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    ssd1306_draw_string(&ssd, "Button B", 35, 30); // Desenha uma string

    if(gpio_get(led_B))
        ssd1306_draw_string(&ssd, "Blue LED ON", 20, 48); // Desenha uma string      
    else
        ssd1306_draw_string(&ssd, "Blue LED OFF", 20, 48); // Desenha uma string

    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(1000);
}

void show_display_3(){
    for (int i = 32; i < 127; i++)
    {
        cor = !cor;
        // Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
        ssd1306_draw_string(&ssd, "ASCII Table", 35, 30); // Desenha uma string
        char str[20];
        sprintf(str, "%d - Char: %c", i, character);
        ssd1306_draw_string(&ssd, str, 10, 48); // Desenha uma string
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(200);
    }
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

void show_display_4(){
    for(int i = 0; i < 3; i++){
        cor = !cor;
        // Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
        char showChar[20];
        sprintf(showChar, "Char: %c", character);
        ssd1306_draw_string(&ssd, showChar, 10, 30); // Desenha uma string

        char str[20];
        if(i == 0){
            sprintf(str, "Bin: %s", get_binary(character));
            ssd1306_draw_string(&ssd, str, 10, 48); // Desenha uma string
        }else if(i == 1){
            sprintf(str, "Dec: %d", character);
            ssd1306_draw_string(&ssd, str, 10, 48); // Desenha uma string
        }else{
            sprintf(str, "Hex: %s", get_hex(character));
            ssd1306_draw_string(&ssd, str, 10, 48); // Desenha uma string
        }
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(1000);
    }
}

int main()
{
  // Inicializações
  stdio_init_all();
  init_display();
  init_leds();
  init_buttons();
  init_serial_uart();

  //Configurações da PIO
  PIO pio = pio0; 
  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, WS2812_PIN);

  // Configuração da interrupção com callback
  gpio_set_irq_enabled_with_callback(button_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(button_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  bool cor = true;
  while (true)
  {
    switch (display)
    {
    case 0:
        show_display_0();
        break;
    case 1:
        show_display_1();
        break;
    case 2:
        show_display_2();
        break;
    case 3:
        show_display_3();
        break;
    case 4:
        show_display_4();
        break;
    default:
        break;
    }
  }

    return 0;
}



