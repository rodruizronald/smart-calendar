//*****************************************************************************
//
//  The following are header files for the application.
//
//*****************************************************************************
#include "mp3.h"
#include "oauth2.h"
#include "calendar.h"
#include "geolocation.h"
#include "distance_matrix.h"
#include "utility.h"
#include "app.h"

void setup()
{
    Serial.begin();
    Time.zone(TIME_ZONE);
    Time.setFormat(TIME_FORMAT_ISO8601_FULL);
    init_mp3_player();
    //  Play a different MP3 file depending on 
    //  the current OAuth2.0 state. 
    if (OAuth2.authenticated()) 
    {   
        play_status_info(MP3_File::UPDATE_DEVICE);
    }
    else
    {
        play_status_info(MP3_File::OPEN_TERMINAL);
        while (Serial.read() == 0)
        {
            Serial.println("Press enter to continue...");
            delay(1000);
        }
    }
    //  Configure the distance matrix event.
    //  Travel Mode: DRIVING, TRANSIT.
    Distance_Matrix_Event.travel_mode = Distance_Matrix_Travel_Mode::TRANSIT;
    //  Transit Mode: SUBWAY, TRAIN, TRAM, RAIL, NONE.
    Distance_Matrix_Event.transit_mode = Distance_Matrix_Transit_Mode::BUS;

#ifdef GEOLOC_ENABLED
    change_app_stage_to(App_Stage::GEOLOCATION);
#else
    //  Set the device location.
    Distance_Matrix_Event.origin_lat = 0.0;
    Distance_Matrix_Event.origin_lng = 0.0;
    change_app_stage_to(App_Stage::OAUTH2);
#endif
}

void loop()
{
    switch (app_stage)
    {
        case App_Stage::GEOLOCATION:
            geolocation_loop();
            break;

        case App_Stage::OAUTH2:
            oauth2_loop();
            break;

        case App_Stage::CALENDAR:
            calendar_loop();
            break;

        case App_Stage::DISTANCE_MATRIX:
            distance_matrix_loop();
            break;

        case App_Stage::ASSISTANT:
            Serial.println("waiting for a user request...");
            delay(1000);
            break;

        case App_Stage::DATA_PROCESSING:
            calc_departure_time();
            break;

        case App_Stage::FAILED:
            print_app_error();
            break;

        default:
            break;
    }
}

