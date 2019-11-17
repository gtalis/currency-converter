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

#ifndef CC_RATE_MANAGER_H
#define CC_RATE_MANAGER_H

#include <iostream>
#include <map>
#include <string>
#include <ctime>

namespace CurrencyConverter {

typedef std::map <std::string, double> CurrencyRatesTable;

class RateManager {
public:
	static RateManager& Instance();
	void ExtractRatesFromECBXml(void *buffer, size_t len);
	double Convert(const double &sum, const std::string &fromCurrency, const std::string &toCurrency);
	const time_t GetRatesLastUpdatedDate();

private:
	RateManager();
	~RateManager();
	int getRates();
	int getStoredRates();
	int getECBRates();
	void storeECBRates();
	void storedECBUpdateTime();
	double CurrencyToRate(const std::string &currencyCode);
	bool storedRatesUpToDate();

	CurrencyRatesTable m_rates;
	time_t m_lastUpdated;
    static RateManager m_instance;
};
} // namespace CurrencyConverter

#endif
