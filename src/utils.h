#ifndef CC_UTILS_H
#define CC_UTILS_H

#include "rateManager.h"
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <fstream>

std::string _getStorageFileName();
std::string _getLastUpdatedFileName();
time_t ecbDateToTime(const char *date);
time_t getEcbLastUpdateTime();

#endif
