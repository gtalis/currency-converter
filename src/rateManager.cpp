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

using namespace CurrencyConverter;

static const char * EURO_FOREIGN_EXCHANGE_REFERENCE_RATES_LINK =
    "https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml";

RateManager RateManager::m_instance = RateManager();

RateManager& RateManager::Instance()
{
    return m_instance;
}

int RateManager::getRates()
{
	if ( getStoredRates() < 0 ) {
        return getECBRates();
    }

    return 0;
}

RateManager::RateManager()
{
	m_rates = {};
}
	
RateManager::~RateManager()
{
}

#define ATTR_NAME_IS(A)  !strcmp( (const char *)attr->name, A)
static void _extractRatesFromXmlDoc(xmlNode * a_node, CurrencyRatesTable *rates)
{
    xmlNode *cur_node = NULL;
    xmlNode *children = NULL;
    
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
                    printf("Exchange rate date is %s\n", (char *) attr->children->content);
                }
            	attr = attr->next;
            }

			if (currency != "none") {
				rates->insert(std::pair<std::string, double> (currency, currencyVal) );
				printf("New Currency (%s) Value  (%.4f)\n", currency.c_str(), currencyVal);
			}
			
			// Reset currency name
			currency= "none";
        }

        _extractRatesFromXmlDoc(cur_node->children, rates);
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
    _extractRatesFromXmlDoc(root_element, &m_rates);
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

        // Store rates locally
        storeECBRates();
    }

    curl_global_cleanup();
    return (ret == CURLE_OK) ? 0 : -1;
}

static const char *XDG_LOCAL_DIR = "/.local/share";
static const char *LOCAL_STORAGE_FN = "currency_converter.gt";

static std::string _getStorageFileName()
{
    std::string dataHomeDir;
    char *_dataHomeDir = getenv("XDG_DATA_HOME");
    if (_dataHomeDir) {
        dataHomeDir += _dataHomeDir;
    } else {
        _dataHomeDir = getenv("HOME");
        if (! _dataHomeDir) {
            printf("Failed to find local home directory\n");
            return {};
        }

        dataHomeDir += _dataHomeDir;
        dataHomeDir += XDG_LOCAL_DIR;
    }

    std::string storageFn = dataHomeDir;
    storageFn += "/";
    storageFn += LOCAL_STORAGE_FN;

    return storageFn;
}

void RateManager::storeECBRates()
{
    std::string localStorageFn = _getStorageFileName();

    // Try and open local storage file for writing
    std::ofstream ofs;
    ofs.open (localStorageFn, std::ofstream::out);

    if (ofs.is_open()) {
        for (auto& currency : m_rates) {
            ofs << currency.first << ":" << currency.second << '\n';
        }
    }

    ofs.close();
}

int RateManager::getStoredRates()
{
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

double RateManager::CurrencyToRate(const std::string &currency)
{
	double rate;

	// EUR is not part of table, so we need to
	// treat it separately
	if (currency == "EUR"){
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