#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/device.h>
#include <stdio.h>


//#include <logging/log.h>

/* HOW TO ADD A CHARACTERISTIC */
// 1. Create a new characteristic UUID in "ble_lemon_iot_service.h" 
//		#define BT_UUID_LEMON_IOT_NEWCHARACERSTICNAME_CHRC_VAL 	BT_UUID_128_ENCODE((0x03315091, 0xf738, 0x4976, 0x8b15, 0x70449e509c9e)	// ENSURE EACH UUID IS UNIQUE!
//		#define BT_UUID_LEMON_IOT_NEWCHARACERSTICNAME_CHRC 	BT_UUID_DECLARE_128(BT_UUID_LEMON_IOT_NEWCHARACERSTICNAME_CHRC_VAL)
// 2. Add a string descriptor for your new characteristic in "ble_lemon_iot_service.h"
// 		#define BLE_LEMON_IOT_CHAR_NAME_NEWCHARACERSTICNAME	"Description of NEWCHARACERSTICNAME Characteristic"
// 3. If a the characteristic has a "write" permission, 
//	  - Add a characteristic-specific callback prototype to the  "ble_lemonIotService_cb" structure in "ble_lemon_iot_service.h"
//		void (*NEWCHARACERSTICNAMEDataReceived)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
//    - Add the callback in "ble_lemonIotService_init" function in "ble_lemon_iot_service.c"
//		lemonIoTServiceCallbacks.NEWCHARACERSTICNAMEDataReceived = lemonIotService_cb->NEWCHARACERSTICNAMEDataReceived;
//	  - Add a variable to hold the value at the top of "ble_lemon_iot_service.c"
//		static uint8_t newCharacteristicValue[6];
//	  - Add both a write callback function and prototype (near top) in "ble_lemon_iot_service.c"
//		static ssize_t ble_lemonIoT_NEWCHARACERSTICNAME_onWrite(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,uint16_t len,uint16_t offset,uint8_t flags);
//		static ssize_t ble_lemonIoT_NEWCHARACERSTICNAME_onWrite(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,uint16_t len,uint16_t offset,uint8_t flags)
//		{
//	    	if (lemonIoTServiceCallbacks.NEWCHARACERSTICNAMEDataReceived) {
//        		lemonIoTServiceCallbacks.NEWCHARACERSTICNAMEDataReceived(conn, buf, len);
//    		}
//	    	return len;
//		}
//	  - In your main file, add a callback function.
//		void NEWCHARACTERISTIC_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
//		{
//		// do something	
//		}
// 	  - In your main file, add a prototype towards the top.
//		void NEWCHARACTERISTIC_onDataReceived(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
//	  - In your main file, link the new callback funtion with ble_lemonIot_callbacks	
//		struct ble_lemonIotService_cb ble_lemonIot_callbacks = {
//			...
//			.NEWCHARACERSTICNAMEDataReceived = NEWCHARACTERISTIC_onDataReceived,
//		}
//	  - Add the characteristic to the service in "ble_lemon_iot_service.c"
//		BT_GATT_CHARACTERISTIC(BT_UUID_LEMON_IOT_NEWCHARACERSTICNAME_CHRC,BT_GATT_CHRC_WRITE_WITHOUT_RESP,BT_GATT_PERM_WRITE,NULL, ble_lemonIoT_NEWCHARACERSTICNAME_onWrite, NULL),
//	  - Add the characterisitc descriptor to the service
//		BT_GATT_CUD(BLE_LEMON_IOT_CHAR_NAME_NEWCHARACERSTICNAME, BT_GATT_PERM_READ),

/** @brief UUID of the Lemon IoT Service. Service UUID generated from here: https://www.guidgenerator.com/online-guid-generator.aspx **/
#define BT_UUID_LEMON_IOT_SERV_VAL BT_UUID_128_ENCODE(0x03315090, 0xf738, 0x4976, 0x8b15, 0x70449e509c9e)

#define BT_UUID_LEMON_IOT_SERVICE  BT_UUID_DECLARE_128(BT_UUID_LEMON_IOT_SERV_VAL)

/** @brief UUID of the MODE Characteristic. **/
#define BT_UUID_LEMON_IOT_MODE_CHRC_VAL 	BT_UUID_128_ENCODE(0x03315091, 0xf738, 0x4976, 0x8b15, 0x70449e509c9e)

#define BT_UUID_LEMON_IOT_MODE_CHRC 	BT_UUID_DECLARE_128(BT_UUID_LEMON_IOT_MODE_CHRC_VAL)


/** @brief UUID of the BUTTON Characteristic. **/
#define BT_UUID_LEMON_IOT_BUTTON_CHRC_VAL 	BT_UUID_128_ENCODE(0x03315092, 0xf738, 0x4976, 0x8b15, 0x70449e509c9e)

#define BT_UUID_LEMON_IOT_BUTTON_CHRC 	BT_UUID_DECLARE_128(BT_UUID_LEMON_IOT_BUTTON_CHRC_VAL)


/** @brief UUID of the LED Characteristic. **/
#define BT_UUID_LEMON_IOT_LED_CHRC_VAL 	BT_UUID_128_ENCODE(0x03315093, 0xf738, 0x4976, 0x8b15, 0x70449e509c9e)

#define BT_UUID_LEMON_IOT_LED_CHRC 	BT_UUID_DECLARE_128(BT_UUID_LEMON_IOT_LED_CHRC_VAL)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME)-1)


#define BLE_LEMON_IOT_CHAR_NAME_MODE	"Device mode"
#define BLE_LEMON_IOT_CHAR_NAME_BUTTON	"Push button switch"
#define BLE_LEMON_IOT_CHAR_NAME_LEDS	"RGB LED control"



enum ble_notificationsEnabled {
	BT_NOTIFICATIONS_ENABLED,
	BT_NOTIFICATIONS_DISABLED,
};

struct ble_lemonIotService_cb {
	void (*buttonNotificationChanged)(enum ble_notificationsEnabled status);
	void (*modeNotificationChanged)(enum ble_notificationsEnabled status);
	void (*modeDataReceived)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
	void (*ledDataReceived)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);	
};

void ble_ready(int err);

/** @brief Initialises the Lemon IoT BLE service. This function expects two callback functions. **/
int ble_lemonIotService_init(struct bt_conn_cb *bt_cb, struct ble_lemonIotService_cb *lemonIotService_cb);

/** @brief Update the BUTTON value in Lemon IoT service. **/
void ble_lemonIotService_button_setValue(uint8_t value);

/** @brief Update device MODE in Lemon IoT service **/
void ble_lemonIotService_mode_setValue(uint8_t value);

void led_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

//void ble_lemonIotService_led_onWrite(struct bt_conn *conn, void *user_data);


int ble_lemonIotService_button_sendNotification(struct bt_conn *conn, uint8_t value);
int ble_lemonIotService_mode_sendNotification(struct bt_conn *conn, uint8_t value);

