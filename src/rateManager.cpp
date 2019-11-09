#include "rateManager.h"
#include <curl/curl.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <exception>

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

int RateManager::getStoredRates()
{
    return -1;
}

int RateManager::getECBRates()
{
  CURL *curl;
  CURLcode res;

  int ret = -1;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, EURO_FOREIGN_EXCHANGE_REFERENCE_RATES_LINK);

#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

	/* Define callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _ExtractRatesFromECBXml);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this); 

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
      fprintf(stderr, "ERROR: Failed to get exchange rates from European Central Bank: %s\n",
              curl_easy_strerror(res));
      ret = -1;
    } else {
        ret = 0;
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  return ret;
}

void RateManager::storeDailyRates()
{
	
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