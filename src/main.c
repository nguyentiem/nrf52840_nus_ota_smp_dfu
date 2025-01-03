/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Bluetooth Mesh DFU Target role sample
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <stdint.h>

///////////////////////////////NUS Service /////////////////////////
#define DOOR_STATE_SERVICE 0x0002
#define DOOR_STATE_SERVICE_LEN 10

#define GDO_NAME 6

const uint32_t kAdvertisingOptions = BT_LE_ADV_OPT_CONNECTABLE;
const uint8_t kAdvertisingFlags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
const uint8_t kBTUuid[] = {BT_UUID_NUS_VAL};
const uint16_t kAdvertisingIntervalMin = 100;
const uint16_t kAdvertisingIntervalMax = 150;

static volatile bool isConnected = false;

struct bt_data mAdvertisingItems[2];
struct bt_data serviceData;

// static struct bt_conn_auth_cb sConnAuthCallbacks;
// static struct bt_conn_auth_info_cb sConnAuthInfoCallbacks;
static struct bt_nus_cb sNusCallbacks;

uint8_t BLEName[GDO_NAME];

static struct bt_conn *mBTConnection;

void RxCallback(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	printk("receive %s\n",data); 
}

void TxCallback(struct bt_conn *conn)
{
	//   GetBLESCConnection().SendMsgCallback(BLE_SEND_SUCCESS);
}

// int  Disconnect()
// {
//   int ret = 0;
//   if (GetGDONUSService().mBTConnection != NULL) {
//     ret = bt_conn_disconnect(GetGDONUSService().mBTConnection, BT_HCI_ERR_UNACCEPT_CONN_PARAM);
//     if (ret != 0) {
//       LOG_ERR("<BLE> Can't disconnect! code %d\n", ret);
//     }
//   } else {
//     LOG_ERR("<BLE> BLE is not yet connected\n");
//     ret = -1;
//   }
//   return ret;
// }

bool SendData(const char *const data, size_t length)
{

	if (bt_nus_send(mBTConnection, (uint8_t *)(data), length) != 0)
		return false;

	return true;
}

void Connected(struct bt_conn *conn, uint8_t err)
{
	if (err || !conn)
	{
		printk("<BLE> BLE Connection failed (err %u)\n", err);
		return;
	}
	if (!isConnected)
	{

		printk("connected\n");
		mBTConnection = conn;
		bt_conn_set_security(conn, BT_SECURITY_L1);

		isConnected = true;
	}
}

void Disconnected(struct bt_conn *conn, uint8_t reason)
{
	mBTConnection = NULL;
	isConnected = false;
	printk("Disconnected\n");
}

void PairingFailed(struct bt_conn *conn, enum bt_security_err reason)
{
	// printk("<BLE> BT Pairing failed to %s : reason %d\n", LogAddress(conn), (uint8_t)(reason));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = Connected,
	.disconnected = Disconnected,
};

static struct bt_nus_cb sNusCallbacks = {
	.received = RxCallback,
	.sent = TxCallback,
};


// static const struct bt_data ad[] = {
// 	BT_DATA_BYTES(BT_DATA_FLAGS, kAdvertisingFlags),
// 	// BT_DATA_BYTES(BT_DATA_NAME_COMPLETE,"Tiem"),
// };


void startNUS()
{
	uint8_t BLENameLen = strlen(bt_get_name());
	memcpy(BLEName, bt_get_name(), BLENameLen);
	mAdvertisingItems[0]= (struct bt_data){
		.type = BT_DATA_FLAGS,
		.data_len = sizeof(kAdvertisingFlags),
		.data = &kAdvertisingFlags,
	} ;

	mAdvertisingItems[1].type = BT_DATA_NAME_COMPLETE;
	mAdvertisingItems[1].data_len = BLENameLen;
	mAdvertisingItems[1].data = BLEName;

	serviceData =  (struct bt_data)BT_DATA(BT_DATA_UUID128_ALL, kBTUuid, sizeof(kBTUuid));

	int ret;
	bt_conn_cb_register(&conn_callbacks);

	ret = bt_nus_init(&sNusCallbacks);
	if (ret != 0)
	{
		printk("<BLE> BLE init fail, error code: %d\n", ret);
		return;
	}

#if defined(CONFIG_BT_FIXED_PASSKEY)
	if (bt_passkey_set(123456) != 0)
	{
		printk("error to set key\n");
	}
#endif

	struct bt_le_adv_param params = BT_LE_ADV_PARAM_INIT(kAdvertisingOptions, kAdvertisingIntervalMin, kAdvertisingIntervalMax, NULL);
	// params.id = sBtId;
	const int result = bt_le_adv_start(&params, mAdvertisingItems, ARRAY_SIZE(mAdvertisingItems), &serviceData,
									   1);
	// const int result = bt_le_adv_start(&params, ad, ARRAY_SIZE(ad), &serviceData,
	// 								   sizeof(serviceData));

	if (result != 0)
	{
		printk("Start ADV error");
	}
}

int main(void)
{

	printk("Run main app version 1.0.3\n");
	int err;

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	startNUS();

	while (1)
	{
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
