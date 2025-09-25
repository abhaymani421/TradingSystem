#include "../include/windows_wrapper.h"
#include "../include/PriceService.h"
#include "../include/CandleService.h"
#include "../include/Strategy.h"
#include "../include/ExecutionService.h"
#include <windows.h>
#include <fstream>
#include <ctime>
#include <iostream>
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

    // Pass PriceService instance to ExecutionService
    ExecutionService execService(priceService);

    // Create EMA strategy — short=5, long=20 (in 5-min candles)
    StrategyEMA strategy(1, 2, [&](const Signal &s)
                         {
                             cout << "[Signal] " << (s.side == Side::BUY ? "BUY" : "SELL")
                                  << " " << s.symbol << " price=" << s.price
                                  << " time=" << ctime(&s.timestamp);

                             // Place order via ExecutionService
                             // Using LIMIT order type for demo and fixed quantity 50
                             execService.place_order(s.symbol, 
                                                     (s.side == Side::BUY ? OrderSide::BUY : OrderSide::SELL),
                                                     OrderType::LIMIT,
                                                     s.price, 
                                                     50); });

    size_t lastMinuteCount = 0;
    time_t start = time(0);

    while (time(0) - start < duration)
    {
        double price = priceService.getPrice(symbol);
        time_t now = time(0);

        // Write raw tick to CSV
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        priceFile << buf << "," << symbol << "," << price << "\n";

        // Feed tick into candle generator
        candleService.addPrice(symbol, price, now);

        // Check if a new 1-min candle formed
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

        // Update all orders based on latest LTP
        execService.update_all_orders();

        cout << "Tick: " << price << " at " << buf << endl;
        Sleep(1000); // 1 second
    }

    priceFile.close();

    // After loop, save candles and positions
    candleService.logCandlesToCSV("data/candles.csv", symbol);
    candleService.printCandles(symbol);
    execService.write_positions_csv("data/positions.csv");
    execService.write_dashboard_csv("data/dashboard.csv");
    // execService.overwrite_orders_csv(); // optional: full order snapshot

    return 0;
}
