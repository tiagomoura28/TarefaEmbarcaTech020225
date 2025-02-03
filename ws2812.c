#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BUTTON_A_PIN 5  // Botão A no GPIO 5
#define BUTTON_B_PIN 6  // Botão B no GPIO 6


// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul

#define LED_VERMELHO_PIN 13  // Defina um pino para o LED vermelho (GPIO 13)
#define NUMERO_INICIAL 0
#define NUMERO_MIN 0
#define NUMERO_MAX 9

static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// Prototipação da função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);



// Representação de números de 0 a 9 para a matriz 5x5
bool numeros[10][NUM_PIXELS] = {
    
    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1, 
     1, 1, 1, 1, 1}, // 0

    {0, 1, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 1, 0, 0, 0, 
     0, 0, 0, 1, 0,
     0, 1, 0, 0, 0}, // 1

    {1, 1, 1, 1, 1, 
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 2

    {1, 1, 1, 1, 1, 
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 3

    {0, 1, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 1, 
     1, 0, 0, 1, 0,
     0, 1, 0, 0, 1}, // 4

    {1, 1, 1, 1, 1, 
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}, // 5

    {1, 1, 1, 1, 1, 
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}, // 6

    {0, 1, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 1, 0, 0, 0, 
     0, 0, 0, 1, 0,
     0, 1, 1, 1, 1}, // 7

    {1, 1, 1, 1, 1, 
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // 8
     
    {1, 1, 1, 1, 1, 
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}  // 9
};

// Variável global para armazenar o número atual exibido
int numero_atual = NUMERO_INICIAL;

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


void set_number_on_leds(int num)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (numeros[num][i])
        {
            put_pixel(urgb_u32(led_r, led_g, led_b));  // Acende o LED com a cor atual
        }
        else
        {
            put_pixel(0);  // Apaga o LED
        }
    }
}


    

int main()
{
    // Inicializações
    stdio_init_all(); // Inicialização a comunicação serial
    gpio_init(LED_VERMELHO_PIN);  // Inicializa o pino do LED vermelho
    gpio_set_dir(LED_VERMELHO_PIN, GPIO_OUT);  // Define o LED vermelho como saída

    gpio_init(BUTTON_A_PIN);  // Inicializa o pino do botão A
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);  // Define o botão A como entrada
    gpio_pull_up(BUTTON_A_PIN);  // Habilita o resistor de pull-up no botão A

    gpio_init(BUTTON_B_PIN);  // Inicializa o pino do botão B
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);  // Define o botão B como entrada
    gpio_pull_up(BUTTON_B_PIN);  // Habilita o resistor de pull-up no botão B

    
    // Função de interrupção com debouncing
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    set_number_on_leds(NUMERO_INICIAL);
    
    while (1)
    {
    
        // Piscar LED vermelho
    gpio_put(LED_VERMELHO_PIN, 1);  // Liga o LED vermelho
    sleep_ms(100);  // Espera 100ms (5 vezes por segundo)
    gpio_put(LED_VERMELHO_PIN, 0);  // Desliga o LED vermelho
    sleep_ms(100);  // Espera 100ms (5 vezes por segundo)
   }
    return 0;
}

   
// Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    printf("A = %d\n", a);
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento
        if (!gpio_get(BUTTON_A_PIN)){  // Botão A pressionado (ativo em nível baixo)

            if (numero_atual < NUMERO_MAX) {
                numero_atual++;
            } else {
                numero_atual = NUMERO_MIN;  // Reinicia o número ao atingir o limite
            }
            printf("Incrementando Contador MatrizLed. A = %d\n", a);
            set_number_on_leds(numero_atual);// Incrementa
            a++;
        }
        else if (!gpio_get(BUTTON_B_PIN)){            

        // Verificar o estado do botão B para decrementar o número
      
            if (numero_atual > NUMERO_MIN) {
                numero_atual--;
            } else {
                numero_atual = NUMERO_MAX;  // Reinicia o número ao atingir o limite
            }
            printf("Decrementando Contador MatrizLed. A = %d\n", a);
            set_number_on_leds(numero_atual);// Decrementa
            a++;
        }
    }
}
