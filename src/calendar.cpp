#include "Particle.h"
#include "calendar.h"
#include "utility.h"
#include "oauth2.h"
#include "http_status.h"

//*****************************************************************************
//
//! @brief Google Calendar class constructor.
//!
//!	@param[in] calendar_id Calendar identifier used for the API requests. The 
//!                        primary calendar named "Events" uses your gmail
//!                        as ID.
//!	@param[in] time_zone User time zone. It should be the same as the one use
//!                      in the Google Calendar app.
//
//*****************************************************************************
Google_Calendar::Google_Calendar(const String &calendar_id, const int8_t &time_zone)
    : CALENDAR_ID(calendar_id), TIME_ZONE(time_zone)
{
    callback = nullptr; 
    event_pending = false;
}

//*****************************************************************************
//
//! @brief Subscribes the device to the Google Calendar webhook event.
//!
//! This method subscribes the device to a customized webhook response and 
//! error response event name. The device ID is included in the customized
//! event name so only THIS device will get the response.
//!
//!	@param[in] callback Pointer to the application-level response handler.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Calendar::subscribe(Event_Callback callback)
{
    this->callback = callback;
    String hook_reponse = System.deviceID() + "/hook-response/" + WEBHOOK_EVENT_NAME;
    String hook_error = System.deviceID() + "/hook-error/" + WEBHOOK_EVENT_NAME;
    Particle.subscribe(hook_reponse, &Google_Calendar::response_handler, this, MY_DEVICES);
    Particle.subscribe(hook_error, &Google_Calendar::error_handler, this, MY_DEVICES);
}

//*****************************************************************************
//
//! @brief Publishes the Google Calendar webhook event.
//!
//!	@param[in] oauth2 Google_OAuth2 object used to get the access token.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Calendar::publish(const Google_OAuth2 &oauth2)
{
    int16_t index = 0, last_index = 0;
    //  The Google Calendar API uses two params to define the time range
    //  for the event search. These params must use an RFC3339 timestamp.
    //  i.e. 2011-06-03T10:00:00-07:00, or 2011-06-03T10:00:00Z (Zulu time zone)
    //  Parameters
    //  1. timeMin: Lower bound for an event's end time to filter by.
    //  This param is set to the current data time. The Time.format() returns 
    //  an ISO8601 timestamp which is modified to match the Zulu time zone.
    String date_time = Time.format();
    String date = split_string(date_time, 'T', index, last_index);
    String time = split_string(date_time, date_time.charAt(19), index, last_index);
    String time_min = date + "T";
    time_min += time + "Z";
    //  Get the current time in seconds since Jan 01 1970 (unix timestamp).
    time_t raw_time = unix_time(Time.year(), Time.month(), Time.day(), 
                                Time.hour(), Time.minute(), Time.second());
    //  Change to UTC+0:00 since the raw time already considers the user time zone.
    Time.zone(+0);
    index = 0, last_index = 0;
    //  2. timeMax: Upper bound for an event's end time to filter by.
    //  Choose the number of hours added to the current time, three by default. 
    //  This value defines the upper bound limit for the event search.
    uint8_t hours_added = 3;
    //  Convert the hours added in seconds (multiply by 3600).
    date_time = Time.format(raw_time + (hours_added * 3600L));
    date = split_string(date_time, 'T', index, last_index);
    time = split_string(date_time, date_time.charAt(19), index, last_index);
    String time_max = date + "T";
    time_max += time + "Z";
    //  Set the user time zone back.
    Time.zone(TIME_ZONE);
    String data = String::format("{\"calendar_id\":\"%s\",\"access_token\":\"%s\",\"time_min\":\"%s\",\"time_max\":\"%s\"}",
                                 CALENDAR_ID.c_str(), oauth2.access_token.c_str(), time_min.c_str(), time_max.c_str());
    Particle.publish(WEBHOOK_EVENT_NAME, data, PRIVATE);
}

