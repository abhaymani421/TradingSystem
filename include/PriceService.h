#ifndef PRICESERVICE_H
#define PRICESERVICE_H

#include <bits/stdc++.h>
using namespace std;

class PriceService
{
private:
    map<string, pair<double, double>> instruments; // symbol -> (mean, stddev)
    default_random_engine generator;

public:
    PriceService();
    double getPrice(const string &symbol); // generates one price tick
};

#endif
