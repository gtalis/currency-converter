#include "rateManager.h"
#include <unistd.h>
#include <string>

int main(int argc, char **argv)
{
	CurrencyConverter::RateManager &rr = CurrencyConverter::RateManager::Instance();

	CurrencyConverter::CurrencyRatesTable todayRates;
	rr.getRates(todayRates);   
	
	std::string fromCurrency;
	std::string toCurrency;
	double sum = 0;
	
	int c ;
    while( ( c = getopt (argc, argv, "f:t:s:") ) != -1 ) 
    {
        switch(c)
        {
            case 'f':
                if(optarg) fromCurrency = optarg;
                break;
            case 't':
                if(optarg) toCurrency = optarg;
                break;
            case 's':
                if(optarg) sum = std::atof(optarg);
                break;
        }
    }
	
    double convertedSum = rr.Convert(sum, fromCurrency, toCurrency);
	printf("%.4f %s = %.4f %s\n", sum, fromCurrency.c_str(), convertedSum, toCurrency.c_str());

	return 0;
}