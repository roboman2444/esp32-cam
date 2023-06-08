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

#define ROOMBAPOWERPACK 60
#define ROOMBAPOWERPACKDOCK 120

TaskHandle_t safetytask = NULL;
int safetywaits = -1;	//wait AT LEAST x seconds to do a safety (ie, if set to 5, it will wait between 5 and 6 seconds to safety)
			//negative numbers disable
			//set to 0 means do safety as soon as possible (between 0 and 1 second)
int powerwaits = -1;
// once a second, check safetywaits. If >0, decrement. if == 0, fire safety (and set to -1).
void roomba_safety( void * pvParameters ){
	while(1){
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if(safetywaits == 0){
			ESP_LOGI(TAG,"SAFETYWAITS HIT");
			app_roomba_drive(0, 0x8000);	//this call will safetywaits--
		} if(safetywaits >= 0) safetywaits--;
		if(powerwaits == 0){
			ESP_LOGI(TAG,"POWERWAITS HIT");
//			app_roomba_safe();
//			vTaskDelay(100 / portTICK_PERIOD_MS);
			app_roomba_poweroff();
//			powerwaits = ROOMBAPOWERPACK;
		} if(powerwaits >= 0) powerwaits--;
	}
}





#define ROOMBAENPIN 33

void app_roomba_startup() {

	ESP_LOGI(TAG,"ROOMBY STARTING");
	gpio_set_direction(ROOMBAENPIN,GPIO_MODE_OUTPUT);
	gpio_set_level(ROOMBAENPIN, 1);


	uart_config_t uart_config = {
		.baud_rate = 57600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 12, 13, -1, -1));
	const int uart_buffer_size = (UART_FIFO_LEN*2);
	QueueHandle_t uart_queue;
//	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 5, &uart_queue, 0));
//	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, 0, 5, &uart_queue, 0));
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, uart_buffer_size, 0, 0, 0, ESP_INTR_FLAG_LOWMED));

	ESP_LOGI(TAG,"ROOMBY DONE");

	//create a safety task
	//hopefully this stack size is large enough
	//todo fine-tune this
	xTaskCreate(roomba_safety, "ROOMBASF", 1024, NULL, tskIDLE_PRIORITY, &safetytask );


}

static char startsci	= 0x80;
static char controlsci	= 0x82;
static char safesci	= 0x83;
static char fullsci	= 0x84;
static char powersci	= 0x85;
static char cleansci	= 0x87;
static char sensorssci	= 0x8e;
static char docksci	= 0x8f;
static char drivesci	= 0x89;


void app_roomba_safe(){
	ESP_LOGI(TAG,"app_roomba_safe");
	uart_write_bytes(UART_NUM_1, &safesci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
}
void app_roomba_connect(){
	ESP_LOGI(TAG,"app_roomba_connect");

//	uart_wait_tx_done(UART_NUM_1, 100);
	gpio_set_level(ROOMBAENPIN, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(ROOMBAENPIN, 0);
	vTaskDelay(250 / portTICK_PERIOD_MS);
	uart_write_bytes(UART_NUM_1, &startsci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
	//todo look into using uart_write_bytes_with_break
	vTaskDelay(100 / portTICK_PERIOD_MS);
	uart_write_bytes(UART_NUM_1, &controlsci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
	powerwaits = ROOMBAPOWERPACK;
}
void app_roomba_poweroff(){
	ESP_LOGI(TAG,"app_roomba_poweroff");

	uart_write_bytes(UART_NUM_1, &powersci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(ROOMBAENPIN, 1);	//might not need this
}

void app_roomba_dock(){
	ESP_LOGI(TAG,"app_roomba_dock");

	uart_write_bytes(UART_NUM_1, &docksci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
	powerwaits = ROOMBAPOWERPACKDOCK;
}
void app_roomba_clean(){
	ESP_LOGI(TAG,"app_roomba_clean");

	uart_write_bytes(UART_NUM_1, &cleansci, 1);
//	uart_wait_tx_done(UART_NUM_1, 100);
	powerwaits = ROOMBAPOWERPACKDOCK;
}

void app_roomba_forcedock(){
	app_roomba_clean();
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	app_roomba_dock();
}

void app_roomba_shutdown() {
	ESP_LOGI(TAG,"ROOMBY SHUTDOWN");
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
/*
	putc(drivesci, stdout);
		putc(((char*)&vel)[1], stdout);
		putc(((char*)&vel)[0], stdout);
		putc(((char*)&rad)[1], stdout);
		putc(((char*)&rad)[0], stdout);
	fflush(stdout);
*/
	char drivedata[5];
		drivedata[0] = drivesci;
		drivedata[1] = ((char*)&vel)[1];
		drivedata[2] = ((char*)&vel)[0];
		drivedata[3] = ((char*)&rad)[1];
		drivedata[4] = ((char*)&rad)[0];
	uart_write_bytes(UART_NUM_1, drivedata, 5);
//	uart_wait_tx_done(UART_NUM_1, 100);

	if(!vel){
		safetywaits = -1;	//disable safety timer since we are stopped anyway
		powerwaits = ROOMBAPOWERPACK;
	} else {
		safetywaits = 5;	//enable and set safety timer to 5+ seconds
		powerwaits = ROOMBAPOWERPACK;
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
