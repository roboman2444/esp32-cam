#ifndef _APP_ROOMBA_H_
#define _APP_ROOMBA_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ROOMBA_ENABLED
void app_roomba_startup();
void app_roomba_safe();
void app_roomba_connect();
void app_roomba_poweroff();
void app_roomba_dock();
void app_roomba_clean();
void app_roomba_forcedock();
void app_roomba_shutdown();

void app_roomba_drive(int16_t vel, int16_t rad);
void app_roomba_drive_string(char * str);	//destructive


//void app_illuminator_set_led_intensity(uint8_t duty);
#endif //CONFIG_ROOMBA_ENABLED

#ifdef __cplusplus
}
#endif

#endif
