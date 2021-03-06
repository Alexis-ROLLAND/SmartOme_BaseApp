#include "libSmartOmeBaseApp.h"

#ifndef     HELPER_ALREADY_INCLUDED
#define     HELPER_ALREADY_INCLUDED
#include    "lora_radio_helper.h"
#endif
//---------------------------------------------------------------------------
//----------------------------------------------------------------------------
//using namespace events;

using namespace events;

// Max payload size can be LORAMAC_PHY_MAXPAYLOAD.
// This example only communicates with much shorter messages (<30 bytes).
// If longer messages are used, these buffers must be changed accordingly.
uint8_t     tx_buffer[TX_RX_BUFFER_SIZE];
uint8_t     rx_buffer[TX_RX_BUFFER_SIZE];
uint16_t    packet_len;         // Real size of data to Tx
 
/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
 * Maximum number of retries for CONFIRMED messages before giving up
 */
#define CONFIRMED_MSG_RETRY_COUNTER     3

/**
* This event queue is the global event queue for both the
* application and stack. To conserve memory, the stack is designed to run
* in the same thread as the application and the application is responsible for
* providing an event queue to the stack that will be used for ISR deferment as
* well as application information event queuing.
*/
static EventQueue ev_queue(MAX_NUMBER_OF_EVENTS * EVENTS_EVENT_SIZE);

/**
 * Event handler.
 *
 * This will be passed to the LoRaWAN stack to queue events for the
 * application which in turn drive the application.
 */
static void lora_event_handler(lorawan_event_t event);

/**
 * Constructing Mbed LoRaWANInterface and passing it the radio object from lora_radio_helper.
 */
static LoRaWANInterface lorawan(radio);

/**
 * Application specific callbacks
 */
static lorawan_app_callbacks_t callbacks;


/**
 * Threading stuff
 */
 // Threading stuff
Thread  send_th;

// Create a thread that'll run the event queue's dispatch function
Thread t;

//----------------------------------------------------------------------------
// BP & Leds stuff
DigitalOut  RedLed(PB_11);
InterruptIn Button1(PA_0);
InterruptIn Button2(PA_1);

//----------------------------------------------------------------------------
int8_t  app_init(void)
{
    RedLed = 0;
    Button1.mode(PullUp);
    Button1.fall(&bp1_handler);
    Button2.mode(PullUp);
    Button2.fall(&bp2_handler);

    return 0;
}
//----------------------------------------------------------------------------
int8_t  lora_init(void)
{
    // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;

    // Initialize LoRaWAN stack
    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        printf("\r\n LoRa initialization failed! \r\n");
        return -1;
    }

    #ifdef VERBOSE_MODE
    printf("\r\n Mbed LoRaWANStack initialized \r\n");
    #endif

    // prepare application callbacks
    callbacks.events = mbed::callback(lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        printf("\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        return -1;
    }

    #ifdef VERBOSE_MODE
    printf("\r\n CONFIRMED message retries : %d \r\n",
           CONFIRMED_MSG_RETRY_COUNTER);
    #endif

    // Enable adaptive data rate
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        printf("\r\n enable_adaptive_datarate failed! \r\n");
        return -1;
    }

    #ifdef VERBOSE_MODE
    printf("\r\n Adaptive data  rate (ADR) - Enabled \r\n");
    #endif

    retcode = lorawan.connect();

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("\r\n Connection error, code = %d \r\n", retcode);
        return -1;
    }

    #ifdef VERBOSE_MODE
    printf("\r\n Connection - In Progress ...\r\n");
    #endif
    
    return 0;
}
//----------------------------------------------------------------------------
void    app_run(void)
{
    // make your event queue dispatching events forever
    ev_queue.dispatch_forever();
    t.start(callback(&ev_queue, &EventQueue::dispatch_forever));
}
//----------------------------------------------------------------------------
/**
 * Sends a message to the Network Server
 */
static void send_message()
{
    
    int16_t retcode;
    
    retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packet_len,
                           MSG_UNCONFIRMED_FLAG);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - WOULD BLOCK\r\n")
        : printf("\r\n send() - Error code %d \r\n", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                ev_queue.call_in(3s, send_message);
            }
        }
        return;
    }

    #ifdef VERBOSE_MODE
    printf("\r\n %d bytes scheduled for transmission \r\n", retcode);
    #endif

    memset(tx_buffer, 0, sizeof(tx_buffer));
}
//----------------------------------------------------------------------------
/**
 * Receive a message from the Network Server
 */
static void receive_message()
{
    uint8_t port;
    int flags;
    int16_t retcode = lorawan.receive(rx_buffer, sizeof(rx_buffer), port, flags);

    if (retcode < 0) {
        printf("\r\n receive() - Error code %d \r\n", retcode);
        return;
    }

    printf(" RX Data on port %u (%d bytes): ", port, retcode);
    for (uint8_t i = 0; i < retcode; i++) {
        printf("%02x ", rx_buffer[i]);
    }
    printf("\r\n");
    
    memset(rx_buffer, 0, sizeof(rx_buffer));
}
//----------------------------------------------------------------------------
/**
 * Event handler
 */
static void lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            #ifdef VERBOSE_MODE
            printf("\r\n Connection - Successful \r\n");
            #endif 

            // Start  threads
            send_th.start(send_thread);
            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            #ifdef VERBOSE_MODE
            printf("\r\n Disconnected Successfully \r\n");
            #endif

            break;
        case TX_DONE:
            #ifdef VERBOSE_MODE
            printf("\r\n Message Sent to Network Server \r\n");
            #endif
            
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("\r\n Transmission Error - EventCode = %d \r\n", event);
            // try again
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case RX_DONE:
            printf("\r\n Received message from Network Server \r\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("\r\n Error in reception - Code = %d \r\n", event);
            break;
        case JOIN_FAILURE:
            printf("\r\n OTAA Failed - Check Keys \r\n");
            break;
        case UPLINK_REQUIRED:
            printf("\r\n Uplink required by NS \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
    
        default:
            MBED_ASSERT("Unknown Event");
    }
}
//----------------------------------------------------------------------------
void    bp1_handler(void)
{
    RedLed = !RedLed;     // For test purposes only
    
}

//----------------------------------------------------------------------------
void    bp2_handler(void)
{
    RedLed = !RedLed;     // For test purposes only
    

}
//----------------------------------------------------------------------------
// Send thread
void    send_thread(void)
{
    while(1)
    {
        ThisThread::sleep_for(10s);
        
        tx_buffer[0] = 0x55;         
        tx_buffer[1] = 0xAA;         
        packet_len = 2;      

        send_message();
    }
}
//----------------------------------------------------------------------------

//EOF


