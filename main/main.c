
#include "esp_log.h"

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/pulse_cnt.h"

#include "driver/gpio.h"



static const char * TAG = "main";
 


#define PCNT_HIGH_LIMIT 100
#define PCNT_LOW_LIMIT  -100


#define EC11_GPIO_A 7
#define EC11_GPIO_B 2

#define MAX_GLITCH_NS 1000 //1µs



//-----------------------------------------------
//            main            
//-----------------------------------------------

void app_main(void)
{

    ESP_LOGI(TAG, "install pcnt unit");
    
    //create pcnt unit
    pcnt_unit_config_t unit_config =
    {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    pcnt_new_unit(&unit_config, &pcnt_unit);

    ESP_LOGI(TAG, "set glitch filter");

    //glitch filter ; ignore short pulses that last < max_glitch_ns
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = MAX_GLITCH_NS, //1µs
    };
    pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config);

    

    ESP_LOGI(TAG, "install pcnt channels");

    //channel a config: if edge on A : check B lvl
    pcnt_chan_config_t chan_a_config =
    {
        .edge_gpio_num = EC11_GPIO_A, //changing signal (rising edge or falling edge)
        .level_gpio_num = EC11_GPIO_B //level to check when edge signal happens
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a);



    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");

    //◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘
    //edge action + lvl action on channel a
    //CONFIG says: channel a edge = pin A ; channel a level = pin B
    
    //channel_a falling edge (A falling edge): count++ ; channel_a rising edge (A rising edge): count--
    pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    //channel_a lvl (= B lvl)(checked when edge on channel_a (=edge on A)):
    //B lvl = 0 : HOLD <=> no count <=> count+=0 ; B lvl = 1 : KEEP <=> count++ or count-- (according to edge) 
    pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD);
    //◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘◘


    ESP_LOGI(TAG, "enable pcnt unit");
    pcnt_unit_enable(pcnt_unit);

    ESP_LOGI(TAG, "clear pcnt unit");
    pcnt_unit_clear_count(pcnt_unit);

    ESP_LOGI(TAG, "start pcnt unit");
    pcnt_unit_start(pcnt_unit);


    int pulse_count = 0;
    int event_count = 0;

    while (1)
    {
    
        //log count
        pcnt_unit_get_count(pcnt_unit, &pulse_count);
        ESP_LOGI(TAG, "Pulse count: %d", pulse_count);

    }
}
