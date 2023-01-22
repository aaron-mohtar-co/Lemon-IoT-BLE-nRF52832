#include "ble_lemon_iot_service.h"

//static uint8_t ledValues[6];

/** @brief Button value **/
static uint8_t buttonValue = 1; // active low

/** @brief Device mode **/
static uint8_t mode;

static K_SEM_DEFINE(bt_init_ok, 0, 1);	// define a semephore to wait for callback before continuing with applciaitons.

static struct ble_lemonIotService_cb lemonIoTServiceCallbacks;

// Function prototypes
static ssize_t ble_lemonIotService_mode_readCharacteristicCallback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset);
static ssize_t ble_lemonIotService_button_readCharacteristicCallback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset);
//static ssize_t readLedCharacteristicCb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
//                                            void *buf, uint16_t len, uint16_t offset);


void mode_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
void button_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

static ssize_t ble_lemonIotService_led_onWrite(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,
			  uint16_t len,uint16_t offset,uint8_t flags);

static ssize_t ble_lemonIoTService_mode_onWriteCallback(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,
			  uint16_t len,uint16_t offset,uint8_t flags);


BT_GATT_SERVICE_DEFINE(lemon_iot_srv,

    // Attribute 0
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LEMON_IOT_SERVICE), 

    // Attribute 1
    BT_GATT_CHARACTERISTIC(BT_UUID_LEMON_IOT_MODE_CHRC,
                    BT_GATT_CHRC_WRITE_WITHOUT_RESP|BT_GATT_CHRC_READ|BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ|BT_GATT_PERM_WRITE,
                    ble_lemonIotService_mode_readCharacteristicCallback, ble_lemonIoTService_mode_onWriteCallback, NULL),
    BT_GATT_CCC(mode_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD(BLE_LEMON_IOT_CHAR_NAME_MODE, BT_GATT_PERM_READ),

    // Attribute 4
    BT_GATT_CHARACTERISTIC(BT_UUID_LEMON_IOT_BUTTON_CHRC,
                    BT_GATT_CHRC_READ|BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    ble_lemonIotService_button_readCharacteristicCallback, NULL, NULL),
	BT_GATT_CCC(button_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD(BLE_LEMON_IOT_CHAR_NAME_BUTTON, BT_GATT_PERM_READ),

    // Attribute 7
	BT_GATT_CHARACTERISTIC(BT_UUID_LEMON_IOT_LED_CHRC,
                    BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                    BT_GATT_PERM_WRITE,
                    NULL, ble_lemonIotService_led_onWrite, NULL),
    BT_GATT_CUD(BLE_LEMON_IOT_CHAR_NAME_LEDS, BT_GATT_PERM_READ),

    
);

void mode_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
   bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
   //printf("Mode notifications %s\n", notif_enabled? "enabled":"disabled");
   if (lemonIoTServiceCallbacks.modeDataReceived) {
        lemonIoTServiceCallbacks.modeNotificationChanged(notif_enabled?BT_NOTIFICATIONS_ENABLED:BT_NOTIFICATIONS_DISABLED);
    }
}

void button_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
   bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
   //printf("Button notifications %s\n", notif_enabled? "enabled":"disabled");
   if (lemonIoTServiceCallbacks.buttonNotificationChanged) {
        lemonIoTServiceCallbacks.buttonNotificationChanged(notif_enabled?BT_NOTIFICATIONS_ENABLED:BT_NOTIFICATIONS_DISABLED);
    }
}

void led_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    printf("Notifications %s\n", notif_enabled? "enabled":"disabled");
    
}

static ssize_t ble_lemonIotService_button_readCharacteristicCallback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &buttonValue, sizeof(buttonValue));
}

/*
static ssize_t readLedCharacteristicCb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, ledValues, sizeof(ledValues));
}
*/

static ssize_t ble_lemonIotService_mode_readCharacteristicCallback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &mode, sizeof(mode));
}

static ssize_t ble_lemonIotService_led_onWrite(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,uint16_t len,
			  uint16_t offset,uint8_t flags)
{
    printf("Received data, handle %d, conn %p\n",
        attr->handle, (void *)conn);


    if (lemonIoTServiceCallbacks.ledDataReceived) {
        lemonIoTServiceCallbacks.ledDataReceived(conn, buf, len);
    }
    return len;
}

static ssize_t ble_lemonIoTService_mode_onWriteCallback(struct bt_conn *conn,const struct bt_gatt_attr *attr,const void *buf,uint16_t len,
			  uint16_t offset,uint8_t flags)
{
    printf("Received (MODE) data, handle %d, conn %p\n",
        attr->handle, (void *)conn);


    if (lemonIoTServiceCallbacks.modeDataReceived) {
        lemonIoTServiceCallbacks.modeDataReceived(conn, buf, len);
    }
    else
    {
        printf("Service callback NOT defined!\n");
    }
    return len;
}

void ble_lemonIotService_button_setValue(uint8_t value)
{
	buttonValue = value;
}

void ble_lemonIotService_mode_setValue(uint8_t value)
{
	mode = value;
}


static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LEMON_IOT_SERV_VAL),
};

void ble_ready(int err)
{
    if (err) {
        printf("ble_ready returned %d\n", err);
    }
	else
	{
		printf("Bluetooth Initialised\n");
	}
	k_sem_give(&bt_init_ok);
}

int ble_lemonIotService_init(struct bt_conn_cb *bt_cb, struct ble_lemonIotService_cb *lemonIotService_cb)
{
	int err = 0;
	printf("Initialisizing Bluetooth...\n");

	if (bt_cb != NULL)
    {
        printf("BT register\n");
       // bt_conn_cb_register(bt_cb);
    }

	if (bt_cb == NULL || lemonIotService_cb == NULL) {
        return NRFX_ERROR_NULL;
    }
    bt_conn_cb_register(bt_cb);

 	lemonIoTServiceCallbacks.buttonNotificationChanged = lemonIotService_cb ->buttonNotificationChanged;
    lemonIoTServiceCallbacks.modeNotificationChanged = lemonIotService_cb ->modeNotificationChanged;
	lemonIoTServiceCallbacks.ledDataReceived = lemonIotService_cb->ledDataReceived;
    lemonIoTServiceCallbacks.modeDataReceived = lemonIotService_cb->modeDataReceived;

	err = bt_enable(ble_ready);
    if (err) {
        printf("bt_enable returned %d\n", err);
        return err;
    }

    k_sem_take(&bt_init_ok, K_FOREVER);

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err){
        printf("couldn't start advertising (err = %d", err);
        return err;
    }
	else
	{
		printf("Advertising started.\n");
	}
	
	return err;
}



void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    printf("Notification sent on connection %p\n", (void *)conn);
}

int ble_lemonIotService_button_sendNotification(struct bt_conn *conn, uint8_t value)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &lemon_iot_srv.attrs[5];

    params.attr = attr;
    params.data = &value;
    params.len = 1;
    params.func = on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

int ble_lemonIotService_mode_sendNotification(struct bt_conn *conn, uint8_t value)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &lemon_iot_srv.attrs[2];

    params.attr = attr;
    params.data = &value;
    params.len = 1;
    params.func = on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}