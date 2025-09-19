#include "../include/PriceService.h"
#include "../include/CandleService.h"
#include "../include/Strategy.h"
#include <windows.h>
#include <fstream>
#include <ctime>
using namespace std;

int main()
{
    PriceService priceService;
    CandleService candleService(60); // 1-min candles (60 seconds)

    string symbol = "NIFTY";
    int duration;
    cout << "Enter how many seconds to run simulation: ";
    cin >> duration;

    ofstream priceFile("data/prices.csv");
    priceFile << "time,symbol,ltp\n";

    // Create strategy — short=5, long=20 (in 5-min candles)
    StrategyEMA strategy(5, 20, [](const Signal &s)
                         {
                             cout << "[Signal] " << (s.side == Side::BUY ? "BUY" : "SELL")
                                  << " " << s.symbol << " price=" << s.price
                                  << " time=" << ctime(&s.timestamp);
                             // later this callback -> ExecutionService::placeOrder(...)
                         });

    size_t lastMinuteCount = 0;
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

        // check if new 1-min candle formed
        auto curr = candleService.getCandles(symbol);
        if (curr.size() > lastMinuteCount)
        {
            const Candle &c1 = curr.back();
            bool formed5min = strategy.onNewMinuteCandle(
                symbol, c1.open, c1.high, c1.low, c1.close, c1.timestamp);

            if (formed5min)
            {
                cout << "[Strategy] Processed a new 5-min candle.\n";
            }

            lastMinuteCount = curr.size();
        }

        cout << "Tick: " << price << " at " << buf << endl;
        Sleep(1000); // 1 second
    }

    priceFile.close();

    // after loop
    candleService.logCandlesToCSV("data/candles.csv", symbol);
    candleService.printCandles(symbol);

    return 0;
}
