#ifndef LIB_SMARTOME_BASE_APP_H
#define LIB_SMARTOME_BASE_APP_H

#include <chrono>
#include <cstdint>
#include <stdio.h>

#include "mbed.h"

#include "lorawan/LoRaWANInterface.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "events/EventQueue.h"



#define     TX_RX_BUFFER_SIZE   30      // Max size of Tx and Rx buffers 

    
#define     FAST_DATA_MEASURE_PERIOD                    1s      // Measure each sec
#define     NOMINAL_DATA_MEASURE_PERIOD                60s      // Measure each min
 
#define     NB_MES_BETWEEN_TX                  15      // 1 Tx toutes les 15 mesures

// Sensors

// I2C

/* I2C1 */
/*
#define     I2C_SDA     PB_7
#define     I2C_SCL     PB_6
*/

/* I2C2 */
#define     I2C_SDA     PA_11
#define     I2C_SCL     PA_12


/*
* Declaration of the send_message function
*/
#define     SEND_FLAG   's'
static void send_message(void);

// App Functions
int8_t  app_init(void);         // Non LoRA initialisations (GPIOs, IRQs...)
int8_t  lora_init(void);        // LoRA initialisations
void    app_run(void);

// BP Handlers
void    bp1_handler(void);
void    bp2_handler(void);

// Sending thread
void    send_thread(void);


#endif


