#ifndef STRATEGY_H
#define STRATEGY_H

#include <bits/stdc++.h>
using namespace std;

enum class Side
{
    BUY,
    SELL,
    NONE
};

struct Signal
{
    string symbol;
    Side side;
    double price; // price to use for order (usually last 5-min close)
    int quantity; // suggested qty (placeholder)
    time_t timestamp;
};

using SignalCallback = function<void(const Signal &)>;

class StrategyEMA
{
public:
    // shortPeriod and longPeriod are counts of 5-minute candles (typical: 5 and 20)
    StrategyEMA(int shortPeriod = 5, int longPeriod = 20, SignalCallback cb = nullptr);

    // Call this each time a new 1-minute candle is completed.
    // Strategy groups five 1-min candles into a 5-min candle internally.
    // Returns true if a 5-min candle was formed (and processed).
    bool onNewMinuteCandle(const string &symbol,
                           double open, double high, double low, double close,
                           time_t timestamp);

    void setSignalCallback(SignalCallback cb);

    double getShortEMA() const;
    double getLongEMA() const;

private:
    int shortPeriod_; // in 5-min candles
    int longPeriod_;  // in 5-min candles
    SignalCallback callback_;

    struct MiniCandle
    {
        double open, high, low, close;
        time_t ts;
    };
    vector<MiniCandle> bucket5min_; // collects five 1-min candles

    vector<double> fiveMinCloses_; // history of 5-min closes

    // EMA state
    bool shortInit_;
    bool longInit_;
    double shortEMA_;
    double longEMA_;

    enum LastPos
    {
        UNDEF,
        SHORT_ABOVE,
        SHORT_BELOW
    } lastPos_;

    // helpers
    void processCompleted5MinCandle(const string &symbol, double open, double high, double low, double close, time_t ts);
    double computeSMA(const vector<double> &v, int n);
    double updateEMA(double prevEMA, double close, int period);
};

#endif
