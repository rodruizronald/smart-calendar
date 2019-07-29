#ifndef __APP_H__
#define __APP_H__

//*****************************************************************************
//
//	The following are defines for the Geolocation API.
//
//*****************************************************************************

//  Comment this line to disable Geolocation.
//  If disabled, you must specify your location
//  in the setup() function.
#define GEOLOC_ENABLED

//  Minimum accuracy allowed by the app in meters.
//  If the accuracy retured by the API is less than this,
//  then an error will occur.
#define GEOLOC_MINIMUM_ACC      50  

//*****************************************************************************
//
//	The following are enumeration classes for the DFPlayer Mini.
//
//*****************************************************************************

//  MP3 folders from 01 to 03.
//  Folder order must be the same in the SD card.
enum class MP3_Folder : uint8_t
{
    STATUS_INFO = 1,
    HOURS,
    MINUTES
};

//  MP3 files for folder 01 (STATUS_INFO) from 001 to 010.
//  File order must be the same in the SD card.
enum class MP3_File : uint8_t
{
    // Hi, your device is being updated, please wait.
    UPDATE_DEVICE = 1,
    // Your device is ready. You might now ask Google for your next activity.
    DEVICE_READY,
    // Hi, your device has not been authenticated yet. 
    // Please open a terminal and follow the steps indicated.
    OPEN_TERMINAL,
    // Your request has been received, I am now locating your next event.
    REQ_RECEIVED,
    // Location and time found, please wait while I estimate the 
    // ideal departure time. 
    ESTIMATING_DT,
    // There are no events scheduled on your calendar for the next three hours.
    NO_EVENTS,
    // An error has occurred. Please open a terminal to see what caused 
    // the error and fix it before rebooting your device.
    APP_FAILED,
    // Based on your current location, you would be on time for your 
    // upcoming event by leaving in
    TIME_LEFT,
    // The results showed that you have to leave now to be on time 
    // for your upcoming event.
    NO_TIME_LEFT,
    // Based on your current location, if you leave now, you would be late for
    // your upcoming event by 
    LATE,
};

//*****************************************************************************
//
//	The following are enumeration classes for the application stages and 
//  event states.
//
//*****************************************************************************

enum class App_Stage : uint8_t
{
    GEOLOCATION,
    OAUTH2,
    CALENDAR,
    DISTANCE_MATRIX,
    DATA_PROCESSING,
    ASSISTANT,
    FAILED
};
App_Stage app_stage;
App_Stage last_app_stage;

enum class Event_State : uint8_t
{
    PUBLISHING,
    WAIT_FOR_RESPONSE,
    COMPLETED
};
Event_State event_state;

//*****************************************************************************
//
//	The following are global definitios to configure your application.
//
//*****************************************************************************

//  Digital pin assigned to read the MP3 player state.
const uint8_t MP3_BUSY_PIN = 2; 
//  Set your time zone here. You MUST consider Daylight saving time (DST).
const int8_t TIME_ZONE = +1; 
const String CLIENT_SECRET = "<TYPE_YOUR_CLIENT_SECRET_HERE>";
const String CALENDAR_ID = "<TYPE_YOUR_CALENDAR_ID_HERE>";
const String CLIENT_ID = "<TYPE_YOUR_CLIENT_ID_HERE>";

//*****************************************************************************
//
//	The following are global objects for the DFPlayer and Google classes.
//
//*****************************************************************************

DFPlayer_MP3 MP3(Serial1, MP3_BUSY_PIN);
Google_Calendar Calendar(CALENDAR_ID, TIME_ZONE);
Google_OAuth2 OAuth2(CLIENT_ID, CLIENT_SECRET);
Google_Geolocation Geolocation;
Google_Distance_Matrix Distance_Matrix;
Google_Distance_Matrix::Distance_Matrix_Event Distance_Matrix_Event;

//*****************************************************************************
//
//  Prototypes for the application functions.
//
//*****************************************************************************

void oauth2_loop(void);
void calendar_loop(void);
void calendar_handler(void);
void geolocation_loop(void);
void geolocation_handler(void);
void distance_matrix_loop(void);
void distance_matrix_handler(void);
void calc_departure_time(void);
void assistant_handler(const char *event, const char *data);
void init_mp3_player(void);
void play_status_info(MP3_File mp3_file);
void play_time(MP3_Folder mp3_folder, uint8_t mp3_file);
void print_app_error(void);
void print_event_state(void);
void change_app_stage_to(App_Stage new_stage);

#endif // __APP_H__