//*****************************************************************************
//  @section Data Processing.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Caculates the ideal depature time.
//!
//! This function uses all the data collected from the Google APIs to calcualte
//! the ideal depature time for the user upcoming event/activity/meeting.
//!
//! @return None. 
//
//*****************************************************************************
void calc_departure_time(void)
{
    int16_t index = 0, last_index = 0;
    //  The ideal depature time is calculated as the remaning time that user
    //  has before leaving to get in time to its next event.
    //  For this, the depature time is assumed as the current time
    //  and therefore, the arrival time is the depature time plus the
    //  travel time returned by the Distance Matrix API.
    //  Both times are unix timestamps (seconds since Jan 01 1970).
    time_t departure_time = unix_time(Time.year(), Time.month(), Time.day(),
                                      Time.hour(), Time.minute(), Time.second());
    time_t arrival_time = departure_time + Distance_Matrix.get_duration_to_dest();
    //  The event start date and time is a string RFC3339 timestamp.
    //  i.e. 2011-06-03T10:00:00-07:00.
    //  It is splited and converted into a unix timestamp.
    String event_date_time = Calendar.get_event_date_time();
    int year = split_string(event_date_time, '-', index, last_index).toInt();
    int month = split_string(event_date_time, '-', index, last_index).toInt();
    int day = split_string(event_date_time, 'T', index, last_index).toInt();
    int hour = split_string(event_date_time, ':', index, last_index).toInt();
    int min = split_string(event_date_time, ':', index, last_index).toInt();
    int sec = split_string(event_date_time, event_date_time.charAt(19), index, last_index).toInt();
    time_t event_start_time = unix_time(year, month, day, hour, min, sec);
    //  Change to UTC+0:00 since the unix timestamp already considers the user time zone.
    Time.zone(+0);
    Serial.print("\r\nIf the departure time is (current time): ");
    Serial.println(Time.format(departure_time));
    Serial.print("Then the estimated arrival time would be: ");
    Serial.println(Time.format(arrival_time));
    //  Set the user time zone back.
    Time.zone(TIME_ZONE);
    //  Calcualte the time left before the event start in seconds.
    int32_t time_left = event_start_time - arrival_time;
    //  If positive, user is still on time. Otherwise, it is late.
    bool on_time = (time_left >= 0);
    time_left = abs(time_left);
    //  Get the hours and minutes in time left.
    uint8_t hours = time_left / 3600;
    time_left -= (hours * 3600);
    uint8_t minutes = time_left / 60;
    //  At least, one minute to avoid seconds.
    if (minutes == 0)
    {
        minutes = 1;
    }

    Serial.print("Based on these times, ");
    //  Inform the user about the result.
    //  The play_time() plays time files.
    //  The naming of these time files match the text context. 
    //  So the hours/minutes are used directly to play them.
    //  i.e file name: "002".
    //      MP3 audio: "two minutes".
    if (on_time)
    {
        if (time_left == 0)
        {
            Serial.println("you have to leave now to be one time.");
            play_status_info(MP3_File::NO_TIME_LEFT);
        }
        else
        {
            Serial.println("you still have time left before depature.");
            play_status_info(MP3_File::TIME_LEFT);
            if (hours > 0)
            {
                Serial.print("Hours left: ");
                Serial.println(hours);
                play_time(MP3_Folder::HOURS, hours);
            }
            Serial.print("Minutes left: ");
            Serial.println(minutes);
            play_time(MP3_Folder::MINUTES, minutes);
        }
    }
    else
    {
        Serial.println("you are already late.");
        play_status_info(MP3_File::LATE);
        if (hours > 0)
        {
            Serial.print("Hours late: ");
            Serial.println(hours);
            play_time(MP3_Folder::HOURS, hours);
        }
        Serial.print("Minutes late: ");
        Serial.println(minutes);
        play_time(MP3_Folder::MINUTES, minutes);
    }

    Serial.print("\r\n");
    //  Go back to Assitant mode and wait for a new request.
    change_app_stage_to(App_Stage::ASSISTANT);
}

//*****************************************************************************
//  @section Google Geolocation API.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Geolocation main function.
//!
//! @return None. 
//
//*****************************************************************************
void geolocation_loop(void)
{
    //  Publish the event and wait until the
    //  application-level response handler is called.
    if (event_state == Event_State::PUBLISHING)
    {
        Geolocation.publish();
    }
    print_event_state();
}

//*****************************************************************************
//
//! @brief Geolocation application-level response handler. 
//
//*****************************************************************************
void geolocation_handler(void)
{
    if (!Geolocation.failed())
    {
        //  The application fails if the estimated location is not
        //  accurate enough. This error can be overlook by incresing
        //  GEOLOC_MINIMUM_ACC, although it is no recommended as it
        //  will affect the precision of the travel distance and time
        //  returned by the Distance Matrix API.
        //  It is better to set the device location manually in the
        //  setup() function.
        if (Geolocation.get_accuracy() < GEOLOC_MINIMUM_ACC)
        {
            Serial.println("\r\nYour device has been located.");
            Serial.print("Latitud: ");
            Serial.println(Geolocation.get_lat());
            Serial.print("Longitud: ");
            Serial.println(Geolocation.get_lng());
            Serial.print("Accuracy radius: ");
            Serial.print(Geolocation.get_accuracy());
            Serial.println(" meters.\r\n");
            //  Device location is set automatically with Geolocation.
            Distance_Matrix_Event.origin_lat = Geolocation.get_lat();
            Distance_Matrix_Event.origin_lng = Geolocation.get_lng();
            //  Change to OAuth2.0 to refresh access token or request
            //  the user code, depending on the current state.
            change_app_stage_to(App_Stage::OAUTH2);
        }
        else
        {
            change_app_stage_to(App_Stage::FAILED);
        } 
    }
    else
    {
        change_app_stage_to(App_Stage::FAILED);
    }
}

