// Entry Point of the application
#include "../include/PriceService.h"
#include <bits/stdc++.h>
using namespace std;

int main()
{
    PriceService ps;
    ps.logPricesToCSV("data/prices.csv", 10); // run for 10 seconds
    return 0;
}