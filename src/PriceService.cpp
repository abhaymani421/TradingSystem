// Generate random LTP(last traded price)
#include "PriceService.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <thread>
#include <iostream>
#include <windows.h>

using namespace std;

PriceService::PriceService()
{
    // symbol -> (mean, stddev)
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

    std::normal_distribution<double> distribution(mean, stddev);
    return distribution(generator);
}

void PriceService::logPricesToCSV(const string &filename, int seconds)
{
    ofstream file(filename);
    file << "time,symbol,ltp\n";

    for (int i = 0; i < seconds; i++)
    {
        // get current time
        auto now = chrono::system_clock::now();
        time_t now_c = chrono::system_clock::to_time_t(now);
        tm tm = *localtime(&now_c);

        char timeBuf[9];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tm);

        // log for each symbol
        for (const auto &pair : instruments)
        {
            string symbol = pair.first;
            double price = getPrice(symbol);
            file << timeBuf << "," << symbol << "," << price << "\n";
        }

        file.flush(); // write to disk
        Sleep(1000);  // simulate 1 sec tick
    }

    file.close();
}
