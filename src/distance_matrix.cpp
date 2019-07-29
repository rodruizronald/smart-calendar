#include "Particle.h"
#include "distance_matrix.h"
#include "utility.h"
#include "http_status.h"

//*****************************************************************************
//
//! @brief Google Geolocation class constructor.
//
//*****************************************************************************
Google_Distance_Matrix::Google_Distance_Matrix()
{
    callback = nullptr;
}

//*****************************************************************************
//
//! @brief Subscribes the device to a Google Distance Matrix webhook event.
//!
//! This method subscribes the device to a customized webhook response and 
//! error response event name. The device ID is included in the customized
//! event name so only THIS device will get the response.
//!
//!	@param[in] event Distance Matrix event used to setup the webhook.
//!	@param[in] callback Pointer to the application-level response handler.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Distance_Matrix::subscribe(const Distance_Matrix_Event &event, Event_Callback callback)
{
    this->callback = callback;
    //  Select the webhook event name to subscribe  
    //  the device to depending on the travel mode.
    if (event.travel_mode == Distance_Matrix_Travel_Mode::DRIVING)
    {
        WEBHOOK_EVENT_NAME = WEBHOOK_DISTANCE_DRIVING;
    }  
    else if (event.travel_mode == Distance_Matrix_Travel_Mode::TRANSIT)
    {
        WEBHOOK_EVENT_NAME = WEBHOOK_DISTANCE_TRANSIT;
    }
    String hook_reponse = System.deviceID() + "/hook-response/" + WEBHOOK_EVENT_NAME;
    Particle.subscribe(hook_reponse, &Google_Distance_Matrix::response_handler, this, MY_DEVICES);
}

//*****************************************************************************
//
//! @brief Publishes a Google Distance Matrix webhook event.
//!
//!	@param[in] event Distance Matrix event used to setup the webhook.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Distance_Matrix::publish(const Distance_Matrix_Event &event)
{
    //  Build an string object with the latitude/longitude coordinates.
    String origin = String::format("%.6f,%.6f", event.origin_lat, event.origin_lng);
    //  Build the webhook query according to 
    //  the travel mode specified by the user. 
    String data;
    if (event.travel_mode == Distance_Matrix_Travel_Mode::DRIVING)
    {
        //  Get the current time in seconds since Jan 01 1970 (unix timestamp).
        time_t curr_time = unix_time(Time.year(), Time.month(), Time.day(),
                                     Time.hour(), Time.minute(), Time.second());
        data = String::format("{\"origin\":\"%s\",\"destination\":\"%s\",\"curr_time\":\"%s\"}",
                              origin.c_str(), event.destination.c_str(), String(curr_time).c_str());
    }
    else if (event.travel_mode == Distance_Matrix_Travel_Mode::TRANSIT)
    {
        //  Select the transit mode specified by the user.
        String transit_mode;
        switch (event.transit_mode)
        {
            case Distance_Matrix_Transit_Mode::BUS:
                transit_mode = "bus";
                break;
            case Distance_Matrix_Transit_Mode::SUBWAY:
                transit_mode = "subway";
                break;
            case Distance_Matrix_Transit_Mode::TRAIN:
                transit_mode = "train";
                break;
            case Distance_Matrix_Transit_Mode::TRAM:
                transit_mode = "tram";
                break;
            case Distance_Matrix_Transit_Mode::RAIL:
                transit_mode = "rail";
                break;
            default:
                transit_mode = "bus";
                break;
        }
        data = String::format("{\"origin\":\"%s\",\"destination\":\"%s\",\"transit_mode\":\"%s\"}",
                              origin.c_str(), event.destination.c_str(), transit_mode.c_str());
    }
    Particle.publish(WEBHOOK_EVENT_NAME, data, PRIVATE);
}

//*****************************************************************************
//
//! @brief Parses the webhook response.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Distance_Matrix::parser(const char *event, const char *data)
{
    int16_t index = 0, last_index = 0;
    String str_data = String(data);
    //  The returned data is divided by '~' and does not change as both Distance
    //  Matrix webhooks (dist_driving/dist_transit) request the same data.
    distance_to_dest = split_string(str_data, '~', index, last_index).toInt();
    duration_to_dest = split_string(str_data, '~', index, last_index).toInt();
    String element_status = split_string(str_data, '~', index, last_index);
    String top_status = split_string(str_data, '\0', index, last_index);
    //  The Distance Martix API returns an HTTP 200 status code even if 
    //  something goes wrong with the last request. Errors are handle by  
    //  an element- and top-level status code. This is why no error handler 
    //  was implemented in this class.
    //  1. Top-level status code: Contains information about the request 
    //     in general.
    //  2. Element-level status code: Contains information about an element 
    //     particular origin-destination pairing. In this application there
    //     is only one element per request.
    //  Error description is provided in the API source link.
    if (top_status.equals("OK"))
    {
        if (element_status.equals("OK"))
        {
            http_status_code = HTTP_OK;
        }
        else
        {
            //  An HTTP error is forced.
            http_status_code = HTTP_BAD_REQUEST;
            //  Specify level error.
            http_error = "\r\nError: Element-level error, ";
            http_error += element_status;
        }
    }
    else
    {
        //  An HTTP error is forced.
        http_status_code = HTTP_BAD_REQUEST;
        //  Specify level error.
        http_error = "\r\nError: Top-level error, ";
        http_error += top_status;
    }
}

//*****************************************************************************
//
//! @brief Google Distance Matrix webhook response handler.
//!
//! This method is called by the OS whenever the HTTP status code received 
//! in the response is EQUAL TO 200.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//
//*****************************************************************************
void Google_Distance_Matrix::response_handler(const char *event, const char *data)
{
    //  Parse the webhook reponse.
    parser(event, data);
    //  Invoke the user subscribed response handler.
    (*callback)();
}

//*****************************************************************************
//
//! @brief Checks if the Google Distance Matrix API failed.
//!
//! @return false if did not fail, true if failed.
//
//*****************************************************************************
bool Google_Distance_Matrix::failed(void)
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
void Google_Distance_Matrix::print_error(void)
{
    Serial.println(http_error);
}

//*****************************************************************************
//
//! @brief Gets the travel duration, in seconds.
//!
//! @return An unsigned 32-bit number.
//
//*****************************************************************************
uint32_t Google_Distance_Matrix::get_duration_to_dest(void)
{
    return duration_to_dest;
}

//*****************************************************************************
//
//! @brief Gets the travel distance, in miles.
//!
//! @return An unsigned 16-bit number.
//
//*****************************************************************************
uint16_t Google_Distance_Matrix::get_distance_to_dest(void)
{
    return distance_to_dest;
}