//*****************************************************************************
//  @section Google OAuth2.0 protocol.
//*****************************************************************************
//*****************************************************************************
//
//! @brief OAuth2.0 main function.
//!
//! @return None. 
//
//*****************************************************************************
void oauth2_loop(void)
{
    //  Execute the OAuth2.0 authorization algorithm.
    //  OAuth2.loop() should run freely without delay
    //  until the application has been authorized.
    OAuth2.loop();
    if (OAuth2.authorized())
    {
        //  In case that the access token had to be refreshed and the Calendar 
        //  stage were interrupted, then go back and finish the user request. 
        //  Otherwise, switch to Assitant mode and wait for a new request.
        if (last_app_stage == App_Stage::CALENDAR)
        {
            change_app_stage_to(App_Stage::CALENDAR);
        }
        else
        {
            change_app_stage_to(App_Stage::ASSISTANT);
            //  Only during initialization, inform the user device is ready.    
            play_status_info(MP3_File::DEVICE_READY);
        }
    }
    else if (OAuth2.failed())
    {
        change_app_stage_to(App_Stage::FAILED);
    }
}

//*****************************************************************************
//  @section Google Calendar API.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Calendar main function.
//!
//! @return None. 
//
//*****************************************************************************
void calendar_loop(void)
{
    //  Publish the event and wait until the
    //  application-level response handler is called.
    if (event_state == Event_State::PUBLISHING)
    {
        //  If the access token has not expired yet, use the API.
        //  Otherwise, change stage to OAuth2.0 to refresh token.
        if (OAuth2.is_token_valid())
        {
            //  OAuth2 is passed to get the access token.
            Calendar.publish(OAuth2);
        }
        else
        {
            change_app_stage_to(App_Stage::OAUTH2);
        }
    }
    print_event_state();
}

//*****************************************************************************
//
//! @brief Calendar application-level response handler.
//
//*****************************************************************************
void calendar_handler(void)
{
    if (!Calendar.failed())
    {
        //  Check if the webhook returned any user event/activity/meeting... 
        if (Calendar.is_event_pending())
        {
            Serial.println("\r\nThere is an event scheduled on your Google Calendar.");
            Serial.print("Location: ");
            Serial.println(Calendar.get_event_location());
            Serial.print("Date and time: ");
            Serial.println(Calendar.get_event_date_time() + "\r\n");
            //  Set the destination (event location).
            Distance_Matrix_Event.destination = Calendar.get_event_location();
            //  Change to Distance Matrix mode to request 
            //  the travel distance and time to the event location.
            change_app_stage_to(App_Stage::DISTANCE_MATRIX);
            play_status_info(MP3_File::ESTIMATING_DT);
        }
        else
        {
            Serial.println("\r\nNo pending events!\r\n");
            //  Go back to Assitant mode and wait for a new user request.
            change_app_stage_to(App_Stage::ASSISTANT);
            play_status_info(MP3_File::NO_EVENTS);
        }
    }
    else
    {
        change_app_stage_to(App_Stage::FAILED);
    }
}

//*****************************************************************************
//  @section Google Distance Matrix API.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Distance Matrix main function.
//!
//! @return None. 
//
//*****************************************************************************
void distance_matrix_loop(void)
{
    //  Publish the event and wait until the
    //  application-level response handler is called.
    if (event_state == Event_State::PUBLISHING)
    {
        Distance_Matrix.publish(Distance_Matrix_Event);
    }
    print_event_state();
}

