/*
 *  Copyright (c) 2019 Gilles Talis
 *
 *	This file is part of CurrencyConverter.
 *
 *	CurrencyConverter is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	CurrencyConverter is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include <cstring>
#include <cstdlib>

static const char *XDG_LOCAL_DIR = "/.local/share";
std::string _getLocalDir()
{
	std::string dataHomeDir = {};
	char *_dataHomeDir = getenv("XDG_DATA_HOME");
	if (_dataHomeDir) {
	    dataHomeDir += _dataHomeDir;
	} else {
	    _dataHomeDir = getenv("HOME");
	    if (! _dataHomeDir) {
	        std::cerr << "Failed to find local home directory\n";
	        return dataHomeDir;
	    }

	    dataHomeDir += _dataHomeDir;
	    dataHomeDir += XDG_LOCAL_DIR;
	}

	dataHomeDir += "/";

	return dataHomeDir;
}

static const char *LOCAL_STORAGE_FN = "currency_converter.gt";
std::string _getStorageFileName()
{
	std::string storageFn = _getLocalDir();
	storageFn += LOCAL_STORAGE_FN;

    return storageFn;
}

static const char *LOCAL_LAST_UPDATED_FN = "currency_converter.last_updated";
std::string _getLastUpdatedFileName()
{
	std::string lastUpdatedFn = _getLocalDir();
	lastUpdatedFn += LOCAL_LAST_UPDATED_FN;

	return lastUpdatedFn;
}

#define ECB_RATES_UPDATE_TIME_UTC_HOUR		15
#define ECB_RATES_UPDATE_TIME_UTC_MIN		30
#define ECB_RATES_UPDATE_TIME_UTC_SEC		0
#define NUM_SECONDS_DAY						86400

time_t ecbDateToTime(const char *ecbDate)
{
	// ECB date format is: YYYY-MM-DD
	struct tm tmDate;

	std::string sdate = ecbDate;
	tmDate.tm_year = std::atoi ( sdate.substr(0,4).c_str() );
	tmDate.tm_year -= 1900;
	tmDate.tm_mon = std::atoi ( sdate.substr(5,2).c_str() );
	tmDate.tm_mon -= 1;
	tmDate.tm_mday = std::atoi ( sdate.substr(8,2).c_str() );

	tmDate.tm_hour = ECB_RATES_UPDATE_TIME_UTC_HOUR;
	tmDate.tm_min = ECB_RATES_UPDATE_TIME_UTC_MIN;
	tmDate.tm_sec = ECB_RATES_UPDATE_TIME_UTC_SEC;

	return mktime ( &tmDate );
}

static bool after_ecb_update_time_limit (const struct tm *now)
{
	if ((now->tm_hour > ECB_RATES_UPDATE_TIME_UTC_HOUR) ||
		((now->tm_hour == ECB_RATES_UPDATE_TIME_UTC_HOUR) && (now->tm_min > ECB_RATES_UPDATE_TIME_UTC_MIN)) ) {
		return true;
	}

	return false;
}

time_t getEcbLastUpdateTime()
{
	//
	// From "https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html"
	// The reference rates are usually updated around 16:00 CET on every working day,
	// except on TARGET closing days
	// This means that reference rates are updated Monday to Friday around 15:00 UTC
	// Our implementation gives time to the ECB to update the rates, so our reference
	// update time will be a bit later than 15:00 UTC
	// Note that our implementation DOES NOT take TARGET closing days into account
	// This is a limitation.

	// Here, implementation computes the last update date and time from the current UTC time

	// Get current UTC time
	time_t utcNow_;
	time ( &utcNow_ );
	struct tm * utcNow = gmtime ( &utcNow_ );

	// Now, find what was the last day rate were updated
	// If current UTC time is before time limit, rates were updated yesterday or
	// last friday otherwise, they were updated today
	int num_of_days_to_rewind_to = 0;

	// Get last update day
	switch (utcNow->tm_wday) {
		case 0: // sunday
			// last update was on friday, so 2 days ago
			num_of_days_to_rewind_to = 2;
			break;
		case 1: // monday
			if (after_ecb_update_time_limit(utcNow)) {
				// Current UTC time is after update time limit:
				// rates were updated today
				num_of_days_to_rewind_to = 0;
			} else {
				// Current UTC time is before update time limit:
				// last update was on friday, so 3 days ago.
				num_of_days_to_rewind_to = 3;
			}
			break;
		case 2: // tuesday
		case 3: // wednesday
		case 4: // thursday
		case 5: // friday
			if (after_ecb_update_time_limit(utcNow)) {
				// Current UTC time is after update time limit:
				// rates were updated today
				num_of_days_to_rewind_to = 0;
			}
			else {
				// Current UTC time is before update time limit:
				// last update was yesterday
				num_of_days_to_rewind_to = 1;
			}
			break;
		case 6: // saturday
			// last update was on friday, so yesterday
			num_of_days_to_rewind_to = 1;
			break;
	}
	utcNow->tm_hour = ECB_RATES_UPDATE_TIME_UTC_HOUR;
	utcNow->tm_min = ECB_RATES_UPDATE_TIME_UTC_MIN;
	utcNow->tm_sec = ECB_RATES_UPDATE_TIME_UTC_SEC;

	time_t ecbRatesLastUpdatedTime = mktime ( utcNow ) - (num_of_days_to_rewind_to*NUM_SECONDS_DAY);
	return ecbRatesLastUpdatedTime;
}
