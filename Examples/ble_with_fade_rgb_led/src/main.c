
#include <stdio.h>
#include <string.h>
#include <hal/nrf_gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <date_time.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/settings/settings.h>

#include "ble_lemon_iot_service.h"


struct bt_conn *ble_currentConnection;

static const struct pwm_dt_spec pwm_led[] = { 	PWM_DT_SPEC_GET(DT_ALIAS(red_pwm_led)),
												PWM_DT_SPEC_GET(DT_ALIAS(green_pwm_led)),
												PWM_DT_SPEC_GET(DT_ALIAS(blue_pwm_led)) };

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});

enum device_mode{
	NORMAL,
	TEST,
};

enum led_mode{
	OFF,
	ON,
	HEARTBEAT,
	PULSING,
};

enum led_color
{
	RED,
	GREEN,
	BLUE,
};

enum led_heartrateDirection
{
	UP,
	DOWN,
};

struct rgb_led_data{
	uint8_t redLevelMax;
	uint8_t greenLevelMax;
	uint8_t blueLevelMax;
	float redLevel;
	float greenLevel;
	float blueLevel;
	float redPulseStep;
	float greenPulseStep;
	float bluePulseStep;
	uint8_t heartrateDirection;
	enum led_mode	mode;
	uint16_t periodMsX10;
	uint8_t onPeriodMsX10;
};


struct rgb_led_data rgbData;
uint8_t mode;

/* Function declarations */
void ble_connectedCallback(struct bt_conn *conn, uint8_t err);
void ble_disconnectedCallback(struct bt_conn *conn, uint8_t reason);

void ble_lemonIotService_button_onNotificationChanged(enum ble_notificationsEnabled status);
void ble_lemonIotService_mode_onNotificationChanged(enum ble_notificationsEnabled status);

void ble_lemonIotService_led_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
void ble_lemonIotService_mode_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len);

void led_initialise(uint8_t red,uint8_t green, uint8_t blue, enum led_mode newMode, uint16_t periodMs,uint8_t dutyCycle);
void led_changeMode(enum led_mode newMode);
int led_configPwm(uint8_t intensity,enum led_color color);
void led_changeColor(uint8_t red,uint8_t green, uint8_t blue);

void test_all_gpio(void);

struct bt_conn_cb ble_connectionCallback = {
	.connected 	= ble_connectedCallback,
	.disconnected 	= ble_disconnectedCallback,
};

struct ble_lemonIotService_cb ble_lemonIotServiceCallbacks = {
	.buttonNotificationChanged = ble_lemonIotService_button_onNotificationChanged,
	.modeNotificationChanged = ble_lemonIotService_mode_onNotificationChanged,
	.ledDataReceived = ble_lemonIotService_led_onDataReceived,
	.modeDataReceived = ble_lemonIotService_mode_onDataReceived,
};

void ble_connectedCallback(struct bt_conn *conn, uint8_t err)
{
	if (err) {
        printk("BTconnection failed, err %d\n", err);
		return;
    }
	else
	{
		led_changeColor(0,0x80,0);
		printk("BT Connected.\n");
	}
	ble_currentConnection = bt_conn_ref(conn);
	
}
void ble_disconnectedCallback(struct bt_conn *conn, uint8_t reason)
{

	printk("BT Disconnected (reason: %d)\n", reason);
	if(ble_currentConnection) {
		bt_conn_unref(ble_currentConnection);
		ble_currentConnection = NULL;
	}

	led_changeColor(0x80,0,0);
}

void ble_lemonIotService_button_onNotificationChanged(enum ble_notificationsEnabled status)
{
    if (status == BT_NOTIFICATIONS_ENABLED) {
        printf("BLE button characteristic: notifications enabled\n");
    } else {
        printf("BLE button characteristic: notifications disabled\n");
    }
}

void ble_lemonIotService_mode_onNotificationChanged(enum ble_notificationsEnabled status)
{
    if (status == BT_NOTIFICATIONS_ENABLED) {
        printf("BLE mode characteristic: notifications enabled\n");
    } else {
        printf("BLE mode characteristic: notifications disabled\n");
    }
}

void ble_lemonIotService_led_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint8_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    printf("Received data on conn %p. Len: %d\n", (void *)conn, len);
    printf("Data: %s\n", temp_str);

	if(len == 1)	// only Mode sent
	{
		rgbData.mode = data[0];
	}
	else if(len == 4) // LED mode + RGB data sent
	{
		rgbData.mode = data[0];
		led_changeColor(data[1],data[2], data[3]);
		
	}

}


