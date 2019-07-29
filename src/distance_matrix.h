#ifndef __DISTANCE_MATRIX_H__
#define __DISTANCE_MATRIX_H__

//*****************************************************************************
//
//	The following are enumeration classes for the travel modes and transit
//  modes available in the Distance Matrix Event.
//
//*****************************************************************************

enum class Distance_Matrix_Travel_Mode : uint8_t
{
    DRIVING,
    TRANSIT
};

enum class Distance_Matrix_Transit_Mode : uint8_t
{
    BUS,
    SUBWAY,
    TRAIN,
    TRAM,
    RAIL,
    NONE
};

//*****************************************************************************
//
//! @brief Google Distance Martrix class.
//!
//! This class uses the Google Distance Matrix API to get the travel duration 
//! and distance between to points either driving or using public transport.
//!
//! Source: https://developers.google.com/maps/documentation/distance-matrix/intro
//
//*****************************************************************************
class Google_Distance_Matrix
{
    private:
        //  Distance Matrix event structure.
        struct distance_matrix_event
        {
            public:
                //  To calculate travel distance and time:
                //  1. Starting point: In the form of latitude/longitude coordinates.
                float origin_lat;
                float origin_lng;
                //  2. Finishing point: In the form of an address.
                String destination;
                //  Preferred modes of travel and transit.
                Distance_Matrix_Travel_Mode travel_mode;
                Distance_Matrix_Transit_Mode transit_mode;
                //  Default constructor.
                distance_matrix_event()
                    : origin_lat(0.0), origin_lng(0.0), destination("none"),
                        travel_mode(Distance_Matrix_Travel_Mode::DRIVING),
                            transit_mode(Distance_Matrix_Transit_Mode::NONE) {};
        };

        //  Typedef function pointer for the user webhook reponse handler.
        typedef void (*Event_Callback)(void);
        Event_Callback callback;
        
        //  Particle webhooks event name.
        String WEBHOOK_EVENT_NAME;
        const String WEBHOOK_DISTANCE_DRIVING = "dist_driving";
        const String WEBHOOK_DISTANCE_TRANSIT = "dist_transit";

        //  Distance Matrix API data.
        uint32_t duration_to_dest;
        uint16_t distance_to_dest;
        
        //  Http status code and error response returned from webhooks.
        String http_error;
        uint16_t http_status_code;
        
        //  Private member functions.
        void parser(const char *event, const char *data);

    protected:
        //  Particle webhooks event handler.
        void response_handler(const char *event, const char *data);

    public:
        //  Typedef struct to specify the Particle webhook params.
        typedef struct distance_matrix_event Distance_Matrix_Event;
        
        //  Class constructor.
        Google_Distance_Matrix();

        //  Public member functions.
        void subscribe(const Distance_Matrix_Event &event, Event_Callback callback);
        void publish(const Distance_Matrix_Event &event);
        bool failed(void);
        void print_error(void);
        uint32_t get_duration_to_dest(void);
        uint16_t get_distance_to_dest(void);
};

#endif  //  __DISTANCE_MATRIX_H__