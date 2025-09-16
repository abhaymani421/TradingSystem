#ifndef PRICESERVICE_H
#define PRICESERVICE_H
#include <bits/stdc++.h>
using namespace std;
class PriceService
{
private:
    map<string, pair<double, double>> instruments; // symbol on instrument --> (mean price,std. Deviation)
    default_random_engine generator;

public:
    PriceService();
    double getPrice(const string &symbol);
    void logPricesToCSV(const string &filename, int seconds);
};

#endif