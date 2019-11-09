#include "rateManager.h"
#include <curl/curl.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <exception>

using namespace currencyconverter;

void RateManager::getRates(CurrencyRatesTable & rates)
{
	getDailyRates();
	rates = m_rates;
}

RateManager::RateManager()
{
	m_rates = {};
}
	
RateManager::~RateManager()
{
}

#define ATTR_NAME_IS(A)  !strcmp( (const char *)attr->name, A)

static void store_rates(xmlNode * a_node, CurrencyRatesTable *rates)
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

        store_rates(cur_node->children, rates);
    }
}

void RateManager::decodeData(void *buffer, size_t size)
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
    store_rates(root_element, &m_rates);
}

static size_t decode_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	RateManager *rr = (RateManager *) userp;

	rr->decodeData(buffer, nmemb);
	return nmemb;
}

void RateManager::getDailyRates()
{
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml");

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
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, decode_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this); 

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();	
}

void RateManager::storeDailyRates()
{
	
}

static double currencyToRate(CurrencyRatesTable *rates, std::string &currency)
{
	double rate;

	// EUR is not part of table, so we need to
	// treat it separately
	if (currency == "EUR"){
		rate = (double) 1;
		return rate;
	}

	try {
		rate = rates->at(currency);
	} catch (std::exception &e) {
		return (double) -1;
	}
	
	return rate;	
}


int main(int argc, char **argv)
{
	RateManager rr;

	CurrencyRatesTable todayRates;
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
	
	double toRate = currencyToRate(&todayRates, toCurrency);
	if (toRate < 0) {
		std::cout << "ERROR: Could not find Currency " << toCurrency << '\n';
	}
	
	double fromRate = currencyToRate(&todayRates, fromCurrency);
	if (fromRate < 0) {
		std::cout << "ERROR: Could not find Currency " << fromCurrency << '\n';
	}

    if (!sum or sum < 0) {
        std::cout << "ERROR: Sum to convert (" << sum << ") is invalid\n";
    }
	
	double convertedSum = sum * toRate / fromRate;
	printf("%.4f %s = %.4f %s\n", sum, fromCurrency.c_str(), convertedSum, toCurrency.c_str());

	return 0;
}