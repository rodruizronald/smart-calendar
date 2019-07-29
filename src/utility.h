#ifndef __UTILITY_H__
#define __UTILITY_H__

//  Application template to convert 
//  enumeration classes into unsigned 8-bit numbers.
template <typename enum_class>
uint8_t enum_to_uint8(enum_class ec)
{
    return static_cast<uint8_t>(ec);
}

//  Utility functions.
extern String split_string(String &str, char delimiter, int16_t &index, int16_t &last_index);
extern time_t unix_time(int year, int month, int day, int hour, int min, int sec);


#endif // __UTILITY_H__