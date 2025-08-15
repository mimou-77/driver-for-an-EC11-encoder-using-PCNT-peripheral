
/**
 * @brief in the pcnt peripheral:
 * 
 *          - interrupt on channel a :
 *                  - edge interrupt : pin A
 *                  - level to check when edge interrupt happens : pin B
 * 
 *          - interrupt on channel b :
 *                  - edge interrupt : pin B
 *                  - level to check when edge interrupt happens : pin A
 * 
 *          CONFIG:
 *              A: falling edge before B => CW
 *              B: falling edge before A => CCW
 *              example: CW then CCW
 *              1. 
 * 
 * @date 2025-08-15
 */
#define outline_check_haut 0


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
//            private fn            
//-----------------------------------------------

/**
 * @brief when the event "watchpoint reached" happens, this cb executes
 *        this cb does the following:
 *        - send watchpoint value to the queue + context switch to main() task when queue receives
 *  
 * @param unit when event happens, it checks the unit on which it happened
 * @param edata event data = event details = which watchpoint was reached
 * @param user_ctx passed from main to cb 
 * @return 
 */
static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup; //like a bool that indicates if a high priority task woke up
    
    //user_ctx is the value of the variable: QueueHandle_t in main (QueueHandle_t = QueueDefinition *)
    //queue here (inside cb) points the same memory object as main()>queue
    //=> if something is sent to cb queue <=> it is sent to main queue
    QueueHandle_t queue = (QueueHandle_t)user_ctx;

    //send event watch_point_value to queue
    //now the variable queue defined in main has watch_point_value inside it
    //giving 3 parameters to xQueueSendFromISR() <=> do a context switch before exiting the interrupt cb
    //if sending the elt to the queue caused a high priority task to wakeup [here: main() task wakes up after it's
    //been blocked by xQueueReceive(), so xQueueSendFromISR() asks for a context switch to the woken task =main()
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);

    return (high_task_wakeup == pdTRUE);
}






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




    ESP_LOGI(TAG, "add watch points and register callbacks");

    //when count reaches these specif values, an event "watchpoint reached" happens
    int watch_points[] = {PCNT_LOW_LIMIT, -50, 0, 50, PCNT_HIGH_LIMIT};

    //traverse the array from 0 to len-1 and add watch points -1000, -50, 0, 50, 1000
    //Now that we added the watchpoints: each time a watchpoint value is reached: an event happens and a cb is
    //executed ; event data is the watchpoint value
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++)
    {
        pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]);
    }

    //this is just a set_of_pcnt_cbs variable
    pcnt_event_callbacks_t cbs =
    {
        .on_reach = pcnt_on_reach
    };

    //the queue can contain up to 10 ints
    QueueHandle_t queue = xQueueCreate(10, sizeof(int));

    
    //attach cb to pcnt_unit_interrupt
    //queue is passed as a parameter to the cb when it executes
    //queue is user_ctx
    //when a watchpoint event happens: the cb executes
    //cb sends event data (= watchpoint value) to the queue with xQueueSendFromISR()
    //=> unblocks the main task blocked by xQueueReceive() and does a ctx switch to main task
    pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue);

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
        //check if the queue received an element; (timeout = 1000 ms)
        //if yes: store the received element inside the variable: event_count (elt = watchpoint value)
        //this is a blocking function, the main task remains blocked for 1000 ms until the queue received something
        //
        // when the event happens and the cb is executed: the event data is sent to the queue,
        // => the blocked main task wakes up. [ blocked by xQueueReceive() ]
        // the main task wakes up, so inside the cb: xQueueSendFromISR() sets high_task_wakeup to pdTRUE,
        // and a context switch happens towards the main task. [ ctx switch before interrupt is finished ]
        if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(500)))
        {
            //the element received by the queue is the watchpoint value
            ESP_LOGI(TAG, "Watch point event, count: %d", event_count); //log the watchpoint value
        }
        else //if the queue received nothing and timeout : log the count (from pcnt unit)
        {
            pcnt_unit_get_count(pcnt_unit, &pulse_count);
            ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
        }
    }
}
