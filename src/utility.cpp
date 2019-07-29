#include "Particle.h"
#include "utility.h"

//*****************************************************************************
//
//! @brief Splits an string.
//!
//!	@param[in] str String object to be splited.
//!	@param[in] delimiter Character used to divide the string.
//!	@param[in] index Upper limit of the split.
//!	@param[in] last_index Lower limit of the split.
//!
//!	@return None.
//
//*****************************************************************************
String split_string(String &str, char delimiter, int16_t &index, int16_t &last_index)
{
    //  Search for the delimiter position.
    index = str.indexOf(delimiter, index);
    //  Generate an string from the given indexs values.
    String result = str.substring(last_index, index);
    last_index = ++index;
    return result;
}

//*****************************************************************************
//
//! @brief Converts the given time into a unix timestamp (since Jan 01 1970).
//!
//!	@param[in] year Number representation of the years to be converted.
//!	@param[in] month Number representation of the months to be converted.
//!	@param[in] day Number representation of the days to be converted.
//!	@param[in] hour Number representation of the hours to be converted.
//!	@param[in] min Number representation of the minutes to be converted.
//!	@param[in] sec Number representation of the seconds to be converted.
//!
//!	@return A unix timestamp corresponding to the parameters passed.
//
//*****************************************************************************
time_t unix_time(int year, int month, int day, int hour, int min, int sec)
{
  struct tm t;
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_isdst = 0;
  return mktime(&t);
}