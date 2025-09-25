#ifndef EXECUTIONSERVICE_H
#define EXECUTIONSERVICE_H

#include <bits/stdc++.h>

#include "PriceService.h"
using namespace std;

enum class OrderStatus
{
    OPEN,
    PARTIAL,
    COMPLETED
};
enum class OrderSide
{
    BUY,
    SELL
};
enum class OrderType
{
    LIMIT,
    MARKET
};

struct Order
{
    int id;
    string symbol;
    OrderType type;
    OrderSide side;
    double price;   // order price (limit)
    int quantity;   // total requested
    int filled_qty; // total filled so far
    OrderStatus status;
    time_t created_ts;
};

struct Position
{
    int quantity;
    double avg_entry;   // average entry price (for longs positive qty)
    double realized_pl; // realized pnl for that symbol
    double urpl = 0.0;
    double pl = 0.0;
};

class ExecutionService
{
public:
    ExecutionService(PriceService &ps);
    ~ExecutionService();

    // place an order (records it and returns id)
    int place_order(const string &symbol, OrderSide side, OrderType type, double price, int quantity);

    // check and update status for a particular order (uses LTP)
    void update_order_status(int order_id);

    // aggressive chasing: modify order price towards LTP until fill or attempts exhausted
    void chase_order(int order_id, int maxAttempts = 10, int stepMillis = 1000);

    // convenience: run update on all open/partial orders
    void update_all_orders();

    // write positions snapshot to CSV
    void write_positions_csv(const string &filename = "data/positions.csv");

    // helper: get order by id (const)
    Order *get_order(int order_id);
    void write_dashboard_csv(const string &filename);

private:
    PriceService &priceService;
    int nextOrderId;
    map<int, Order> orders;          // order_id -> Order
    map<string, Position> positions; // symbol -> position

    // internal helpers
    void persist_order_row(const Order &o);
    void overwrite_orders_csv(); // optional complete rewrite (not used frequently)
    void maybe_create_files();   // ensure headers exist
    void apply_fill(Order &o, int fill_qty, double fill_price);
};

#endif
