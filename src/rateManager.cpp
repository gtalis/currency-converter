/*
 *  Copyright (c) 2019 Gilles Talis
 *
 *  This file is part of CurrencyConverter.
 *
 *  CurrencyConverter is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CurrencyConverter is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "rateManager.h"
#include <curl/curl.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <fstream>
#include "utils.h"

using namespace CurrencyConverter;

static const char * EURO_FOREIGN_EXCHANGE_REFERENCE_RATES_LINK =
    "https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml";

RateManager RateManager::m_instance = RateManager();

RateManager& RateManager::Instance()
{
    return m_instance;
}

RateManager::RateManager()
{
    m_rates = {};
    m_lastUpdated = 0;
}
	
RateManager::~RateManager()
{
}

bool RateManager::storedRatesUpToDate()
{
    //
    // From "https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html"
    // The reference rates are usually updated around 16:00 CET on every working day,
    // except on TARGET closing days
    // This means that reference rates are updated Monday to Friday around 15:00 UTC
    // Our implementation gives time to the ECB to update the rates, so our reference
    // update time will be a bit later than 15:00 UTC

    // Here, implementation computes the last update date and time from the current UTC time
    // and compares it with the last update date and time stored locally
    // If these numbers differ, it means that rates are not up to date and must be updated.
    //

    std::string updateDateFn = _getLastUpdatedFileName();

    // Try and open file that contains last updated date
    std::ifstream ifs;
    ifs.open (updateDateFn, std::ifstream::in);
    if (!ifs.is_open()) {
#ifdef DEBUG
        std::cerr << "Failed to open " << updateDateFn << '\n';
#endif
        return false;
    }

    // Get last updated date from file: it is a time_t value in decimal format
    char time[128];
    ifs.getline(time, 128);
    ifs.close();

    // Get ECB updated time as per current date and time
    // and compare with value retrieved from the file
    time_t ecbUpdatedTime = getEcbLastUpdateTime();
    time_t localUpdatedTime = static_cast<time_t> ( std::atoi ( time ) );

    if ( localUpdatedTime < ecbUpdatedTime)
        return false;

#ifdef DEBUG
    std::cout << "Rates are up to date: local=" << localUpdatedTime << ", website= " << ecbUpdatedTime << '\n';
#endif

    // Update "last updated" member variable
    m_lastUpdated = localUpdatedTime;
    return true;
}


#define ATTR_NAME_IS(A)  !strcmp( (const char *)attr->name, A)
static void _extractRatesFromXmlDoc(xmlNode * a_node, CurrencyRatesTable *rates, time_t *updateDate)
{
    xmlNode *cur_node = NULL;
    xmlAttr *attr = NULL;
    std::string currency = "none";
    double currencyVal;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            attr = cur_node->properties;
            while (attr) {
                if ( ATTR_NAME_IS("currency") ) {
                    currency = (const char *)attr->children->content;
                } else if (ATTR_NAME_IS("rate"))  {
                    currencyVal = std::atof( (const char *) attr->children->content);
                } else if (ATTR_NAME_IS("time"))  {
#ifdef DEBUG
                    printf("Exchange rate date is %s\n", (char *) attr->children->content);
#endif
                    *updateDate = ecbDateToTime( (char *) attr->children->content );
                }
                attr = attr->next;
            }

            if (currency != "none") {
                rates->insert(std::pair<std::string, double> (currency, currencyVal) );
#ifdef DEBUG
                printf("New Currency (%s) Value  (%.4f)\n", currency.c_str(), currencyVal);
#endif
            }

            // Reset currency name
            currency= "none";
        }

        _extractRatesFromXmlDoc(cur_node->children, rates, updateDate);
    }
}

void RateManager::ExtractRatesFromECBXml(void *buffer, size_t size)
{
   /*
     * The document being in memory, it have no base per RFC 2396,
     * and the "noname.xml" argument will serve as its base.
     */
    xmlDocPtr  doc = xmlReadMemory((const char *)buffer, (int)size, "noname.xml", NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse document\n");
		return;
    }

    /*Get the root element node */
    xmlNode *root_element = xmlDocGetRootElement(doc);

    _extractRatesFromXmlDoc(root_element, &m_rates, &m_lastUpdated);
}

