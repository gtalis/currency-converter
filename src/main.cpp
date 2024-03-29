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

#include "rateManager.h"
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <ctime>

static const int NUM_OF_ARGS_VERSION_HELP	= 2;
static const int NUM_OF_ARGS_CONVERSION		= 7;

void print_details()
{
	std::cout <<
		"CurrencyConverter " \
		VERSION \
		" - " \
		"Copyright (c) 2019 Gilles Talis \n";
}

void usage()
{
	std::cout <<
		"Usage: \n" \
		"  currencyconverter [options]\n" \
		"  currencyconverter -a <amount> -f <currency> -t <currency>\n\n" \
		"Performs currency conversion based on European Central Bank reference rates \n\n" \
		"Options: \n" \
		"-h 		Displays this help message\n" \
		"-v 		Displays tool version\n" \
		"-t <currency>	Defines the currency to convert the amount to\n" \
		"-f <currency>	Defines the currency to convert the amount from\n" \
		"-a <amount>	Sets the amount to convert\n";
}


int main(int argc, char **argv)
{
	CurrencyConverter::RateManager &rr = CurrencyConverter::RateManager::Instance();

	std::string fromCurrency = {};
	std::string toCurrency = {};
	double amount = 0;

	// For now, we only accept:
	// - one argument (help or version)
	// - six arguments: sum, source currency, destination currency
	if ( (argc != NUM_OF_ARGS_VERSION_HELP) // help / version
		&& (argc != NUM_OF_ARGS_CONVERSION) // conversion
		) {
		usage();
		return 0;
	}

	int c ;
	while( ( c = getopt (argc, argv, "f:t:a:vh") ) != -1 )
	{
		switch(c)
		{
			case 'f':
				if(optarg) fromCurrency = optarg;
				break;
			case 't':
				if(optarg) toCurrency = optarg;
				break;
			case 'a':
				if(optarg) amount = std::atof(optarg);
				break;
			case 'v':
				print_details();
				return 0;
				break;
			case 'h':
				usage();
				return 0;
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
				break;
		}
	}

	print_details();
	double convertedAmount = rr.Convert(amount, fromCurrency, toCurrency);
	if (convertedAmount >= 0) {
		time_t update = rr.GetRatesLastUpdatedDate();
		printf("%.2f %s = %.2f %s\n", amount, fromCurrency.c_str(), convertedAmount, toCurrency.c_str());
		std::cout << "European Central Bank reference rates last update: UTC " << ctime ( &update );
	}

	return 0;
}