//*****************************************************************************
//
//! @brief Distance Matrix application-level response handler.
//
//*****************************************************************************
void distance_matrix_handler(void)
{
    if (!Distance_Matrix.failed())
    {
        Serial.println("\r\nAccording to the Distance Matrix API, to get to your next event");
        Serial.print("Travel distance is: ");
        Serial.print(Distance_Matrix.get_distance_to_dest());
        Serial.println(" miles");
        Serial.print("Travel duration is: ");
        Serial.print(Distance_Matrix.get_duration_to_dest());
        Serial.println(" sec");
        //  Change stage to Data Processing to answer the user request.
        change_app_stage_to(App_Stage::DATA_PROCESSING);
    }
    else
    {
        change_app_stage_to(App_Stage::FAILED);
    }
}

//*****************************************************************************
//  @section Google Assistance (IFTTT event).
//*****************************************************************************
//*****************************************************************************
//
//! @brief Google Assistance webhook response handler.
//!
//! This handler is called by the OS the whenever the user says the Google
//! Assistance phrased configured in the IFTTT event.
//!
//!	@param[in] event Pointer to a char array holidng the webhook event info.
//!	@param[in] data Pointer to a char array holding the webhook reponse. 
//
//*****************************************************************************
void assistant_handler(const char *event, const char *data)
{
    Serial.println("\r\nAssistant event published!\r\n");
    //  Change stage to Calendar to process the user request.
    change_app_stage_to(App_Stage::CALENDAR);
    play_status_info(MP3_File::REQ_RECEIVED);
}

//*****************************************************************************
//  @section DFPlayer Mini application fucntions.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Plays an MP3 file from the STATUS_INFO folder.
//!
//!	@param[in] mp3_file MP3 file to be played by the DFPlayer Mini.
//!
//! @return None. 
//
//*****************************************************************************
void play_status_info(MP3_File mp3_file)
{
    //  Convert folder and file to an unsigned 8-bit number.
    uint8_t folder = enum_to_uint8(MP3_Folder::STATUS_INFO);
    uint8_t file = enum_to_uint8(mp3_file);
    MP3.play_folder(folder, file);
    //  Hold the program until the last play has finished.
    while (!MP3.free()) {}
}

//*****************************************************************************
//
//! @brief Plays an MP3 file from the HOURS/MINUTES folder.
//!
//!	@param[in] mp3_folder Folder where the MP3 file is located.
//!	@param[in] mp3_file MP3 file to be played by the DFPlayer Mini.
//!
//! @return None. 
//
//*****************************************************************************
void play_time(MP3_Folder mp3_folder, uint8_t mp3_file)
{
    //  Convert folder to an unsigned 8-bit number.
    uint8_t folder = enum_to_uint8(mp3_folder);
    MP3.play_folder(folder, mp3_file);
    //  Hold the program until the last play has finished.
    while (!MP3.free()) {}
}

//*****************************************************************************
//
//! @brief Initilizes the MP3 player and internal interfaces (UART1).
//!
//! @return None. 
//
//*****************************************************************************
void init_mp3_player(void)
{
    //  Initialize the UART1 to 9600 bps.
    Serial1.begin(9600);
    //  Initialize the DFPlayer Mini. 
    //  It checks communication and SD card status. 
    if (!MP3.begin())
    {
        while (1)
        {
            Serial.println("\r\nError: MP3 palyer unable to begin.");
            Serial.println("Please check your connections and make sure that the micro SD card is inserted!");
            delay(1000);
        }
    }
    else
    {
        Serial.println("\r\nMP3 player online!\r\n");
        //  Set MP3 volume at 20 (from 0-30)
        MP3.volume(20);
    }
}

