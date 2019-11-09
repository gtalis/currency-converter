#include "rateretriever.h"
#include <curl/curl.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <exception>

using namespace currencyconverter;

void RateRetriever::getRates(CurrencyRatesTable & rates)
{
	getDailyRates();
	rates = m_rates;
}

RateRetriever::RateRetriever()
{
	m_rates = {};
}
	
RateRetriever::~RateRetriever()
{
}

#define ATTR_NAME_IS(A)  !strcmp( (const char *)attr->name, A)

static void print_element_names(xmlNode * a_node, CurrencyRatesTable *rates)
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
				printf("New Currency (%s) Value  (%f) new size of rates: %d\n", currency.c_str(), currencyVal, rates->size());
			}
			
			// Reset currency name
			currency= "none";
        }

        print_element_names(cur_node->children, rates);
    }
}

void RateRetriever::decodeData(void *buffer, size_t size)
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
    print_element_names(root_element, &m_rates);
}

static size_t decode_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	RateRetriever *rr = (RateRetriever *) userp;

	rr->decodeData(buffer, nmemb);
	return nmemb;
}

void RateRetriever::getDailyRates()
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

void RateRetriever::storeDailyRates()
{
	
}

static double currencyToRate(CurrencyRatesTable *rates, std::string &currency)
{
	double rate;
	if (currency == "EUR") {
		rate = (double)1;
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
	RateRetriever rr;

	CurrencyRatesTable todayRates;
	rr.getRates(todayRates);    
	
	std::string fromCurrency;
	std::string toCurrency;
	double sum;
	
	int c ;
    while( ( c = getopt (argc, argv, "f:t:v:") ) != -1 ) 
    {
        switch(c)
        {
            case 'f':
                if(optarg) fromCurrency = optarg;
                printf("Need to perform a conversion from %s\n", fromCurrency.c_str());
                break;
            case 't':
                if(optarg) toCurrency = optarg;
                printf("Need to perform a conversion to %s\n", toCurrency.c_str());
                break;
            case 'v':
                if(optarg) sum = std::atof(optarg);
                printf("Need to convert %f\n", sum);
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
	
	double convertedSum = sum * toRate / fromRate;
	printf("%f %s = %f %s\n", sum, fromCurrency.c_str(), convertedSum, toCurrency.c_str());

	return 0;
}


