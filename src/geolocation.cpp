#include "Particle.h"
#include "geolocation.h"
#include "utility.h"
#include "http_status.h"

//  Size of a JSON WiFi access point object in bytes.
#define WIFI_AP_SIZE            46
//  Max. number of WiFi access points allowed to store. 
#define MAX_NUM_APS             6
//  Size of x number of WiFi access points.
#define WIFI_AP_BUFF_SIZE       ((WIFI_AP_SIZE * MAX_NUM_APS) + 1)

//  Pointer to the access point memory location.
static char *wifi_ap_ptr;
//  Total number of bytes available in the buffer.
static size_t buff_size;
//  Current number of access points registered.
static uint8_t wifi_ap_cnt;
//  Space in memory to store the access points.
static char wifi_ap_buff[WIFI_AP_BUFF_SIZE];

//*****************************************************************************
//
//! @brief WiFi scan callback function.
//!
//! This function is called by the OS once for each scanned access point. The 
//! number of access points available is unknown by the user prior the 
//! execution of this callback. To increase the number of potential access 
//! points, it is recommended to attach an WiFi antenna to the Particle Argon.
//!
//!	@param[in] wap Pointer to a WiFi access point struct.
//!	@param[in] data Pointer to an optional second param (not used).
//
//*****************************************************************************
static void wifi_scan_callback(WiFiAccessPoint *wap, void *data)
{
    //  Only 6 access points are captured by default. It was considered
    //  enough to get an accuarte location from the Geolocation API.
    if (wifi_ap_cnt < MAX_NUM_APS)
    {
        WiFiAccessPoint &ap = *wap;
        //  Build a string object with the 
        //  6-byte MAC address of an access point.
        String bssid = String::format("%02X:%02X:%02X:%02X:%02X:%02X",
                                      ap.bssid[0], ap.bssid[1], ap.bssid[2], 
                                      ap.bssid[3], ap.bssid[4], ap.bssid[5]);
        //  Form a JSON WiFi access point object.
        //  m: MAC address.
        //  s: signal strength.
        //  c: channel.
        //  i.e. {\"m\":\"00:25:9c:cf:1c:ac\",\"s\":-79,\"c\":11},
        String wap_obj = String::format("{\"m\":\"%s\",\"s\":\"%d\",\"c\":\"%d\"},",
                                       bssid.c_str(), ap.rssi, ap.channel);
        //  Write the JSON object in the buffer and check for encoding errors.
        int wap_size = snprintf(wifi_ap_ptr, buff_size, "%s", wap_obj.c_str());
        if (wap_size < 0) return;
        //  Update the buffer size, pointer location,  
        //  and number of current access points recorded.
        buff_size -= wap_size;
        wifi_ap_ptr += wap_size;
        wifi_ap_cnt++;
    }
}

//*****************************************************************************
//
//! @brief Google Geolocation class constructor.
//
//*****************************************************************************
Google_Geolocation::Google_Geolocation()
{
    callback = nullptr;
}

//*****************************************************************************
//
//! @brief Subscribes the device to the Google Geolocation webhook event.
//!
//! This method subscribes the device to a customized webhook response and 
//! error response event name. The device ID is included in the customized
//! event name so only THIS device will get the response.
//!
//!	@param[in] callback Pointer to the user reponse handler function.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Geolocation::subscribe(Event_Callback callback)
{
    this->callback = callback;
    String hook_reponse = System.deviceID() + "/hook-response/" + WEBHOOK_EVENT_NAME;
    String hook_error = System.deviceID() + "/hook-error/" + WEBHOOK_EVENT_NAME;
    Particle.subscribe(hook_reponse, &Google_Geolocation::response_handler, this, MY_DEVICES);
    Particle.subscribe(hook_error, &Google_Geolocation::error_handler, this, MY_DEVICES);
}

