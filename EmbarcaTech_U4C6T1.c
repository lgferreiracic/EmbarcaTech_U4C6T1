#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

int main()
{
  // Inicializa a comunicação I2C
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define a função do pino GPIO para I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define a função do pino GPIO para I2C
  gpio_pull_up(I2C_SDA); // Ativa o pull up na linha de dados
  gpio_pull_up(I2C_SCL); // Ativa o pull up na linha de clock
  ssd1306_t ssd; // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  bool cor = true;
  while (true)
  {
    cor = !cor;
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 10); // Desenha uma string
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 30); // Desenha uma string
    ssd1306_draw_string(&ssd, "U4C6T1", 60, 48); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1000);
  }
}