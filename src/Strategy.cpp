#include "../include/Strategy.h"
#include <iostream>
using namespace std;

StrategyEMA::StrategyEMA(int shortPeriod, int longPeriod, SignalCallback cb)
    : shortPeriod_(shortPeriod),
      longPeriod_(longPeriod),
      callback_(cb),
      shortInit_(false),
      longInit_(false),
      shortEMA_(0.0),
      longEMA_(0.0),
      lastPos_(UNDEF)
{
}

void StrategyEMA::setSignalCallback(SignalCallback cb)
{
    callback_ = cb;
}

double StrategyEMA::computeSMA(const vector<double> &v, int n)
{
    if ((int)v.size() < n)
        return 0.0;
    double s = 0.0;
    for (int i = (int)v.size() - n; i < (int)v.size(); ++i)
        s += v[i];
    return s / n;
}

double StrategyEMA::updateEMA(double prevEMA, double close, int period)
{
    double k = 2.0 / (period + 1);
    return close * k + prevEMA * (1 - k);
}

bool StrategyEMA::onNewMinuteCandle(const string &symbol, double open, double high, double low, double close, time_t timestamp)
{
    // add 1-minute candle to the 5-min bucket
    bucket5min_.push_back({open, high, low, close, timestamp});

    if ((int)bucket5min_.size() < 5)
        return false; // need 5 1-min candles

    // aggregate 5 1-min candles -> one 5-min candle
    double o = bucket5min_.front().open;
    double c = bucket5min_.back().close;
    double h = bucket5min_[0].high;
    double l = bucket5min_[0].low;
    for (int i = 1; i < 5; ++i)
    {
        if (bucket5min_[i].high > h)
            h = bucket5min_[i].high;
        if (bucket5min_[i].low < l)
            l = bucket5min_[i].low;
    }
    time_t ts = bucket5min_.front().ts; // start time of the 5-min candle

    // store 5-min close
    fiveMinCloses_.push_back(c);

    // initialize or update EMAs
    if (!shortInit_ && (int)fiveMinCloses_.size() >= shortPeriod_)
    {
        shortEMA_ = computeSMA(fiveMinCloses_, shortPeriod_);
        shortInit_ = true;
    }
    else if (shortInit_)
    {
        shortEMA_ = updateEMA(shortEMA_, c, shortPeriod_);
    }

    if (!longInit_ && (int)fiveMinCloses_.size() >= longPeriod_)
    {
        longEMA_ = computeSMA(fiveMinCloses_, longPeriod_);
        longInit_ = true;
    }
    else if (longInit_)
    {
        longEMA_ = updateEMA(longEMA_, c, longPeriod_);
    }

    // generate signal only if both EMAs initialized
    if (shortInit_ && longInit_)
    {
        LastPos current = (shortEMA_ > longEMA_) ? SHORT_ABOVE : SHORT_BELOW;
        if (lastPos_ == UNDEF)
        {
            lastPos_ = current;
        }
        else if (current != lastPos_)
        {
            // crossover happened
            Signal sig;
            sig.symbol = symbol;
            sig.price = c; // use 5-min close as signal price
            sig.timestamp = ts;
            sig.quantity = 1; // placeholder quantity
            sig.side = (current == SHORT_ABOVE) ? Side::BUY : Side::SELL;

            if (callback_)
                callback_(sig);

            // also log to CSV
            ofstream file("data/signals.csv", ios::app);
            if (file.tellp() == 0)
            {
                // file is empty → write header
                file << "time,symbol,side,price,quantity\n";
            }
            char buf[20];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&ts));
            file << buf << ","
                 << symbol << ","
                 << (sig.side == Side::BUY ? "BUY" : "SELL") << ","
                 << sig.price << ","
                 << sig.quantity << "\n";
            file.close();

            lastPos_ = current;
        }
    }

    bucket5min_.clear(); // ready for next 5-min bucket
    return true;
}

double StrategyEMA::getShortEMA() const { return shortEMA_; }
double StrategyEMA::getLongEMA() const { return longEMA_; }
