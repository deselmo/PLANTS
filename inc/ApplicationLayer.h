#ifndef APPLICATION_LAYER_H
#define APPLICATION_LAYER_H

#include "MicroBit.h"
#include "SerialCom.h"
#include "NetworkLayer.h"

#include <map>

//sensing interval in milliseconds
#define SENSING_INTERVAL 500

#define APPLICATION_ID 50

//Serial messages from raspy to microbit
/**
 * uint32_t microbit_id
 * uint32_t gradient_id
 * string sensor_name
 * byte min_val
 * byte max_val
 * [
 *  uint32_t min_val_threshold
 *  uint32_t max_val_threshold
 * ]
 */
#define SENSING_REQ 3

/**
 * uint32_t microbit_id
 * uint32_t gradient_id
 */
#define REMOVE_SENSING_REQ 4

//Serial messages from microbit to raspy

/**
 * uint32_t gradient_id
 * byte:
 *  0 -> fine
 *  1 -> no route to microbit
 *  2 -> microbit disconnected
 */
#define SENSING_RESP 1

/**
 * uint32_t gradient_id
 * byte:
 *  0 -> fine
 *  1 -> no route to microbit
 *  2 -> microbit disconnected
 */
#define REMOVE_SENSING_RESP 2

/**
 * uint32_t microbit_id
 * string sensor_name
 * float value
 */
#define NEW_SAMPLE 5

/**
 * uint32_t microbit_id
 * string [] sensors
 */
#define NEW_PLANT 6

/**
 * uint32_t microbit_id
 */
#define DISCONNECTED_PLANT 7


//Network messages

/**
 * SET GRADIENT Message
 * 
 * uint32_t gradient id
 * string sensor name
 * byte min_val if set to 1 then we have a min_val gradient
 * byte max_val if set to 1 then we have a max_val gradient
 * [
 *  uint32_t min_val_threshold if the min_val was 1
 *  uint32_t max_val_threshold if the max_val was 1  
 * ]
 */
#define SET_GRADIENT 1

/**
 * REMOVE_GRADIENT Message
 * 
 * uint32_t gradient id
 */
#define REMOVE_GRADIENT 2

/**
 * APP_DATA Message
 * 
 * string sensor name
 * float value
 */
#define APP_DATA 3

/**
 * MICRO_INFO Message
 * 
 * string [] sensors names
 */
#define MICRO_INFO 4


struct Message{
    uint8_t type;
    uint8_t *payload;
};

struct Gradient{

    uint32_t id;
    bool min_val;
    uint32_t min_val_threshold;
    bool max_val;
    uint32_t max_val_threshold;
};

class ApplicationLayer{

    MicroBit *uBit;
    NetworkLayer *nl;
    SerialCom *serial;
    bool sink_mode;

    std::map<uint32_t, Gradient> gradients;

    void recv_sensing_req(ManagedBuffer);
    void recv_remove_sensing_req(ManagedBuffer);

    void send_app_data(ManagedString sensor, float value);
    void send_micro_info();
    
    void recv_from_network(MicroBitEvent);

    friend void sensing_loop(void *);

public:
    
    ApplicationLayer(MicroBit*,NetworkLayer*,SerialCom*,bool);
    void init();

};





#endif