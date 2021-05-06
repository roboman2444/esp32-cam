#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_log.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "app_roomba.h"
#include "string.h"

#ifdef CONFIG_ROOMBA_ENABLED

static const char *TAG = "Roomba";


TaskHandle_t safetytask = NULL;
int safetywaits = -1;	//wait AT LEAST x seconds to do a safety (ie, if set to 5, it will wait between 5 and 6 seconds to safety)
			//negative numbers disable
			//set to 0 means do safety as soon as possible (between 0 and 1 second)
// once a second, check safetywaits. If >0, decrement. if == 0, fire safety (and set to -1).
void roomba_safety( void * pvParameters ){
	while(1){
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if(safetywaits == 0){
			app_roomba_drive(0, 0x8000);	//this call will safetywaits--
		} else if(safetywaits > 0) safetywaits--;
	}
}





#define ROOMBAENPIN 33

void app_roomba_startup() {
/*
  gpio_set_direction(CONFIG_LED_LEDC_PIN,GPIO_MODE_OUTPUT);
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_8_BIT,            // resolution of PWM duty
    .freq_hz         = 1000,                        // frequency of PWM signal
    .speed_mode      = LEDC_LOW_SPEED_MODE,  // timer mode
    .timer_num       = CONFIG_LED_LEDC_TIMER        // timer index
  };
  ledc_channel_config_t ledc_channel = {
    .channel    = CONFIG_LED_LEDC_CHANNEL,
    .duty       = 0,
    .gpio_num   = CONFIG_LED_LEDC_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = CONFIG_LED_LEDC_TIMER
  };
  #ifdef CONFIG_LED_LEDC_HIGH_SPEED_MODE
  ledc_timer.speed_mode = ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
  #endif
  switch (ledc_timer_config(&ledc_timer)) {
    case ESP_ERR_INVALID_ARG: ESP_LOGE(TAG, "ledc_timer_config() parameter error"); break;
    case ESP_FAIL: ESP_LOGE(TAG, "ledc_timer_config() Can not find a proper pre-divider number base on the given frequency and the current duty_resolution"); break;
    case ESP_OK: if (ledc_channel_config(&ledc_channel) == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "ledc_channel_config() parameter error");
      }
      break;
    default: break;
  }
*/
/*
const int uart_num = UART_NUM_0;
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
    .rx_flow_ctrl_thresh = 122,
};
	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	*/

	gpio_set_direction(ROOMBAENPIN,GPIO_MODE_OUTPUT);
	gpio_set_level(ROOMBAENPIN, 1);

	ESP_LOGI(TAG,"ROOMBY STARTING");
//	uart_wait_tx_done(UART_NUM_0, 1000);
	uart_set_baudrate(UART_NUM_0, 57600);
	ESP_LOGI(TAG,"ROOMBY DONE");

	//create a safety task
	//hopefully this stack size is large enough
	//todo fine-tune this
	xTaskCreate(roomba_safety, "ROOMBASF", 1024, NULL, tskIDLE_PRIORITY, &safetytask );


//	uart_wait_tx_done(UART_NUM_0, 1000);


/*
	vTaskDelay(500 / portTICK_PERIOD_MS);
	printf("\nWE GAAAAN\n");
	app_roomba_connect();
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	app_roomba_clean();
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	app_roomba_poweroff();
	printf("\nAll done\n");
*/
}

static char startsci	= 0x80;
static char controlsci	= 0x82;
static char safesci	= 0x83;
static char powersci	= 0x85;
static char cleansci	= 0x87;
static char docksci	= 0x8f;
static char drivesci	= 0x89;


void app_roomba_safe(){
//	printf("%c", safesci);
	putc(safesci, stdout);
	fflush(stdout);
}
void app_roomba_connect(){
	fflush(stdout);
	gpio_set_level(ROOMBAENPIN, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(ROOMBAENPIN, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
//	printf("%c", startsci);
	putc(startsci, stdout);
	fflush(stdout);
	vTaskDelay(100 / portTICK_PERIOD_MS);
//	printf("%c", controlsci);
	putc(controlsci, stdout);
	fflush(stdout);
}
void app_roomba_poweroff(){
	//verify pin is high
//	printf("%c", powersci);
	putc(powersci, stdout);
	fflush(stdout);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(ROOMBAENPIN, 1);	//might not need this
}

void app_roomba_dock(){
//	printf("%c", docksci);
	putc(docksci, stdout);
	fflush(stdout);
}
void app_roomba_clean(){
//	printf("%c", cleansci);
	putc(cleansci, stdout);
	fflush(stdout);
}

void app_roomba_forcedock(){
	app_roomba_clean();
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	app_roomba_dock();
}

void app_roomba_shutdown() {
	if(safetytask){
		vTaskDelete(safetytask);
	}
	//force a poweroff just in case
	app_roomba_poweroff();
//	ledc_stop(LEDC_LOW_SPEED_MODE,CONFIG_LED_LEDC_CHANNEL,0);
}


void app_roomba_drive(int16_t vel, int16_t rad){
	//apparently this is big-e?
//	printf("%c%c%c%c%c", drivesci,
//		((char*)&vel)[1], ((char*)&vel)[0],
//		((char*)&rad)[1], ((char*)&rad)[0]);
	putc(drivesci, stdout);
		putc(((char*)&vel)[1], stdout);
		putc(((char*)&vel)[0], stdout);
		putc(((char*)&rad)[1], stdout);
		putc(((char*)&rad)[0], stdout);
	fflush(stdout);
	if(!vel){
		safetywaits = -1;	//disable safety timer since we are stopped anyway
	} else {
		safetywaits = 5;	//enable and set safety timer to 5+ seconds
	}
}


//destructive
void app_roomba_drive_string(char * val){
	//TODO would use sscanf for this
	int32_t vel, rad;
	char *ditz = strchr(val, ',');
	if(ditz && *ditz && *(ditz+1)){
		*ditz = 0;
		ditz++;
		vel = atoi(val);
		rad = atoi(ditz);
	} else {	//err, stop bot
		vel = 0;
		rad = 0;
	}
	if(vel > 500) vel = 500;
	else if(vel < -500) vel = -500;
	if(rad > 2000) rad = 2000;
	else if(rad < -2000) rad = -2000;
	else if(!rad) rad = 0x8000;
	app_roomba_drive(vel, rad);
}

/*
void app_roomba_set_led_intensity(uint8_t duty) {      // Turn LED On or Off
    uint8_t _duty = CONFIG_LED_MAX_INTENSITY;
    if (duty < _duty) _duty = duty;
    ledc_set_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL, _duty);
    ledc_update_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL);
    ESP_LOGI(TAG, "Set LED intensity to %d", _duty);
}
*/
#endif
