#include "../include/PriceService.h"
#include <random>

PriceService::PriceService()
{
    instruments["NIFTY"] = {20000, 100};
    instruments["RELIANCE"] = {2500, 50};
    instruments["TCS"] = {3600, 80};
}

double PriceService::getPrice(const string &symbol)
{
    auto it = instruments.find(symbol);
    if (it == instruments.end())
        return 0.0;

    double mean = it->second.first;
    double stddev = it->second.second;

    normal_distribution<double> distribution(mean, stddev);
    return distribution(generator);
}
