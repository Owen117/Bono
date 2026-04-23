#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"

#define PIN_LED_A         GPIO_NUM_17
#define PIN_LED_B         GPIO_NUM_4
#define PIN_BTN_R         GPIO_NUM_5
#define PIN_BTN_L         GPIO_NUM_16

#define CANAL_ADC_POT     ADC1_CHANNEL_7

#define PIN_HS_L          GPIO_NUM_18
#define PIN_LS_L          GPIO_NUM_15
#define PIN_HS_R          GPIO_NUM_13
#define PIN_LS_R          GPIO_NUM_12

#define PWM_MODO          LEDC_LOW_SPEED_MODE
#define PWM_TIMER_SEL     LEDC_TIMER_0
#define PWM_RESOLUCION    LEDC_TIMER_8_BIT
#define PWM_FREQ_HZ       100

#define PWM_CANAL_HS_L    LEDC_CHANNEL_0
#define PWM_CANAL_HS_R    LEDC_CHANNEL_1
#define PWM_DUTY_TOPE     255

#define PIN_SEG_A   GPIO_NUM_19
#define PIN_SEG_B   GPIO_NUM_21
#define PIN_SEG_C   GPIO_NUM_22
#define PIN_SEG_D   GPIO_NUM_23
#define PIN_SEG_E   GPIO_NUM_25
#define PIN_SEG_F   GPIO_NUM_33
#define PIN_SEG_G   GPIO_NUM_32

#define PIN_DIG_2    GPIO_NUM_14
#define PIN_DIG_3    GPIO_NUM_27
#define PIN_DIG_4    GPIO_NUM_26


const uint8_t tabla_digitos[10][7] = {
    {0,0,0,0,0,0,1}, // 0
    {1,0,0,1,1,1,1}, // 1
    {0,0,1,0,0,1,0}, // 2
    {0,0,0,0,1,1,0}, // 3
    {1,0,0,1,1,0,0}, // 4
    {0,1,0,0,1,0,0}, // 5
    {0,1,0,0,0,0,0}, // 6
    {0,0,0,1,1,1,1}, // 7
    {0,0,0,0,0,0,0}, // 8
    {0,0,0,0,1,0,0}  // 9
};

typedef enum {
    DIR_DETENIDO = 0,
    DIR_HORARIO,
    DIR_ANTIHORARIO
} direccion_motor_t;


static gptimer_handle_t timer_delay = NULL;


void iniciar_timer_delay(void)
{
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000  // 1 tick = 1 µs
    };
    gptimer_new_timer(&cfg, &timer_delay);
    gptimer_enable(timer_delay);
    gptimer_start(timer_delay);
}

void delay_us(uint32_t us)
{
    uint64_t inicio = 0;
    gptimer_get_raw_count(timer_delay, &inicio);
    uint64_t fin = inicio + us;

    uint64_t ahora = 0;
    do {
        gptimer_get_raw_count(timer_delay, &ahora);
    } while (ahora < fin);
}

void iniciar_pines(void)
{
    gpio_set_direction(PIN_LED_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LED_B, GPIO_MODE_OUTPUT);

    gpio_set_direction(PIN_BTN_R, GPIO_MODE_INPUT);
    gpio_set_direction(PIN_BTN_L, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_R, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_BTN_L, GPIO_PULLUP_ONLY);

    gpio_set_direction(PIN_HS_L, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LS_L, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_HS_R, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_LS_R, GPIO_MODE_OUTPUT);

    gpio_set_direction(PIN_SEG_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SEG_G, GPIO_MODE_OUTPUT);

    gpio_set_direction(PIN_DIG_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DIG_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DIG_4, GPIO_MODE_OUTPUT);
}

void iniciar_adc(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(CANAL_ADC_POT, ADC_ATTEN_DB_11);
}

void iniciar_pwm(void)
{
    ledc_timer_config_t cfg_timer = {
        .speed_mode = PWM_MODO,
        .duty_resolution = PWM_RESOLUCION,
        .timer_num = PWM_TIMER_SEL,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&cfg_timer);

    ledc_channel_config_t canal_l = {
        .gpio_num = PIN_HS_L,
        .speed_mode = PWM_MODO,
        .channel = PWM_CANAL_HS_L,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TIMER_SEL,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&canal_l);

    ledc_channel_config_t canal_r = {
        .gpio_num = PIN_HS_R,
        .speed_mode = PWM_MODO,
        .channel = PWM_CANAL_HS_R,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TIMER_SEL,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&canal_r);
}

int leer_adc_promedio(void)
{
    int acum = 0;

    for (int k = 0; k < 10; k++)
    {
        acum += adc1_get_raw(CANAL_ADC_POT);
        delay_us(200);
    }

    return acum / 10;
}

void apagar_digitos(void)
{
    gpio_set_level(PIN_DIG_2, 1);
    gpio_set_level(PIN_DIG_3, 1);
    gpio_set_level(PIN_DIG_4, 1);
}

