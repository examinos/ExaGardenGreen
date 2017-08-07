#include <application.h>

// LED instance
bc_led_t led;

uint64_t my_device_address;
uint64_t peer_device_address;

// Button instance
bc_button_t button;
bc_button_t button_5s;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        bc_radio_pub_push_button(&event_count);

        event_count++;
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        static bool base = false;
        if (base)
            bc_radio_enrollment_start();
        else
            bc_radio_enroll_to_gateway();
        base = !base;

        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
}

static void button_5s_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
	(void) self;
	(void) event_param;

	if (event == BC_BUTTON_EVENT_HOLD)
	{
		bc_radio_enrollment_stop();

		uint64_t devices_address[BC_RADIO_MAX_DEVICES];

		// Read all remote address
		bc_radio_get_peer_devices_address(devices_address, BC_RADIO_MAX_DEVICES);

		for (int i = 0; i < BC_RADIO_MAX_DEVICES; i++)
		{
			if (devices_address[i] != 0)
			{
				// Remove device
				bc_radio_peer_device_remove(devices_address[i]);
			}
		}

		bc_led_pulse(&led, 2000);
	}
}

void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    peer_device_address = bc_radio_get_event_device_address();

    if (event == BC_RADIO_EVENT_ATTACH)
    {
        bc_led_pulse(&led, 1000);
    }
    else if (event == BC_RADIO_EVENT_DETACH)
	{
		bc_led_pulse(&led, 3000);
	}
    else if (event == BC_RADIO_EVENT_ATTACH_FAILURE)
    {
        bc_led_pulse(&led, 10000);
    }

    else if (event == BC_RADIO_EVENT_INIT_DONE)
    {
        my_device_address = bc_radio_get_device_address();
    }
}

void bc_radio_on_push_button(uint64_t *peer_device_address, uint16_t *event_count)
{
	(void) peer_device_address;
	(void) event_count;

    bc_led_pulse(&led, 100);
}






static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    (void) event_param;
    float value;
    static uint8_t i2c_thermometer = 0x48;

    if (event == BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        if (bc_tag_temperature_get_temperature_celsius(self, &value))
        {
            bc_radio_pub_thermometer(i2c_thermometer, &value);

            bc_led_pulse(&led, 10);
        }
    }
}

static void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    (void) event_param;
    float value;
    static uint8_t i2c_humidity = 0x40;

    if (event == BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        if (bc_tag_humidity_get_humidity_percentage(self, &value))
        {
            bc_radio_pub_humidity(i2c_humidity, &value);

            bc_led_pulse(&led, 10);
        }
    }
}



void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_button_init(&button_5s, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button_5s, button_5s_event_handler, NULL);
    bc_button_set_hold_time(&button_5s, 5000);

    //temperature tag
    static bc_tag_temperature_t temperature_tag_0_48;
    bc_tag_temperature_init(&temperature_tag_0_48, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_48, 60000);
    bc_tag_temperature_set_event_handler(&temperature_tag_0_48, temperature_tag_event_handler, NULL);

    //humidity tag
    static bc_tag_humidity_t humidity_tag_0_40;
    bc_tag_humidity_init(&humidity_tag_0_40, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_0_40, 60000);
    bc_tag_humidity_set_event_handler(&humidity_tag_0_40, humidity_tag_event_handler, NULL);

    // Initialize radio
    bc_radio_init();
    bc_radio_set_event_handler(radio_event_handler, NULL);
    bc_radio_listen();
}