void ble_lemonIotService_mode_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint8_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    printf("Received MODE data on conn %p. Len: %d\n", (void *)conn, len);
    printf("Data: %s\n", temp_str);
	if(len == 1)
	{
		switch (data[0])
		{
			case NORMAL:
			{
				printf("Mode changed to NORMAL\n");
				mode = 0;
				ble_lemonIotService_mode_setValue(mode);
				ble_lemonIotService_button_sendNotification(ble_currentConnection,mode);
				break;
			}
			case TEST:
			{
				printf("Mode changed to TEST\n");
				mode = 1;
				ble_lemonIotService_mode_setValue(mode);
				ble_lemonIotService_button_sendNotification(ble_currentConnection,mode);
				test_all_gpio();
				break;
			}
			default:
			{
				printf("Default case\n");
			}
		}
	}
}



K_SEM_DEFINE(pb_pushed, 0, 1);

static struct gpio_callback button_cb_data;
static struct k_work_delayable led_indicator_work;
static void led_indicator_work_function(struct k_work *work);

static uint32_t pulse_width = 0U;
//static uint8_t dir = 1U;
//static uint8_t colour = 0U;

uint8_t switchPosition;


void button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if(pins == (0x01<<button.pin))	// check mask
	{
		
		switchPosition = nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(0,button.pin));//nrf_gpio_pin_input_get(NRF_GPIO_PIN_MAP(0,button.pin)); 
		
		//k_sem_give(&pb_pushed);
		printf("Switch changed to %d, pin = %d\n", switchPosition, button.pin);
		ble_lemonIotService_button_sendNotification(ble_currentConnection,switchPosition);
	}

}

void test_all_gpio(void)
{
	int i;

	printf("Sequencing LEDs GPIO P0.00 to P0.31\n");


	// Set all GPIO pins to output
	for (i = 0; i <= 31; i++) {
		nrf_gpio_cfg(NRF_GPIO_PIN_MAP(0,i),NRF_GPIO_PIN_DIR_OUTPUT,NRF_GPIO_PIN_INPUT_DISCONNECT,NRF_GPIO_PIN_NOPULL,NRF_GPIO_PIN_S0S1,NRF_GPIO_PIN_NOSENSE);
	}

	// tests the GPIO 5 times before stopping.
	for(int j =0; j<5;j++)
	{
		for (i = 0; i <= 31; i++) {
			nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(0,i));
			k_sleep(K_MSEC(250));
			nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(0,i));
			k_sleep(K_MSEC(10));
		}
	}
	printf("Test complete!\n");
	mode = 0;

	// Update mode in the BLE service.
	ble_lemonIotService_mode_setValue(mode);
	ble_lemonIotService_button_sendNotification(ble_currentConnection,mode);

}

void test_mode(void)
{
	//int err;

	printk("\n*** Lemon IoT - LTE nRF52832 - Test Program ***\nBoard Name: %s\n", CONFIG_BOARD);

	test_all_gpio();
}