void borrar_segmentos(void)
{
    gpio_set_level(PIN_SEG_A, 1);
    gpio_set_level(PIN_SEG_B, 1);
    gpio_set_level(PIN_SEG_C, 1);
    gpio_set_level(PIN_SEG_D, 1);
    gpio_set_level(PIN_SEG_E, 1);
    gpio_set_level(PIN_SEG_F, 1);
    gpio_set_level(PIN_SEG_G, 1);
}

void escribir_segmentos(int n)
{
    gpio_set_level(PIN_SEG_A, tabla_digitos[n][0]);
    gpio_set_level(PIN_SEG_B, tabla_digitos[n][1]);
    gpio_set_level(PIN_SEG_C, tabla_digitos[n][2]);
    gpio_set_level(PIN_SEG_D, tabla_digitos[n][3]);
    gpio_set_level(PIN_SEG_E, tabla_digitos[n][4]);
    gpio_set_level(PIN_SEG_F, tabla_digitos[n][5]);
    gpio_set_level(PIN_SEG_G, tabla_digitos[n][6]);
}

void encender_digito(int pos, int val)
{
    apagar_digitos();
    escribir_segmentos(val);

    switch (pos)
    {
        case 2: gpio_set_level(PIN_DIG_2, 0); break;
        case 3: gpio_set_level(PIN_DIG_3, 0); break;
        case 4: gpio_set_level(PIN_DIG_4, 0); break;
    }

    delay_us(2000);
    apagar_digitos();
}

void mostrar_valor(int num)
{
    if (num < 0)   num = 0;
    if (num > 100) num = 100;

    int c = num / 100;
    int d = (num / 10) % 10;
    int u = num % 10;

    if (num == 100)
        encender_digito(2, c);
    else
    {
        apagar_digitos();
        borrar_segmentos();
        delay_us(2000);
    }

    if (num >= 10)
        encender_digito(3, d);
    else
    {
        apagar_digitos();
        borrar_segmentos();
        delay_us(2000);
    }

    encender_digito(4, u);
}

static inline void ls_izq_on(void)  { gpio_set_level(PIN_LS_L, 0); }
static inline void ls_izq_off(void) { gpio_set_level(PIN_LS_L, 1); }

static inline void ls_der_on(void)  { gpio_set_level(PIN_LS_R, 0); }
static inline void ls_der_off(void) { gpio_set_level(PIN_LS_R, 1); }

void set_pwm_izq(int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    uint32_t duty = (pct * PWM_DUTY_TOPE) / 100;
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_L, duty);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_L);
}

void set_pwm_der(int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    uint32_t duty = (pct * PWM_DUTY_TOPE) / 100;
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_R, duty);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_R);
}

void cortar_pwm(void)
{
    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_L, 0);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_L);

    ledc_set_duty(PWM_MODO, PWM_CANAL_HS_R, 0);
    ledc_update_duty(PWM_MODO, PWM_CANAL_HS_R);
}

void frenar_motor(void)
{
    cortar_pwm();
    ls_izq_off();
    ls_der_off();
}

void girar_horario(int vel)
{
    frenar_motor();
    delay_us(200);

    ls_izq_off();
    ls_der_on();

    set_pwm_der(0);
    set_pwm_izq(vel);
}

void girar_antihorario(int vel)
{
    frenar_motor();
    delay_us(200);

    ls_der_off();
    ls_izq_on();

    set_pwm_izq(0);
    set_pwm_der(vel);
}

void app_main(void)
{
    iniciar_timer_delay();
    iniciar_pines();
    iniciar_adc();
    iniciar_pwm();

    int prev_btn_r = 1;
    int prev_btn_l = 1;

    gpio_set_level(PIN_LED_A, 0);
    gpio_set_level(PIN_LED_B, 0);

    apagar_digitos();
    borrar_segmentos();

    direccion_motor_t dir = DIR_DETENIDO;
    int velocidad = 0;

    frenar_motor();

    while (1)
    {
        int btn_r = gpio_get_level(PIN_BTN_R);
        int btn_l = gpio_get_level(PIN_BTN_L);

        if (prev_btn_r == 1 && btn_r == 0)
        {
            gpio_set_level(PIN_LED_A, 1);
            gpio_set_level(PIN_LED_B, 0);
            dir = DIR_HORARIO;
        }

        if (prev_btn_l == 1 && btn_l == 0)
        {
            gpio_set_level(PIN_LED_A, 0);
            gpio_set_level(PIN_LED_B, 1);
            dir = DIR_ANTIHORARIO;
        }

        prev_btn_r = btn_r;
        prev_btn_l = btn_l;

        int lectura = leer_adc_promedio();
        velocidad = (lectura * 100) / 4095;

        if (velocidad < 15)
            velocidad = 0;

        if (dir == DIR_HORARIO)
            girar_horario(velocidad);
        else if (dir == DIR_ANTIHORARIO)
            girar_antihorario(velocidad);
        else
            frenar_motor();

        for (int i = 0; i < 25; i++)
        {
            mostrar_valor(velocidad);
        }

        printf("ADC: %d | Vel: %d | Dir: %d\n", lectura, velocidad, dir);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}