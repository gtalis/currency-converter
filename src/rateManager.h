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
