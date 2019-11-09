#include <iostream>
#include <map>
#include <stdlib.h>  

namespace currencyconverter {

typedef std::map <std::string, double> CurrencyRatesTable;

class RateManager {
public:
	RateManager();
	~RateManager();
	void getRates(CurrencyRatesTable &);
	void decodeData(void *buffer, size_t len);

private:
	void getDailyRates();
	void storeDailyRates();

	CurrencyRatesTable m_rates;
};

}