//*****************************************************************************
//
//! @brief Publishes the Google Geolocation webhook event.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Geolocation::publish(void)
{
    scan_access_points();
    //  Build the webhook query with the data obtained 
    //  from the scan function and pusblish the event.
    String data = String::format("{\"a\":[%s}", wifi_ap_buff);
    Particle.publish(WEBHOOK_EVENT_NAME, data, PRIVATE);
}

//*****************************************************************************
//
//! @brief Scans nearby WiFi access points.
//!
//! This method uses the Argon WiFi API to get information about access points
//! within range of the device. It implements a callback function that receives 
//! each scanned access point.
//!
//!	@return None.
//
//*****************************************************************************
void Google_Geolocation::scan_access_points(void)
{
    buff_size = WIFI_AP_BUFF_SIZE;
    wifi_ap_cnt = 0;
    wifi_ap_ptr = wifi_ap_buff;
    //  Zero out the buffer.
    memset(wifi_ap_buff, 0, buff_size);
    //  Scan for available access points.
    //  After calling the WiFi.scan() function, the
    //  OS holds the application until all available 
    //  access points have been processed.
    WiFi.scan(wifi_scan_callback);
    //  Remove comma from last object and close the array of JSON objects.
    *(--wifi_ap_ptr) = ']';
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
void Google_Geolocation::parser(const char *event, const char *data)
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
    //  the same as there is only one webhook event.
    if (hook.equals("hook-response"))
    {
        latitud = split_string(str_data, '~', index, last_index).toFloat();
        longitud = split_string(str_data, '~', index, last_index).toFloat();
        accuracy = split_string(str_data, '\0', index, last_index).toInt();
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
//! @brief Google Geolocation webhook response handler.
//!
//! This method is called by the OS whenever the HTTP status code received 
//! in the response is EQUAL TO 200.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//
//*****************************************************************************
void Google_Geolocation::response_handler(const char *event, const char *data)
{
    //  Parse the webhook reponse.
    parser(event, data);
    //  Invoke the user subscribed response handler.
    (*callback)();
}

//*****************************************************************************
//
//! @brief Google Geolocation webhook error response handler.
//!
//! This method is called by the OS whenever the HTTP status code received 
//! in the response is DIFFERENT THAN 200.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse.
//
//*****************************************************************************
void Google_Geolocation::error_handler(const char *event, const char *data)
{
    //  Parse the webhook error reponse.
    parser(event, data);
    //  A string object is built with the HTTP status code 
    //  and an error message to infrom the user.
    http_error = String::format("\r\nHTTP ERROR - %d", http_status_code);
    if (http_status_code == HTTP_BAD_REQUEST)
    {
        http_error += "\r\nError: Invalid API key or request body.";
    }
    else if (http_status_code == HTTP_FORBIDDEN)
    {
        http_error += "\r\nError: User rate limit exceeded, or API key has restricted access.";
    }
    else if (http_status_code == HTTP_NOT_FOUND)
    {
        http_error += "\r\nError: The request was valid, but no results were returned.";
    }
    //  Invoke the user subscribed response handler.
    (*callback)();
}

//*****************************************************************************
//
//! @brief Checks if the Google Geolocation API failed.
//!
//! @return false if did not fail, true if failed.
//
//*****************************************************************************
bool Google_Geolocation::failed(void)
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
void Google_Geolocation::print_error(void)
{
    Serial.println(http_error);
}

//*****************************************************************************
//
//! @brief Gets the accuracy of the estimated location, in meters.
//!
//! @return A unsigned 16-bit number.
//
//*****************************************************************************
uint16_t Google_Geolocation::get_accuracy(void)
{
    return accuracy;
}

//*****************************************************************************
//
//! @brief Gets the latitude coordinate.
//!
//! @return A floating point number.
//
//*****************************************************************************
float Google_Geolocation::get_lat(void)
{
    return latitud;
}

//*****************************************************************************
//
//! @brief Gets the longitude coordinate.
//!
//! @return A floating point number.
//
//*****************************************************************************
float Google_Geolocation::get_lng(void)
{
    return longitud;
}
