#ifndef __GEOLOCATION_H__
#define __GEOLOCATION_H__

//*****************************************************************************
//
//! @brief Google Geolocation class.
//!
//! This class uses the Google Geolocation API to locate a Particle Argon 
//! through nearby WiFi access points.
//!
//! Source: https://developers.google.com/maps/documentation/geolocation/intro
//
//*****************************************************************************
class Google_Geolocation 
{
    private:
        //  Typedef function pointer for the user webhook reponse handler.
        typedef void (*Event_Callback)(void);
        Event_Callback callback;

        //  Particle webhook event name.
        const String WEBHOOK_EVENT_NAME = "geolocation";

        //  Http status code and error response returned from webhooks.
        String http_error;
        uint16_t http_status_code;
        
        //  Geolocation API data.
        float latitud;
        float longitud;
        uint16_t accuracy;

        //  Private member functions.
        void scan_access_points(void);
        void parser(const char *event, const char *data);

    protected:
        //  Particle webhook event handlers.
        void response_handler(const char *event, const char *data);
        void error_handler(const char *event, const char *data);

    public:
        //  Class constructor.
        Google_Geolocation();
        
        //  Public member functions.
        void subscribe(Event_Callback callback);
        void publish(void); 
        bool failed(void);
        float get_lat(void);
        float get_lng(void);
        uint16_t get_accuracy(void);
        void print_error(void);
};

#endif  //  __GEOLOCATION_H__
