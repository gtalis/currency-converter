#include <iostream>
#include <map>
#include <stdlib.h>  

namespace currencyconverter {

typedef std::map <std::string, double> CurrencyRatesTable;

class RateRetriever {
public:
	RateRetriever();
	~RateRetriever();
	void getRates(CurrencyRatesTable &);
	void decodeData(void *buffer, size_t len);

private:
	void getDailyRates();
	void storeDailyRates();

	std::map<std::string, double> m_rates;
};

}
