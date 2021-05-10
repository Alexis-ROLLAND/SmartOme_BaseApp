#include <chrono>
#include <cstdint>
#include <stdio.h>

#include "mbed.h"

#include "libSmartOmeBaseApp.h"




// Misc Global Variables

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
/**
 * Entry point for application
 */
int main(void)
{   // Misc
    
    // Non LoRA initialisations
    app_init();

    // LoRA initialisations
    lora_init();

    // Run app (mainly dipatch events)
    app_run();
    
    


    return 0;
}
//----------------------------------------------------------------------------

// EOF