static size_t _ExtractRatesFromECBXml(void *buffer, size_t size, size_t nmemb, void *userp)
{
	RateManager *rr = (RateManager *) userp;

	rr->ExtractRatesFromECBXml(buffer, nmemb);
	return nmemb;
}

int RateManager::getECBRates()
{
    CURL *curl;
    CURLcode ret = CURLE_COULDNT_CONNECT;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, EURO_FOREIGN_EXCHANGE_REFERENCE_RATES_LINK);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _ExtractRatesFromECBXml);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

        /* Try and get the XML file from European Central Bank */
        ret = curl_easy_perform(curl);
        if (ret != CURLE_OK) {
            std::cerr <<  "ERROR: Failed to get exchange rates from European Central Bank: " <<
                curl_easy_strerror(ret) << std::endl;
        }

        curl_easy_cleanup(curl);

        // Store rates and update time locally
        storeECBRates();
        storedECBUpdateTime();
    }

    curl_global_cleanup();
    return (ret == CURLE_OK) ? 0 : -1;
}

void RateManager::storeECBRates()
{
    std::string localStorageFn = _getStorageFileName();

    // Try and open local storage file for writing only
    std::ofstream ofs;
    ofs.open (localStorageFn, std::ofstream::out);

    if (ofs.is_open()) {
        for (auto& currency : m_rates) {
            ofs << currency.first << ":" << currency.second << '\n';
        }
    }

    ofs.close();
}

void RateManager::storedECBUpdateTime()
{
    std::string updateDateFn = _getLastUpdatedFileName();

    // Try and open local "last updated date" for writing only
    std::ofstream ofs;
    ofs.open (updateDateFn, std::ofstream::out);

    if (ofs.is_open()) {
        ofs << m_lastUpdated;
    }

    ofs.close();
}

int RateManager::getStoredRates()
{
    // Check that stored rates are not outdated
    if ( ! storedRatesUpToDate() ) {
#ifdef DEBUG
        std::cout << "Stored rates are outdated \n";
#endif
        return -1;
    }

    std::string localStorageFn = _getStorageFileName();

    // Try and open local storage file for reading
    std::ifstream ifs;
    ifs.open (localStorageFn, std::ifstream::in);
    if (!ifs.is_open()) {
        return -1;
    }

    // Now loop through each line of the file to retrieve stored data
    int ret = -1;
    char line[32];
    std::string lineS;
    while (!ifs.eof()) {
        ifs.getline(line, 32);
        lineS = line;
        // Last line is an empty line. We must ignore it
        if (! lineS.empty()) {
            // Currency code is 3 letters long
            std::string currencyCode = lineS.substr(0,3);
            // Rest of the line is the currency rate
            double currencyRate = std::atof(lineS.substr(4, lineS.size()).c_str() );
            m_rates.insert(std::pair<std::string, double> (currencyCode, currencyRate) );
            ret = 0;
        }
    }

    return ret;
}

int RateManager::getRates()
{
    if ( getStoredRates() < 0 ) {
        // stored rates are either outdated or non present:
        // get rates from European Central Bank website
        return getECBRates();
    }

    return 0;
}

double RateManager::CurrencyToRate(const std::string &currency)
{
    double rate;

    // EUR is not part of table, so we need to
    // treat it separately
    if (currency == "EUR") {
        rate = (double) 1;
        return rate;
    }

    try {
        rate = m_rates.at(currency);
    } catch (std::exception &e) {
        return (double) -1;
    }

    return rate;
}

double RateManager::Convert(const double &sum, const std::string &fromCurrency, const std::string &toCurrency)
{
    if ( getRates() < 0) {
        return (double) -1;
    }

    double toRate = CurrencyToRate(toCurrency);
    if (toRate < 0) {
        std::cerr << "ERROR: Could not find Currency " << toCurrency << '\n';
        return (double) -1;
    }

    double fromRate = CurrencyToRate(fromCurrency);
    if (fromRate < 0) {
        std::cerr << "ERROR: Could not find Currency " << fromCurrency << '\n';
        return (double) -1;
    }

   if (sum < 0) {
        std::cerr << "ERROR: Sum to convert (" << sum << ") is invalid\n";
        return (double) -1;
    }

    return sum * toRate / fromRate;
}

const time_t RateManager::GetRatesLastUpdatedDate()
{
    return m_lastUpdated;
}