static void led_indicator_work_function(struct k_work *work)
{

	// In test mode, don't do anything with LEDs.
	if(mode == TEST)
	{
		return;
	}

	switch (rgbData.mode)
	{
		case ON:
		{
			rgbData.redLevel = rgbData.redLevelMax;
			rgbData.greenLevel	= rgbData.greenLevelMax;
			rgbData.blueLevel = rgbData.blueLevelMax;
			break;
		}
		case OFF:
		{
			rgbData.redLevel = 0;
			rgbData.greenLevel	= 0;
			rgbData.blueLevel = 0;
			break;
		}
		case HEARTBEAT:
		{

			if (rgbData.heartrateDirection == UP) 
			{
				rgbData.redLevel += rgbData.redPulseStep;
				rgbData.greenLevel += rgbData.greenPulseStep;
				rgbData.blueLevel += rgbData.bluePulseStep;

				if ((rgbData.redLevel > rgbData.redLevelMax)||(rgbData.greenLevel>rgbData.greenLevelMax)||(rgbData.blueLevel>rgbData.blueLevelMax)) {
					rgbData.redLevel = rgbData.redLevelMax;
					rgbData.greenLevel = rgbData.greenLevelMax;
					rgbData.blueLevel = rgbData.blueLevelMax;
					rgbData.heartrateDirection = DOWN;
				}
			} 
			else // direction is DOWN
			{
				if ((rgbData.redLevel < rgbData.redPulseStep)||(rgbData.greenLevel < rgbData.greenPulseStep)||(rgbData.blueLevel < rgbData.bluePulseStep))
				{
					rgbData.redLevel = 0;
					rgbData.greenLevel = 0;
					rgbData.blueLevel = 0;
					rgbData.heartrateDirection = UP;
				}
				else
				{
					rgbData.redLevel -= rgbData.redPulseStep;
					rgbData.greenLevel -= rgbData.greenPulseStep;
					rgbData.blueLevel -= rgbData.bluePulseStep;
				}

				
			}
			break;
		}
		case PULSING:
		{

			break;
		}
		default:
		{

		}
	}

	led_configPwm(rgbData.redLevel,RED);
	led_configPwm(rgbData.greenLevel,GREEN);
	led_configPwm(rgbData.blueLevel,BLUE);

	if(rgbData.mode == HEARTBEAT)
	{
		k_work_schedule(&led_indicator_work,K_MSEC(25));
	}
	else
	{
		k_work_schedule(&led_indicator_work,K_MSEC(10));
	}

	return;

	//if (ret) 
	//{
	//	printk("Error %d: failed to set pulse width\n", ret);
	//	return;
	//}

	
}

void main(void)
{
	int ret;
	int err;

	printk("PWM-based RGB LED Fade w/Test Mode\n");

	if (!device_is_ready(pwm_led[0].dev)) {
		printk("Error: PWM device %s is not ready\n", pwm_led[0].dev->name);
		return;
	}

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_callback, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	k_work_init_delayable(&led_indicator_work, led_indicator_work_function);
	k_work_schedule(&led_indicator_work, K_MSEC(10));

	k_msleep(1000);
	err = ble_lemonIotService_init(&ble_connectionCallback,&ble_lemonIotServiceCallbacks);
	if (err) {
        printf("Couldn't initialize Bluetooth. err: %d\n", err);
    }
	else
	{
		printf("Running\n");
	}
	
	led_initialise(0x00,0x00, 0x80, HEARTBEAT, 100, 1);

	while (1) {
		k_msleep(1000);

	//	k_msleep(2);

	}

	while (1) {

		if (k_sem_take(&pb_pushed, K_MSEC(100)) != 0) {
			
		} else {
			printk("Push button pressed\n");

		}
	}
}

/** @brief Initialise the LED and set the default settings 
 * @param red sets the intensity of the red LED (0x00-0xFF)
 * @param green sets the intensity of the green LED (0x00-0xFF)
 * @param blue sets the intensity of the blue LED (0x00-0xFF)
 * @param newMode sets the LED mode: OFF = 0, ON = 1, HEARTRATE = 2, PUSLING = 3
 * @param periodX10 set the period of the LED in 10's or ms. This only applies when mode = PULSING. E.g. 100 -> 1000ms = 1s
 * @param onPeriodX10 set the ON period of the LED in 10's of ms. This only applies when mode = PULSING. 1 -> 10ms
*/
void led_initialise(uint8_t red,uint8_t green, uint8_t blue, enum led_mode newMode, uint16_t periodX10,uint8_t onPeriodX10)
{
	led_changeColor(red,green,blue);
	rgbData.mode = newMode;
	rgbData.periodMsX10 = periodX10;
	rgbData.onPeriodMsX10 = onPeriodX10;
}

void led_changeColor(uint8_t red,uint8_t green, uint8_t blue)
{
	rgbData.redLevelMax = red;
	rgbData.greenLevelMax = green;
	rgbData.blueLevelMax = blue;

	rgbData.redPulseStep = (float)rgbData.redLevelMax/40;
	rgbData.greenPulseStep = (float)rgbData.greenLevelMax/40;
	rgbData.bluePulseStep = (float)rgbData.blueLevelMax/40;

	rgbData.redLevel = 0;
	rgbData.greenLevel = 0;
	rgbData.blueLevel = 0;
	rgbData.heartrateDirection = UP;

}

void led_changeMode(enum led_mode newMode)
{

}

int led_configPwm(uint8_t intensity,enum led_color color)
{
	pulse_width = ((float)intensity*pwm_led[color].period)/256;
	return pwm_set_pulse_dt(&pwm_led[color], pulse_width);
}
