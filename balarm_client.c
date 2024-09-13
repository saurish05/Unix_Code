// Burglar Alarm Application
// Event Driven
// Jason Losh / Bud Davis

//-----------------------------------------------------------------------------
// Compile notes for C code
//-----------------------------------------------------------------------------

// gcc -std=gnu11 -Wall -g  -o alarm_client balarm_client.c udp3.c -lpigpio
// sudo ./alarm_client 192.168.1.2

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdlib.h>          // EXIT_ codes
#include <stdbool.h>         // bool
#include <stdio.h>           // printf, scanf
#include <string.h>          // strlen, strcmp
#include <poll.h>            // poll
#include <unistd.h>          // sleep
#include <assert.h>          // assert
#include "udp3.h"            // custom udp library
#include "pigpio.h"          // RPI GPIO library

#define SERVER_LISTENER_PORT 4096

#define TIMEOUT_SECONDS 25

// GPIO pins
// TODO.  Set the constants for the GPIO pins
//        that will be used
#define DOOR       2
#define PIR        14
#define ARM_DISARM 3

#define TIMER 1         // The first timer
#define IO_DEBUG false  // Turn on for debug prints

// global variables
bool armed = false;
char *remoteIp = NULL;
int remotePort = SERVER_LISTENER_PORT;

bool isAnyKeyPressed()
{
    struct pollfd pfd;
    pfd.fd = 1;
    pfd.events = POLLIN;
    poll(&pfd, 1, 1);
    return pfd.revents == POLLIN;
}

// This function is called when the door GPIO pin changes value
void door(int gpio, int level, uint32_t tick)
{
    if (IO_DEBUG)
       printf("%d %d\n",gpio,level);
    assert(gpio==DOOR);
    // TODO:  Complete the door function as described in the
    //        assignment requirements
    if(!level && armed) {
    printf("door button");
     sendUdpData(remoteIp, remotePort, "door");
    
    }
    
}

// This function is called when the arm disarm button is pressed
void armdisarm(int gpio, int level, uint32_t tick)
{
    if (IO_DEBUG)
        printf("%d %d\n",gpio,level);
    assert(gpio==ARM_DISARM);
    if (!level)
    {
        armed=!armed;
        if (armed)
        {
            printf("Arming system\n");
            sendUdpData(remoteIp, remotePort, "armed");
        }
        else
        {
            printf("Disrming system\n");
            sendUdpData(remoteIp, remotePort, "disarmed");
        }
    }
}

// This function is called when the PIR sensor changes value
void pir(int gpio, int level, uint32_t tick)
{
    if (IO_DEBUG)
       printf("%d %d\n",gpio,level);
    assert(gpio == PIR);
    if (level && armed)
    {
       printf("motion\n");
       sendUdpData(remoteIp, remotePort, "motion");
    }
}

void timer()
{
    printf("sending pulse\n");
    sendUdpData(remoteIp, remotePort, "pulse");
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // turn buffering off for stdout
    // needed due to the asynchronous nature of the callbacks
    setbuf(stdout, NULL);

    bool quit = false;

    // Verify arguments are good
    bool goodArguments = (argc == 2);
    if (goodArguments)
        remoteIp = argv[1];
    if (!goodArguments)
    {
        printf("usage: alarm IPV4_ADDRESS\n");
        printf("  where:\n");
        printf("  IPV4_ADDRESS is address of the remote machine\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the library
    gpioInitialise();

    // Setup pins as inputs
    gpioSetMode(DOOR, PI_INPUT);
    gpioSetMode(ARM_DISARM, PI_INPUT);
    gpioSetMode(PIR, pir);
    // TODO: Add code to configure the door button, arm/disarm button, 
    //       and motion sensor pins are inputs
    // gpioSetMode(ARM_DISARM, PI_INPUT);

    // Register the callback functions
    // TODO: Add code to register the 3 callback functions.
    //       door, armdisarm, and pir
    gpioSetAlertFunc(ARM_DISARM, armdisarm);
    gpioSetAlertFunc(DOOR, door);
    gpioSetAlertFunc(PIR, pir);

    // Debounce the switch(s)
    gpioGlitchFilter(DOOR, 100 );
    gpioGlitchFilter(ARM_DISARM, 100);

    // And set up the timer callback
    gpioSetTimerFunc(TIMER, TIMEOUT_SECONDS * 1e3, timer);

    // Send disarmed message
    sendUdpData(remoteIp, remotePort, "disarmed");
    printf("disarmed\n");

    // Loop until 'q' is pressed on keyboard
    while (!quit)
    {
        // If key pressed on keyboard, get key and process
        // If 'q', quit
        if (isAnyKeyPressed())
        {
            if(fgetc(stdin)){
                    quit = true;
                
            }
        }
        if (!quit)
        {
            // Sleep for a second
            sleep(1);
        }
    }
    // Shut down the library
    gpioTerminate();
    return EXIT_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////
/* Troubleshooting / Debugging 

1/  There are some strategic print statements commented out in the code.
    Enable them.
2/  The door and arm switch should be high when disconnected.  Pull the wire
    and check it.  Then plug the wire into ground, it should then go low.
3/  run 'sudo raspoi-gpio get'
    The GPIO pins used for switch inputs need to be 'pull=UP'.  The GPIO
    pin used for the PIR sensor needs to be 'pull=NONE'.  Setting the
    pull up / down from the source code is intermittent and is broken
    regularly with software updates.  Pick GPIO pins that are set up the
    way needed.  Use those.
4/  sudo '/usr/sbin/tcpdump -i etho udp dst 192.168.1.2 port 4096 -X'
    to look at network traffic'
*/
///////////////////////////////////////////////////////////////////////////
