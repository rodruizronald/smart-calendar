#ifndef __CALENDAR_H__
#define __CALENDAR_H__

//  Foward declaration.
class Google_OAuth2;

//*****************************************************************************
//
//! @brief Google Calendar class.
//!
//! This class uses the Google Calendar API to read user calendar events only.
//! It requires an OAuth2.0 access token to perfom the HTTP requests.
//!
//! Source: https://developers.google.com/calendar/v3/reference/events/list
//
//*****************************************************************************
class Google_Calendar
{
    private:
        //  Typedef function pointer for the user webhook reponse handler.
        typedef void (*Event_Callback)(void);
        Event_Callback callback;
        
        //  Google calendar param.
        const String CALENDAR_ID;
        const int8_t TIME_ZONE;
        
        //  Particle webhook event name.
        const String WEBHOOK_EVENT_NAME = "calendar_event";
        
        //  Calendar API event data.
        String event_location;
        String event_date_time;
        bool event_pending;
        
        //  Http status code and error response returned from webhooks.
        String http_error;
        uint16_t http_status_code;

        //  Private member functions.
        void parser(const char *event, const char *data);

    protected:
        //  Particle webhook event handlers.
        void response_handler(const char *event, const char *data);
        void error_handler(const char *event, const char *data);

    public:
        //  Class constructor.
        Google_Calendar(const String &calendar_id, const int8_t &time_zone);
        
        //  Public member functions.
        void subscribe(Event_Callback callback);
        void publish(const Google_OAuth2 &oauth2);
        bool is_event_pending(void);
        bool failed(void);
        void print_error(void);
        String get_event_location(void);
        String get_event_date_time(void);
};

#endif  //  __CALENDAR_H__