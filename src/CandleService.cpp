
#include "../include/CandleService.h"
#include <fstream>
#include <iomanip>
#include <ctime>
using namespace std;

CandleService::CandleService(int duration) : candleDuration(duration) {}

void CandleService::addPrice(const string &symbol, double price, time_t timestamp)
{
    vector<Candle> &cList = candles[symbol];

    if (cList.empty() || timestamp - cList.back().timestamp >= candleDuration)
    {
        Candle newCandle = {symbol, price, price, price, price, timestamp};
        cList.push_back(newCandle);
    }
    else
    {
        Candle &last = cList.back();
        last.close = price;
        if (price > last.high)
            last.high = price;
        if (price < last.low)
            last.low = price;
    }
}

vector<Candle> CandleService::getCandles(const string &symbol)
{
    return candles[symbol];
}

void CandleService::printCandles(const string &symbol)
{
    vector<Candle> cList = candles[symbol];
    cout << "Candles for " << symbol << ":\n";
    for (const Candle &c : cList)
    {
        tm *timeinfo = localtime(&c.timestamp);
        cout << put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " | "
             << "O:" << c.open << " H:" << c.high << " L:" << c.low << " C:" << c.close << "\n";
    }
}

void CandleService::logCandlesToCSV(const string &filename, const string &symbol)
{
    ofstream file(filename);
    file << "time,open,high,low,close\n";

    for (auto &c : candles[symbol])
    {
        tm *timeinfo = localtime(&c.timestamp);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);

        file << buf << ","
             << c.open << "," << c.high << "," << c.low << "," << c.close << "\n";
    }
    file.close();
}
