#ifndef __MP3_H__
#define __MP3_H__

//*****************************************************************************
//
//! @brief DFPlayer Mini class.
//!
//! This class provides basic functionality of the DFPlayer Mini module to
//! play MP3 files.
//!
//! Source: http://www.picaxe.com/docs/spe033.pdf
//
//*****************************************************************************
class DFPlayer_MP3
{
    private:
        //  Serial communication packet format
        enum Packet_Format
        {
            //  Start byte, 0x7E by default.
            PACKET_HEADER = 0,
            //  Version information.
            PACKET_VERSION = 1,
            //  Number of bytes (checksum not included).
            PACKET_LENGTH = 2,
            //  User command (play, pause, etc).
            PACKET_CMD = 3,
            //  User command feedback (optional).
            PACKET_ACK = 4,
            //  Param1: Query high data byte.
            //  Param2: Query low data byte.
            PACKET_PARAM = 5,
            //  Accumulation and verification (start/end byte not included).
            PACKET_CHECKSUM = 7,
            //  End byte, 0xEF by deault. 
            PACKET_TAIL = 9
        };
        //  Size of a packet
        const uint8_t BUFF_LENGTH = 10;
        
        //  Serial transmiter and receiver buffers.
        uint8_t rx_buff[10];
        uint8_t tx_buff[10];
        //  Serial receiver buffer index.
        uint8_t rx_index;

        //  Communication stream to control the serial interface.
        Stream &stream;

        //  Digital pin to check the current state of the DFPlayer.
        const uint8_t BUSY_PIN;
        
        //  Private member functions.
        void send_packet(void);
        void send_cmd(uint8_t cmd);
        void send_cmd(uint8_t cmd, uint16_t data);
        void send_cmd(uint8_t cmd, uint8_t high_data, uint8_t low_data);
        void uint16_to_array(uint16_t value, uint8_t *array);
        uint16_t array_to_uint16(uint8_t *array);
        uint16_t calc_checksum(uint8_t *buffer);
        void wait_for_reply(uint32_t time);

    public:
        //  Class constructor.
        DFPlayer_MP3(Stream &stream, uint8_t busy_pin);
        
        //  Public member functions.
        bool begin(void);
        void reset(void);
        void pause(void);
        void sleep(void);
        void next(void);
        void previous(void);
        bool free(void);
        void volume(uint8_t volume);
        void play_file(uint8_t file_num);
        void play_folder(uint8_t folder_num, uint8_t file_num);
};

#endif // __MP3_H__