//*****************************************************************************
//  @section General application functions.
//*****************************************************************************
//*****************************************************************************
//
//! @brief Changes the current stage of the application and subscribes the 
//!        webhook reponse handlers, if necessary.
//!
//!	@param[in] new_stage Stage at which the application is set.
//!
//! @return None. 
//
//*****************************************************************************
void change_app_stage_to(App_Stage new_stage)
{
    //  Save the previous application stage in case of a failure.
    last_app_stage = app_stage;
    app_stage = new_stage;
    //  If the application stage changes, it is assumed 
    //  that the previous event has been completed.
    event_state = Event_State::COMPLETED;     
    //  Unsubsribe all passed events to make space for new ones.
    Particle.unsubscribe();
    //  Event handlers have to be subscribed multiple times since
    //  they are removed between intermediate stages. This is
    //  because the Argon OS does not support more than four
    //  handlers subscribed at the same time.
    //  IMPORTANT: OS handlers are the ones subscribed with the
    //  Particle.subscribe() function which are called within 
    //  the Google classes.
    if (new_stage == App_Stage::ASSISTANT)
    {
        Particle.subscribe("google_assistant", assistant_handler, MY_DEVICES);
    }
    else if (new_stage == App_Stage::CALENDAR)
    {
        Calendar.subscribe(calendar_handler);
    }
    else if (new_stage == App_Stage::GEOLOCATION)
    {
        Geolocation.subscribe(geolocation_handler);
    }
    else if (new_stage == App_Stage::DISTANCE_MATRIX)
    {
        Distance_Matrix.subscribe(Distance_Matrix_Event, distance_matrix_handler);
    }
    else if (new_stage == App_Stage::FAILED)
    {
        //  In case of failure, inform the user.
        play_status_info(MP3_File::APP_FAILED);
    }
}

//*****************************************************************************
//
//! @brief Prints general information about the current state and updates it, 
//!        if necessary.
//!
//! @return None. 
//
//*****************************************************************************
void print_event_state(void)
{
    if (event_state == Event_State::PUBLISHING)
    {
        switch (app_stage)
        {
        case App_Stage::GEOLOCATION:
            Serial.println("\r\nGeolocation event published!");
            break;

        case App_Stage::CALENDAR:
            Serial.println("Calendar event published!");
            break;

        case App_Stage::DISTANCE_MATRIX:
            Serial.println("Distance matrix event published!");
            break;

        default:
            break;
        }
        event_state = Event_State::WAIT_FOR_RESPONSE;
    }
    else if (event_state == Event_State::WAIT_FOR_RESPONSE)
    {
        Serial.println("waiting for a response...");
        delay(1000);
    }
    //  If event completed, switch to publishing 
    //  to enable a new event to be published. 
    else if (event_state == Event_State::COMPLETED)
    {
        event_state = Event_State::PUBLISHING;
    }
}

//*****************************************************************************
//
//! @brief Prints the application error caused by the last stage. Most of this
//!        errors occur when the Google APIs return an HTTP status codes 
//!        different than 200.
//!
//! @return None. 
//
//*****************************************************************************
void print_app_error(void)
{
    switch (last_app_stage)
    {
        case App_Stage::GEOLOCATION:
            //  Two actions can cause a faliure at this stage:
            //  1. Google Geolocation API returns 
            //     an unexpected HTTP status code. 
            if (Geolocation.failed())
            {
                Geolocation.print_error();
            }
            //  2. An inaccurate estimated location.
            else
            {
                Serial.println("\r\nError: The accuracy of the estimated location is insufficient.");
                Serial.print("Minimum accuracy: ");
                Serial.print(GEOLOC_MINIMUM_ACC);
                Serial.println(" meters.");
                Serial.print("Resulting accuracy: ");
                Serial.print(Geolocation.get_accuracy());
                Serial.println(" meters.");
            }
            break;

        case App_Stage::OAUTH2:
            OAuth2.print_error();
            break;

        case App_Stage::CALENDAR:
            Calendar.print_error();
            break;

        case App_Stage::DISTANCE_MATRIX:
            Distance_Matrix.print_error();
            break;

        default:
            break;
    }
    delay(1000);
}
