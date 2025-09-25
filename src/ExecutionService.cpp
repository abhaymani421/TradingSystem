#include "../include/windows_wrapper.h"
#include "../include/ExecutionService.h"
#include <fstream>
#include <windows.h> // Sleep
#include <random>
#include <filesystem>

using namespace std;

ExecutionService::ExecutionService(PriceService &ps) : priceService(ps), nextOrderId(1)
{
    maybe_create_files();
}

ExecutionService::~ExecutionService()
{
    // persist positions on destruction
    write_positions_csv();
}

void ExecutionService::maybe_create_files()
{
    // ensure data directory exists
    std::filesystem::create_directories("data");

    // ensure orders.csv exists with header
    if (!std::filesystem::exists("data/orders.csv"))
    {
        ofstream f("data/orders.csv");
        f << "id,time,symbol,side,type,order_price,quantity,filled_qty,status\n";
        f.close();
    }

    // ensure positions.csv exists with header
    if (!std::filesystem::exists("data/positions.csv"))
    {
        ofstream f("data/positions.csv");
        f << "symbol,quantity,avg_entry,realized_pl\n";
        f.close();
    }
}

int ExecutionService::place_order(const string &symbol, OrderSide side, OrderType type, double price, int quantity)
{
    Order o;
    o.id = nextOrderId++;
    o.symbol = symbol;
    o.type = type;
    o.side = side;
    o.price = price;
    o.quantity = quantity;
    o.filled_qty = 0;
    o.status = OrderStatus::OPEN;
    o.created_ts = time(0);

    orders[o.id] = o;
    persist_order_row(o);
    return o.id;
}

void ExecutionService::persist_order_row(const Order &o)
{
    ofstream f("data/orders.csv", ios::app);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&o.created_ts));
    string side = (o.side == OrderSide::BUY) ? "BUY" : "SELL";
    string type = (o.type == OrderType::LIMIT) ? "LIMIT" : "MARKET";
    string status;
    if (o.status == OrderStatus::OPEN)
        status = "OPEN";
    else if (o.status == OrderStatus::PARTIAL)
        status = "PARTIAL";
    else
        status = "COMPLETED";

    f << o.id << "," << buf << "," << o.symbol << "," << side << "," << type << ","
      << o.price << "," << o.quantity << "," << o.filled_qty << "," << status << "\n";
    f.close();
}

Order *ExecutionService::get_order(int order_id)
{
    auto it = orders.find(order_id);
    if (it != orders.end())
    {
        return &it->second; // return pointer to existing order
    }
    return nullptr; // order not found
}

void ExecutionService::apply_fill(Order &o, int fill_qty, double fill_price)
{
    if (fill_qty <= 0)
        return;
    if (fill_qty + o.filled_qty > o.quantity)
        fill_qty = o.quantity - o.filled_qty;

    // update fill
    o.filled_qty += fill_qty;
    if (o.filled_qty >= o.quantity)
    {
        o.status = OrderStatus::COMPLETED;
    }
    else
    {
        o.status = OrderStatus::PARTIAL;
    }

    // update positions
    Position &pos = positions[o.symbol]; // default constructs if missing
    if (o.side == OrderSide::BUY)
    {
        // buy increases qty and moves avg_entry
        int prevQty = pos.quantity;
        double prevAvg = pos.avg_entry;
        int newQty = prevQty + fill_qty;
        if (newQty == 0)
        {
            pos.avg_entry = 0;
        }
        else
        {
            pos.avg_entry = (prevAvg * prevQty + fill_price * fill_qty) / newQty;
        }
        pos.quantity = newQty;
    }
    else
    {
        // SELL reduces quantity (assuming long-only for simplicity)
        int prevQty = pos.quantity;
        int newQty = prevQty - fill_qty;
        // realize P&L for the sold part
        double realized = 0.0;
        if (prevQty > 0)
        {
            realized = (fill_price - pos.avg_entry) * min(prevQty, fill_qty);
            pos.realized_pl += realized;
        }
        pos.quantity = newQty;
        if (pos.quantity <= 0)
        {
            pos.quantity = 0;
            pos.avg_entry = 0.0;
        }
    }

    // persist updated order row (append a new row for state change)
    persist_order_row(o);
    // update positions csv snapshot
    write_positions_csv();
}