//*****************************************************************************
//
//! @brief Parses the webhook response and error response.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Calendar::parser(const char *event, const char *data)
{
    int16_t index = 0, last_index = 0;
    String str_data = String(data);
    String str_event = String(event);
    //  Get the hook type.
    //  i.e. event: deviceID/hook-response/calendar_event/0
    //  hook: hook-response.
    split_string(str_event, '/', index, last_index); // skip deviceID.
    String hook = split_string(str_event, '/', index, last_index);
    //  Reset the split string indexes.
    index = 0, last_index = 0;
    //  For "hook-response", the returned data is divided by '~' and stays
    //  the same as there is only one webhook event. If no events were found
    //  within the given time range, then a '~' character is returned.
    if (hook.equals("hook-response"))
    {
        if (str_data.equals("~"))
        {
            event_pending = false;
        }
        else
        {
            event_date_time = split_string(str_data, '~', index, last_index);;
            event_location = split_string(str_data, '\0', index, last_index);
            event_pending = true;
        }
        http_status_code = HTTP_OK;
    }
    //  For "hook-error", the returned data is an error message generated by   
    //  the Particle Cloud. From this message only the HTTP status code is taken.
    //  i.e. error status 404 from www.googleapis.com
    //  HTTP status code: 404.
    else if (hook.equals("hook-error"))
    {
        http_status_code = str_data.substring(13, 16).toInt();
    }
}

//*****************************************************************************
//
//! @brief Google Calendar webhook response handler.
//!
//! This method is called by the OS whenever the HTTP status code received 
//! in the response is EQUAL TO 200.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//
//*****************************************************************************
void Google_Calendar::response_handler(const char *event, const char *data)
{
    //  Parse the webhook reponse.
    parser(event, data);
    //  Invoke the user subscribed response handler.
    (*callback)();
}

//*****************************************************************************
//
//! @brief Google Calendar webhook error response handler.
//!
//! This method is called by the OS whenever the HTTP status code received 
//! in the response is DIFFERENT THAN 200.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//
//*****************************************************************************
void Google_Calendar::error_handler(const char *event, const char *data)
{
    //  Parse the webhook error reponse.
    parser(event, data);
    //  A string object is built with the HTTP status code 
    //  and an error message to infrom the user.
    http_error = String::format("\r\nHTTP ERROR - %d", http_status_code);
    if (http_status_code == HTTP_BAD_REQUEST)
    {
        http_error += "\r\nError: The requested ordering is not available for the particular query.";
    }
    else if (http_status_code == HTTP_UNAUTHORIZED)
    {
        http_error += "\r\nError: Invalid credentials.";
    }
    else if (http_status_code == HTTP_NOT_FOUND)
    {
        http_error += "\r\nError: Invalid calendar id.";
    }
    //  Invoke the user subscribed response handler.
    (*callback)();
}

//*****************************************************************************
//
//! @brief Checks if the Google Calendar API failed.
//!
//! @return false if did not fail, true if failed.
//
//*****************************************************************************
bool Google_Calendar::failed(void)
{
    return http_status_code != HTTP_OK;
}

//*****************************************************************************
//
//! @brief Prints the HTTP error response returned by the last event published.
//!
//! @return None.
//
//*****************************************************************************
void Google_Calendar::print_error(void)
{
    Serial.println(http_error);
}

//*****************************************************************************
//
//! @brief Returns the calendar event status.
//!
//! @return false if not pending, true if pending.
//
//*****************************************************************************
bool Google_Calendar::is_event_pending(void)
{
    return event_pending;
}

//*****************************************************************************
//
//! @brief Gets the event location from the last request.
//!
//! @return A string object with the event location using the Google Geocoding 
//!         address format (i.e. 1600 Amphitheatre Parkway, Mountain View, CA).
//
//*****************************************************************************
String Google_Calendar::get_event_location(void)
{
    return event_location;
}

//*****************************************************************************
//
//! @brief Gets the event start date and time from the last request.
//!
//! @return A string object with the event start date and time using the 
//!         RFC3339 format (i.e. 2011-06-03T10:00:00-07:00).
//
//*****************************************************************************
String Google_Calendar::get_event_date_time(void)
{
    return event_date_time;
}
