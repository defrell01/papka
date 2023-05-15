#include "mosquitto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 //#include <unistd.h>
#include <windows.h>

void on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
	int rc;
	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if (reason_code != 0) {
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	/* Making subscriptions in the on_connect() callback means that if the
	 * connection drops and is automatically resumed by the client, then the
	 * subscriptions will be recreated when the client reconnects. */
	rc = mosquitto_subscribe(mosq, NULL, "room21/conditionier", 1);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect(mosq);
	}
}


/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto* mosq, void* obj, int mid)
{
	printf("Message with mid %d has been published.\n", mid);
}

char STATUS;

/* This function pretends to read some data from a sensor and publish it.*/
//void publish_sensor_data(struct mosquitto* mosq)
//{
//	char payload[20];
//	int temp;
//	int rc;
//
//	/* Get our pretend data */
//	//temp = get_temperature();
//	/* Print it to a string for easy human reading - payload format is highly
//	 * application dependent. */
//	snprintf(payload, sizeof(payload), "%d", temp);
//
//	/* Publish the message
//	 * mosq - our client instance
//	 * *mid = NULL - we don't want to know what the message id for this message is
//	 * topic = "example/temperature" - the topic on which this message will be published
//	 * payloadlen = strlen(payload) - the length of our payload in bytes
//	 * payload - the actual payload
//	 * qos = 2 - publish with QoS 2 for this example
//	 * retain = false - do not use the retained message feature for this message
//	 */
//
//	rc = mosquitto_publish(mosq, NULL, "room211/conditionier", strlen(payload), payload, 2, false);
//	if (rc != MOSQ_ERR_SUCCESS) {
//		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
//	}
//}

void switch_mode(struct mosquitto* mosq, char mode)
{
	int rc;
	STATUS = mode;
	rc = mosquitto_publish(mosq, NULL, "room211/status", strlen((const char*)STATUS), (const char*)STATUS, 2, false);
}

void post_status(struct mosquitto* mosq)
{
	int rc;
	rc = mosquitto_publish(mosq, NULL, "room211/status", strlen((const char*)STATUS), (const char*)STATUS, 2, false);
}

void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg)
{
	if (msg->topic == "room211/conditionier")
	{
		char mode = (char) msg->payload;
		switch_mode(mosq, mode);
	}
	else if (msg->topic == "room211/status")
	{
		post_status(mosq);
	}
}

int main(int argc, char* argv[])
{
	STATUS = (char) "c_off";
	struct mosquitto* mosq;
	int rc;

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	mosq = mosquitto_new(NULL, true, NULL);
	if (mosq == NULL) {
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_publish_callback_set(mosq, on_publish);
	mosquitto_message_callback_set(mosq, on_message);

	/* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
	 * This call makes the socket connection only, it does not complete the MQTT
	 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
	 * mosquitto_loop_forever() for processing net traffic. */
	rc = mosquitto_connect(mosq, "test.mosquitto.org", 1883, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	/* Run the network loop in a background thread, this call returns quickly. */


	/* At this point the client is connected to the network socket, but may not
	 * have completed CONNECT/CONNACK.
	 * It is fairly safe to start queuing messages at this point, but if you
	 * want to be really sure you should wait until after a successful call to
	 * the connect callback.
	 * In this case we know it is 1 second before we start publishing.
	 */
	while (1) {
		rc = mosquitto_loop(mosq, 1000, 1);
		if (rc != MOSQ_ERR_SUCCESS) {
			mosquitto_destroy(mosq);
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		;
	}

	mosquitto_lib_cleanup();
	return 0;
}
