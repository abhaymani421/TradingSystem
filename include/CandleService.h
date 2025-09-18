#ifndef CANDLESERVICE_H
#define CANDLESERVICE_H

#include <bits/stdc++.h>
using namespace std;

struct Candle
{
    string symbol;
    double open, high, low, close;
    time_t timestamp; // store start time of candle
};

class CandleService
{
private:
    int candleDuration; // in seconds
    map<string, vector<Candle>> candles;

public:
    CandleService(int duration);
    void addPrice(const string &symbol, double price, time_t timestamp);
    vector<Candle> getCandles(const string &symbol);
    void printCandles(const string &symbol);
    void logCandlesToCSV(const string &filename, const string &symbol);
};

#endif
