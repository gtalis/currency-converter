#include <iostream>
#include <map>
#include <stdlib.h>
#include <string>

namespace CurrencyConverter {

typedef std::map <std::string, double> CurrencyRatesTable;

class RateManager {
public:
	static RateManager& Instance();
	void getRates(CurrencyRatesTable &);
	void decodeData(void *buffer, size_t len);
	double Convert(const double &sum, const std::string &fromCurrency, const std::string &toCurrency);

private:
	RateManager();
	~RateManager();
	void getDailyRates();
	void storeDailyRates();
	double CurrencyToRate(const std::string &currencyCode);

	CurrencyRatesTable m_rates;
    static RateManager m_instance;
};

}
