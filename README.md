# CurrencyConverter

Copyright (c) 2019, Gilles Talis <gilles.talis@gmail.com>

** CurrencyConverter ** is a very basic currency converter tool
It uses the [European Central Bank reference rates](https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html) to perform the conversions


## Build from source

### Dependencies
* curl
* libxml2

### Building
	mkdir build
	cd build
	cmake ..
	make
	

## Usage
	currencyconverter -a <amount_to_convert> -f <source_currency> -t <destination_currency>