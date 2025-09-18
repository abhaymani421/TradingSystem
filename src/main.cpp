#include "../include/PriceService.h"
#include "../include/CandleService.h"
#include <windows.h>
#include <fstream>
#include <ctime>
using namespace std;

int main()
{
    PriceService priceService;
    CandleService candleService(60); // 5-sec candles

    string symbol = "NIFTY";
    int duration;
    cout << "Enter how many seconds to run simulation: ";
    cin >> duration;

    ofstream priceFile("data/prices.csv");
    priceFile << "time,symbol,ltp\n";

    time_t start = time(0);

    while (time(0) - start < duration)
    {
        double price = priceService.getPrice(symbol);
        time_t now = time(0);

        // write raw tick
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        priceFile << buf << "," << symbol << "," << price << "\n";

        // feed into candle generator
        candleService.addPrice(symbol, price, now);

        cout << "Tick: " << price << " at " << buf << endl;
        Sleep(1000); // 1 second
    }

    priceFile.close();

    // after loop
    candleService.logCandlesToCSV("data/candles.csv", symbol);
    candleService.printCandles(symbol);

    return 0;
}
