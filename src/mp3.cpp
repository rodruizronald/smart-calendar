#include "Particle.h"
#include "mp3.h"

//*****************************************************************************
//
//! @brief DFPlayer Mini class constructor.
//!
//! It initializes the transmitter buffer, and sets the busy pin and serial
//! stream to communicate with the MP3 player.
//!
//!	@param[in] stream Communication stream to control the serial interface.
//!	@param[in] BUSY_PIN Digital pin to check the current state of the DFPlayer. 
//
//*****************************************************************************
DFPlayer_MP3::DFPlayer_MP3(Stream &stream, uint8_t BUSY_PIN)
    : stream(stream), BUSY_PIN(BUSY_PIN)
{
    pinMode(this->BUSY_PIN, INPUT);
    tx_buff[PACKET_HEADER] = 0x7E;
    tx_buff[PACKET_VERSION] = 0xFF;
    tx_buff[PACKET_LENGTH] = 0x06;
    tx_buff[PACKET_CMD] = 0x01;
    tx_buff[PACKET_TAIL] = 0xEF;
}

//*****************************************************************************
//
//! @brief Initializes the DFPlayer Mini.
//!
//! @return false if failed, true if did not fail.
//
//*****************************************************************************
bool DFPlayer_MP3::begin(void)
{
    //  Send a reset command.
    reset();
    //  Read the receiving line (Rx) to get reply.
    //  It must wait at least 1.5 seconds after reset.
    wait_for_reply(2000);
    //  Calculate the checksum from the last
    //  packet received and check for errors.
    uint16_t checksum_calc = calc_checksum(rx_buff);
    uint16_t checksum_received = array_to_uint16(&rx_buff[PACKET_CHECKSUM]);
    if (checksum_calc == checksum_received)
    {
        //  The commnad "0x3F" indicates that 
        //  the MP3 player was succesffully initilized.
        uint8_t cmd = rx_buff[PACKET_CMD];
        if (cmd == 0x3F)
        {
            return true;
        }
    }

    return false;
}

//*****************************************************************************
//
//! @brief Waits for a reply from the DFPlayer Mini.
//!
//!	@param[in] time Amount of time to poll the serial interface.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::wait_for_reply(uint32_t time)
{
    uint32_t now = millis();
    rx_index = 0;
    //  Poll the serial interface while there is still time left 
    //  and the number of bytes received is less than BUFF_LENGTH.
    while ((millis() - now) < time)
    {
        if (stream.available() && rx_index < BUFF_LENGTH)
        {
            rx_buff[rx_index] = stream.read();
            rx_index++;
        }
    }
}

//*****************************************************************************
//
//! @brief Calculates the checksum of a serial packet.
//!
//!	@param[in] buffer Serial receiver buffer.
//!
//! @return  A unsiged 16-bit number.
//
//*****************************************************************************
uint16_t DFPlayer_MP3::calc_checksum(uint8_t *buffer)
{
    uint16_t sum = 0;
    //  Start/End byte are not included in the checksum.
    for (int i = PACKET_VERSION; i < PACKET_CHECKSUM; i++)
    {
        sum += buffer[i];
    }
    return -sum;
}

//*****************************************************************************
//
//! @brief Sends a serial packet to the DFPlayer Mini.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::send_packet(void)
{
    //  Transmit the packet.
    stream.write(tx_buff, BUFF_LENGTH);
    //  Read the receiving line (Rx) to get reply.
    //  It should wait at least 75 ms to let 
    //  the DFPlayer Mini process the packet.
    wait_for_reply(75);
}

//*****************************************************************************
//
//! @brief Sends a command without data.
//!
//! @param[in] cmd DFPlayer Mini command to be executed.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::send_cmd(uint8_t cmd)
{
    send_cmd(cmd, 0);
}

//*****************************************************************************
//
//! @brief Forms and sends a serial packet.
//!
//!	@param[in] cmd DFPlayer Mini command to be executed.
//!	@param[in] data Serial data to be sent to the DFPlayer Mini.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::send_cmd(uint8_t cmd, uint16_t data)
{
    //  Add the serial command.
    tx_buff[PACKET_CMD] = cmd;
    //  Convert the serial data into an array 
    //  and add the checksum to the packet.
    uint16_to_array(data, &tx_buff[PACKET_PARAM]);
    uint16_to_array(calc_checksum(tx_buff), &tx_buff[PACKET_CHECKSUM]);
    send_packet();
}

//*****************************************************************************
//
//! @brief Sends a command with data. 
//!
//!	@param[in] cmd DFPlayer Mini command to be executed.
//!	@param[in] high_data Serial high data byte (Param1).
//!	@param[in] low_data Serial low data byte (Param2).
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::send_cmd(uint8_t cmd, uint8_t high_data, uint8_t low_data)
{
    uint16_t buffer = high_data;
    buffer <<= 8;
    send_cmd(cmd, (buffer | low_data));
}

//*****************************************************************************
//
//! @brief Convert an unsigned 8-bit array element into an unsigned 16-bit number. 
//!
//!	@param[in] array Pointer to a 8-bit array element to be converted.
//!
//! @return An unsigned 16-bit number.
//
//*****************************************************************************
uint16_t DFPlayer_MP3::array_to_uint16(uint8_t *array)
{
    uint16_t value = *array;
    value <<= 8;
    value += *(array + 1);
    return value;
}

//*****************************************************************************
//
//! @brief Convert an unsigned 16-bit number into an unsigned 8-bit array 
//!        element. 
//!
//!	@param[in] value Unsigned 16-bit number to be converted.
//!	@param[in] array Pointer to an array where the value will be added.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::uint16_to_array(uint16_t value, uint8_t *array)
{
    *array = (uint8_t)(value >> 8);
    *(array + 1) = (uint8_t)(value);
}

//*****************************************************************************
//
//! @brief Returns the current state of the DFPlayer Mini.
//!
//! @return false if busy, true if free.
//
//*****************************************************************************
bool DFPlayer_MP3::free(void)
{
    return digitalRead(BUSY_PIN);
}

//*****************************************************************************
//
//! @brief Plays the next MP3 file.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::next(void)
{
    send_cmd(0x01);
}

//*****************************************************************************
//
//! @brief Plays the previous MP3 file.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::previous(void)
{
    send_cmd(0x02);
}

//*****************************************************************************
//
//! @brief Plays a specific MP3 file.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::play_file(uint8_t file_num)
{
    send_cmd(0x03, file_num);
}

//*****************************************************************************
//
//! @brief Increases/Decreases the volume (from 0-30).
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::volume(uint8_t volume)
{
    send_cmd(0x06, volume);
}

//*****************************************************************************
//
//! @brief Puts the DFPlayer Mini into sleep mode.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::sleep(void)
{
    send_cmd(0x0A);
}

//*****************************************************************************
//
//! @brief Resets the DFPlayer Mini.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::reset(void)
{
    send_cmd(0x0C);
}

//*****************************************************************************
//
//! @brief Pauses the DFPlayer Mini.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::pause(void)
{
    send_cmd(0x0E);
}

//*****************************************************************************
//
//! @brief Plays an MP3 file stored in a specific folder.
//!
//! @return None.
//
//*****************************************************************************
void DFPlayer_MP3::play_folder(uint8_t folder_num, uint8_t file_num)
{
    send_cmd(0x0F, folder_num, file_num);
}
