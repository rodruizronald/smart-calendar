#ifndef __OAUTH2_H__
#define __OAUTH2_H__

//  Foward declaration.
class Google_Calendar;

//*****************************************************************************
//
//	Enumeration classes for the OAuth2.0 states.
//
//*****************************************************************************

enum class OAuth2_State : uint8_t
{
    REQ_USER_CODE,
    POLLING_AUTH,
    REFRESH_TOKEN,
    AUTHORIZED,
    WAIT_FOR_RESPONSE,
    FAILED
};

//*****************************************************************************
//
//! @brief Google OAuth2.0 class for TV and limited-input device applications.
//!
//! This class follows the official documentation provided by Google to 
//! implement the OAuth2.0 authorization protocol to access Google APIs.
//!
//! Source: https://developers.google.com/identity/protocols/OAuth2ForDevices 
//!
//! The class stores the refresh token in EEPROM and keeps track of the access
//! token lifetime. If the access token expries and the application calls the
//! Google_OAuth2::loop() member function, a new access token will be requested
//! without user intervention.
//!
//
//*****************************************************************************
class Google_OAuth2
{
    private:
        //  Google Calendar class is added as a friend class, 
        //  so it can access the private members, such as the access token. 
        friend class Google_Calendar;
        
        //  OAuth2.0 token structure.
        typedef struct oauth2_token
        {
            public:
            //  Flag to indicate token availability.
            //  if 0, token is available.
            //  if 1, token is not available.
            uint8_t available;
            //  Raw token data (max. 60 characters).
            char data[60];
            //  Default constructor.
            oauth2_token() : available(1), data("Empty") {};
        } OAuth2_Token;
        
        //  OAuth2.0 Refresh Token struct to store in memory.
        //  TOKEN_ADDRESS: Start address in EEPROM to write the token.
        //  TOKEN_LENGTH: Max number of characters to write.
        const uint8_t TOKEN_ADDRESS = 0;
        const uint8_t TOKEN_LENGTH = 60;
        OAuth2_Token Refresh_Token;
        
        //  Particle webhooks event name.
        const String EVENT_REQ_USER_CODE = "oauth_usr_code";
        const String EVENT_POLL_AUTH = "oauth_poll_auth";
        const String EVENT_REFRESH_TOKEN = "oauth_ref_token";
        
        //  OAuth2.0 client credentials.
        const String CLIENT_ID;
        const String CLIENT_SECRET;

        //  Properties of the authorization server response.
        String device_code;
        String user_code;
        String auth_url;

        //  OAuth2.0 authorization tokens.
        String access_token;
        String refresh_token;

        //  OAuth2.0 user code and access token valid time param.
        uint32_t time;
        int32_t life_time;

        //  Google's authorization server polling param.
        uint32_t polling_time;
        uint16_t polling_rate;
        
        //  OAuth2.0 protocol states.
        OAuth2_State state;
        OAuth2_State last_state;
        
        //  Webhooks subscription status.
        bool is_device_subscribed;
        
        //  Http status code and error response returned from webhooks.
        String http_error;
        uint16_t http_status_code;

        //  Private member functions.
        void subscribe_device_to(const String &event);
        void parser(const char *event, const char *data);
        void change_state_to(OAuth2_State new_state);
        bool time_left(void);
        void write_token(void);
        bool read_token(void);
        void erase_token(void);

    protected:
        //  Particle webhooks event handlers.
        void response_handler(const char *event, const char *data);
        void error_handler(const char *event, const char *data);
        
    public:
        //  Class constructor.
        Google_OAuth2(const String &client_id, const String &client_secret);

        //  Public member functions.
        void loop(void);
        void print_error(void);
        bool failed(void);
        bool authorized(void);
        bool authenticated(void);
        bool is_token_valid(void);
};

#endif  //  __OAUTH2_H__