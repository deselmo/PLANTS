#ifndef APPLICATION_LAYER_H
#define APPLICATION_LAYER_H

#include "MicroBit.h"
#include "SerialCom.h"
#include "NetworkLayer.h"

#include <vector>

//default sensing interval 5 min in milliseconds 
#define SENSING_INTERVAL 1000

//max waiting time for an ack
#define ACK_WAITING_TIME 2000

#define APPLICATION_ID 50
#define DEBUG 123

//Serial messages from raspy to microbit
/**
 * 
 * uint8_t resp_event
 * uint32_t microbit_id
 * string sensor_name
 * byte start_sampling
 * byte sample_rate
 * byte min_val
 * byte max_val
 * [
 *  uint32_t sample_rate
 *  uint32_t min_val_threshold
 *  uint32_t max_val_threshold
 * ]
 */
#define SENSING_REQ 2

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
 * uint32_t microbit_id
 * string sensor_name
 * float value
 */
#define NEW_SAMPLE 3

/**
 * uint32_t microbit_id
 * string description
 * string [] sensors
 */
#define NEW_PLANT 4

/**
 * uint32_t microbit_id
 */
#define DISCONNECTED_PLANT 5


//Network messages

/**
 * SET GRADIENT Message
 * 
 * string sensor name
 * byte start_sampling
 * byte sample_rate
 * byte min_val if set to 1 then we have a min_val gradient
 * byte max_val if set to 1 then we have a max_val gradient
 * [
 *  uint32_t sample_rate value if sample_rate was 1
 *  uint32_t min_val_threshold if the min_val was 1
 *  uint32_t max_val_threshold if the max_val was 1  
 * ]
 */
#define SET_GRADIENT 1

/**
 * SET GRADIENT ACK Message
 * 
 * string sensor name
 */
#define SET_GRADIENT_ACK 2
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
 * string description
 * string [] sensors names
 */
#define MICRO_INFO 4


//Private Events
#define PRIVATE_SEND_SENSING_ACK 1

struct Message{
    uint8_t type;
    uint32_t len;
    uint8_t *payload;

    static Message from(DDMessage);  
};

struct Sensor;
struct AppAndData;

class ApplicationLayer{

    MicroBit *uBit;
    NetworkLayer *nl;
    SerialCom *serial;
    ManagedString description;

    bool waiting_ack;
    uint32_t dest;
    bool connected;
    uint8_t serving;

    std::vector<Sensor *> sensors;
    bool sink_mode;

    void recv_sensing_req(ManagedBuffer);

    void update_connection_status(MicroBitEvent);
    void update_connection_status_sink();
    void send_microbit_info();
    
    void recv_from_network(MicroBitEvent);
    bool sendMessage(Message);
    
    friend void send_sensing_ack(void *);
    friend void send_new_plant(void *);
    friend void send_new_sample(void *);

    friend void sensing_loop(void *);
    friend void send_sensing_req(void *);

    void serial_send_debug(ManagedBuffer);

public:
    
    ApplicationLayer(MicroBit*, NetworkLayer*);
    void send_app_data(ManagedString sensor, float value);
    void sleep(uint32_t);
    void init(ManagedString, SerialCom *, bool);
    void addSensor(ManagedString, float (*)(Sensor *));

};

struct AppAndData{
    ApplicationLayer *app;
    uint32_t source;
    ManagedBuffer msg;
};

struct Sensor{

    ManagedString name;

    ApplicationLayer *app;
    bool active_loop;
    bool sensing;

    bool min_value;
    int min_value_threshold;
    bool max_value;
    int max_value_threshold;
    uint32_t sensing_rate;
    float (*loop)(Sensor *);
};

#endif