void ExecutionService::update_order_status(int order_id)
{
    auto it = orders.find(order_id);
    if (it == orders.end())
        return;
    Order &o = it->second;

    if (o.status == OrderStatus::COMPLETED)
        return; // nothing to do

    // Get current LTP
    double ltp = priceService.getPrice(o.symbol);

    // if market order - simulate immediate fill
    if (o.type == OrderType::MARKET)
    {
        apply_fill(o, o.quantity - o.filled_qty, ltp);
        return;
    }

    // LIMIT order logic: BUY fills when order_price >= ltp; SELL fills when order_price <= ltp
    if (o.side == OrderSide::BUY)
    {
        if (o.price >= ltp)
        {
            // simulate probability of full vs partial fill
            // if order price is significantly above LTP, high chance of full fill
            double diff = o.price - ltp;
            double probFull = min(1.0, 0.5 + diff / max(1.0, o.price)); // simple heuristic
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            double r = dis(gen);
            if (r < probFull)
            {
                apply_fill(o, o.quantity - o.filled_qty, ltp);
            }
            else
            {
                // partial fill some random fraction
                int remaining = o.quantity - o.filled_qty;
                std::uniform_int_distribution<> qd(1, max(1, remaining / 2));
                int fillq = qd(gen);
                apply_fill(o, fillq, ltp);
            }
        }
    }
    else
    { // SELL
        if (o.price <= ltp)
        {
            double diff = ltp - o.price;
            double probFull = min(1.0, 0.5 + diff / max(1.0, o.price));
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            double r = dis(gen);
            if (r < probFull)
            {
                apply_fill(o, o.quantity - o.filled_qty, ltp);
            }
            else
            {
                int remaining = o.quantity - o.filled_qty;
                std::uniform_int_distribution<> qd(1, max(1, remaining / 2));
                int fillq = qd(gen);
                apply_fill(o, fillq, ltp);
            }
        }
    }
}

void ExecutionService::chase_order(int order_id, int maxAttempts, int stepMillis)
{
    auto it = orders.find(order_id);
    if (it == orders.end())
        return;
    Order &o = it->second;

    for (int attempt = 0; attempt < maxAttempts && o.status != OrderStatus::COMPLETED; ++attempt)
    {
        // get latest LTP
        double ltp = priceService.getPrice(o.symbol);

        // nudge order price toward LTP
        if (o.side == OrderSide::BUY)
        {
            // raise buy price closer to LTP
            if (o.price < ltp)
            {
                // move half the gap towards LTP
                o.price = o.price + (ltp - o.price) * 0.5;
            }
            else
            {
                // move slightly upward
                o.price = o.price + (ltp - o.price) * 0.1;
            }
        }
        else
        { // SELL
            if (o.price > ltp)
            {
                o.price = o.price - (o.price - ltp) * 0.5;
            }
            else
            {
                o.price = o.price - (o.price - ltp) * 0.1;
            }
        }

        // update modified order in CSV as a new row to show change
        persist_order_row(o);

        // try to fill after adjusting
        update_order_status(order_id);

        if (o.status == OrderStatus::COMPLETED)
            break;

        Sleep(stepMillis);
    }
}

void ExecutionService::update_all_orders()
{
    for (auto &p : orders)
    {
        update_order_status(p.first);
    }
}

void ExecutionService::write_positions_csv(const string &filename)
{
    ofstream f(filename);
    f << "symbol,quantity,avg_entry,realized_pl\n";
    for (auto &kv : positions)
    {
        f << kv.first << "," << kv.second.quantity << "," << kv.second.avg_entry << "," << kv.second.realized_pl << "\n";
    }
    f.close();
}

void ExecutionService::overwrite_orders_csv()
{
    // optional: rewrite full snapshot (not used by default)
    ofstream f("data/orders.csv");
    f << "id,time,symbol,side,type,order_price,quantity,filled_qty,status\n";
    for (auto &kv : orders)
    {
        const Order &o = kv.second;
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&o.created_ts));
        string side = (o.side == OrderSide::BUY) ? "BUY" : "SELL";
        string type = (o.type == OrderType::LIMIT) ? "LIMIT" : "MARKET";
        string status = (o.status == OrderStatus::OPEN) ? "OPEN" : (o.status == OrderStatus::PARTIAL) ? "PARTIAL"
                                                                                                      : "COMPLETED";
        f << o.id << "," << buf << "," << o.symbol << "," << side << "," << type << "," << o.price << "," << o.quantity << "," << o.filled_qty << "," << status << "\n";
    }
    f.close();
}

void ExecutionService::write_dashboard_csv(const string &filename)
{
    ofstream f(filename);
    f << "trading_symbol,quantity,avg_entry,last_price,rpl,urpl,pl\n";

    for (auto &kv : positions)
    {
        const string &symbol = kv.first;
        Position &pos = kv.second;

        double ltp = priceService.getPrice(symbol); // current last traded price
        double urpl = pos.quantity * (ltp - pos.avg_entry);
        double pl = pos.realized_pl + urpl;

        f << symbol << ","
          << pos.quantity << ","
          << pos.avg_entry << ","
          << ltp << ","
          << pos.realized_pl << ","
          << urpl << ","
          << pl << "\n";
    }

    f.close();
}
