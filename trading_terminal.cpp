// ============================================================
// TRADING TERMINAL - C++ STARTER
// Single-file prototype
// C++20
// ============================================================

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstdlib>  // for std::abs

// ============================================================
// TYPES
// ============================================================

using Price = double;
using Quantity = long long;

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT
};

enum class OrderStatus {
    NEW,
    FILLED,
    CANCELLED,
    REJECTED
};

// ============================================================
// MARKET TICK
// ============================================================

struct MarketTick {
    std::string symbol;

    Price bid = 0.0;
    Price ask = 0.0;

    Quantity bidSize = 0;
    Quantity askSize = 0;

    std::chrono::system_clock::time_point timestamp;
};

// ============================================================
// ORDER
// ============================================================

class Order {

public:

    Order(
        const std::string& symbol,
        Side side,
        OrderType type,
        Quantity quantity,
        Price price = 0.0
    )
        : symbol_(symbol),
          side_(side),
          type_(type),
          quantity_(quantity),
          price_(price)
    {
        static long long nextId = 1;

        id_ = "ORD-" + std::to_string(nextId++);

        status_ = OrderStatus::NEW;
    }

    const std::string& id() const {
        return id_;
    }

    const std::string& symbol() const {
        return symbol_;
    }

    Side side() const {
        return side_;
    }

    OrderType type() const {
        return type_;
    }

    Quantity quantity() const {
        return quantity_;
    }

    Price price() const {
        return price_;
    }

    OrderStatus status() const {
        return status_;
    }

    void fill() {
        status_ = OrderStatus::FILLED;
    }

    void cancel() {
        status_ = OrderStatus::CANCELLED;
    }

private:

    std::string id_;
    std::string symbol_;

    Side side_;
    OrderType type_;

    Quantity quantity_;
    Price price_;

    OrderStatus status_;
};

// ============================================================
// POSITION
// ============================================================

struct Position {

    Quantity quantity = 0;

    Price averagePrice = 0.0;

    double realizedPnL = 0.0;
};

// ============================================================
// PORTFOLIO
// ============================================================

class Portfolio {

public:

    Portfolio()
        : cash_(100000.0)
    {
    }

    void executeTrade(
        const Order& order,
        Price executionPrice
    ) {

        const std::string& symbol = order.symbol();

        Position& position = positions_[symbol];

        Quantity qty = order.quantity();

        if (order.side() == Side::BUY) {

            double cost = executionPrice * qty;

            cash_ -= cost;

            if (position.quantity + qty > 0) {
                position.averagePrice =
                    ((position.averagePrice * position.quantity)
                     + cost)
                    /
                    (position.quantity + qty);
            }

            position.quantity += qty;
        }

        else {  // SELL

            double revenue = executionPrice * qty;

            cash_ += revenue;

            // Calculate realized PnL only when reducing long position
            if (position.quantity > 0) {
                Quantity sellQty = std::min(qty, position.quantity);
                double pnl =
                    (executionPrice - position.averagePrice)
                    * sellQty;

                position.realizedPnL += pnl;
            }

            position.quantity -= qty;

            // Reset average price if flat
            if (position.quantity == 0) {
                position.averagePrice = 0.0;
            }
        }
    }

    Position getPosition(
        const std::string& symbol
    ) const {

        auto it = positions_.find(symbol);

        if (it != positions_.end()) {
            return it->second;
        }

        return Position{};
    }

    double getCash() const {
        return cash_;
    }

    double getUnrealizedPnL(
        const std::string& symbol,
        Price marketPrice
    ) const {

        Position position =
            getPosition(symbol);

        if (position.quantity == 0) {
            return 0.0;
        }

        return
            (marketPrice - position.averagePrice)
            * position.quantity;
    }

    double getTotalPnL(
        const std::string& symbol,
        Price marketPrice
    ) const {

        Position position =
            getPosition(symbol);

        return
            position.realizedPnL
            +
            getUnrealizedPnL(symbol, marketPrice);
    }

private:

    double cash_;

    std::map<std::string, Position> positions_;
};

// ============================================================
// RISK MANAGER
// ============================================================

class RiskManager {

public:

    RiskManager()
        : maxOrderSize_(100),
          maxPosition_(1000)
    {
    }

    bool validate(
        const Order& order,
        const Portfolio& portfolio,
        Price currentPrice
    ) const {

        // Maximum order size
        if (order.quantity() > maxOrderSize_) {

            std::cout
                << "[RISK] Order rejected: "
                << "quantity too large\n";

            return false;
        }

        // Cash check for BUY
        if (order.side() == Side::BUY) {
            double cost = currentPrice * order.quantity();
            if (cost > portfolio.getCash()) {
                std::cout
                    << "[RISK] Order rejected: "
                    << "insufficient cash\n";
                return false;
            }
        }

        // Position limit
        Position position =
            portfolio.getPosition(order.symbol());

        Quantity resultingPosition =
            position.quantity;

        if (order.side() == Side::BUY) {
            resultingPosition += order.quantity();
        }
        else {
            resultingPosition -= order.quantity();
        }

        if (std::abs(resultingPosition) > maxPosition_) {

            std::cout
                << "[RISK] Order rejected: "
                << "position limit exceeded\n";

            return false;
        }

        // Prevent selling more than owned (no shorting for this demo)
        if (order.side() == Side::SELL && resultingPosition < 0) {
            std::cout
                << "[RISK] Order rejected: "
                << "not enough shares to sell\n";
            return false;
        }

        return true;
    }

private:

    Quantity maxOrderSize_;

    Quantity maxPosition_;
};

// ============================================================
// ORDER BOOK
// ============================================================

class OrderBook {

public:

    void updateBid(
        Price price,
        Quantity quantity
    ) {

        bids_[price] = quantity;
    }

    void updateAsk(
        Price price,
        Quantity quantity
    ) {

        asks_[price] = quantity;
    }

    Price bestBid() const {

        if (bids_.empty()) {
            return 0.0;
        }

        return bids_.begin()->first;
    }

    Price bestAsk() const {

        if (asks_.empty()) {
            return 0.0;
        }

        return asks_.begin()->first;
    }

    void display(int levels = 5) const {

        std::cout
            << "\n========== ORDER BOOK ==========\n";

        std::cout
            << std::setw(12)
            << "BID"
            << std::setw(12)
            << "SIZE"
            << "   |   "
            << std::setw(12)
            << "ASK"
            << std::setw(12)
            << "SIZE\n";

        auto bid = bids_.begin();
        auto ask = asks_.begin();

        for (int i = 0; i < levels; ++i) {

            if (bid != bids_.end()) {

                std::cout
                    << std::setw(12)
                    << std::fixed << std::setprecision(2)
                    << bid->first
                    << std::setw(12)
                    << bid->second;

                ++bid;
            }

            else {

                std::cout
                    << std::setw(24)
                    << "";
            }

            std::cout << "   |   ";

            if (ask != asks_.end()) {

                std::cout
                    << std::setw(12)
                    << std::fixed << std::setprecision(2)
                    << ask->first
                    << std::setw(12)
                    << ask->second;

                ++ask;
            }

            std::cout << "\n";
        }

        std::cout
            << "================================\n";
    }

private:

    // Highest bid first
    std::map<
        Price,
        Quantity,
        std::greater<Price>
    > bids_;

    // Lowest ask first
    std::map<
        Price,
        Quantity
    > asks_;
};

// ============================================================
// MARKET DATA FEED
// ============================================================

class MarketDataFeed {

public:

    MarketDataFeed()
        : generator_(std::random_device{}()),
          movement_(-0.5, 0.5)
    {
    }

    MarketTick getTick(
        const std::string& symbol,
        Price previousPrice
    ) {

        Price newPrice =
            previousPrice + movement_(generator_);

        if (newPrice <= 1.0) {
            newPrice = previousPrice;
        }

        MarketTick tick;

        tick.symbol = symbol;

        tick.bid =
            newPrice - 0.05;

        tick.ask =
            newPrice + 0.05;

        tick.bidSize = 100;
        tick.askSize = 100;

        tick.timestamp =
            std::chrono::system_clock::now();

        return tick;
    }

private:

    std::mt19937 generator_;

    std::uniform_real_distribution<double>
        movement_;
};

// ============================================================
// EXECUTION ENGINE
// ============================================================

class ExecutionEngine {

public:

    bool execute(
        Order& order,
        Portfolio& portfolio,
        RiskManager& riskManager,
        Price marketPrice
    ) {

        if (!riskManager.validate(
                order,
                portfolio,
                marketPrice)) {

            return false;
        }

        order.fill();

        portfolio.executeTrade(
            order,
            marketPrice);

        std::cout
            << "[EXECUTION] "
            << order.id()
            << " FILLED @ "
            << std::fixed
            << std::setprecision(2)
            << marketPrice
            << "\n";

        return true;
    }
};

// ============================================================
// TRADING TERMINAL
// ============================================================

class TradingTerminal {

public:

    TradingTerminal()
        : symbol_("AAPL"),
          currentPrice_(200.0)
    {
    }

    void run() {

        bool running = true;

        while (running) {

            updateMarket();

            display();

            std::cout
                << "\nCommand: ";

            std::string command;

            std::cin >> command;

            if (command == "b") {

                buy();
            }

            else if (command == "s") {

                sell();
            }

            else if (command == "p") {

                displayPortfolio();
            }

            else if (command == "o") {

                orderBook_.display();
            }

            else if (command == "q") {

                running = false;
            }

            else {

                std::cout
                    << "Unknown command.\n";
            }
        }

        std::cout
            << "\nTrading terminal closed.\n";
    }

private:

    // --------------------------------------------------------
    // MARKET UPDATE
    // --------------------------------------------------------

    void updateMarket() {

        MarketTick tick =
            marketData_.getTick(
                symbol_,
                currentPrice_);

        currentPrice_ =
            (tick.bid + tick.ask) / 2.0;

        orderBook_.updateBid(
            tick.bid,
            tick.bidSize);

        orderBook_.updateAsk(
            tick.ask,
            tick.askSize);
    }

    // --------------------------------------------------------
    // TERMINAL UI
    // --------------------------------------------------------

    void display() {

        Position position =
            portfolio_.getPosition(symbol_);

        double unrealized =
            portfolio_.getUnrealizedPnL(
                symbol_,
                currentPrice_);

        std::cout
            << "\n\n============================================\n";

        std::cout
            << "          C++ TRADING TERMINAL\n";

        std::cout
            << "============================================\n";

        std::cout
            << "Symbol: "
            << symbol_
            << "\n";

        std::cout
            << "Price:  "
            << std::fixed
            << std::setprecision(2)
            << currentPrice_
            << "\n";

        std::cout
            << "Cash:   $"
            << portfolio_.getCash()
            << "\n";

        std::cout
            << "Position: "
            << position.quantity
            << "\n";

        std::cout
            << "Unrealized P&L: $"
            << unrealized
            << "\n";

        std::cout
            << "--------------------------------------------\n";

        std::cout
            << "[b] Buy 100\n"
            << "[s] Sell 100\n"
            << "[p] Portfolio\n"
            << "[o] Order Book\n"
            << "[q] Quit\n";

        std::cout
            << "============================================\n";
    }

    // --------------------------------------------------------
    // BUY
    // --------------------------------------------------------

    void buy() {

        Order order(
            symbol_,
            Side::BUY,
            OrderType::MARKET,
            100);

        execution_.execute(
            order,
            portfolio_,
            riskManager_,
            currentPrice_);
    }

    // --------------------------------------------------------
    // SELL
    // --------------------------------------------------------

    void sell() {

        Order order(
            symbol_,
            Side::SELL,
            OrderType::MARKET,
            100);

        execution_.execute(
            order,
            portfolio_,
            riskManager_,
            currentPrice_);
    }

    // --------------------------------------------------------
    // PORTFOLIO
    // --------------------------------------------------------

    void displayPortfolio() {

        Position position =
            portfolio_.getPosition(symbol_);

        std::cout
            << "\n========== PORTFOLIO ==========\n";

        std::cout
            << "Symbol: "
            << symbol_
            << "\n";

        std::cout
            << "Position: "
            << position.quantity
            << "\n";

        std::cout
            << "Average Price: $"
            << std::fixed << std::setprecision(2)
            << position.averagePrice
            << "\n";

        std::cout
            << "Realized P&L: $"
            << position.realizedPnL
            << "\n";

        std::cout
            << "Unrealized P&L: $"
            << portfolio_.getUnrealizedPnL(
                   symbol_,
                   currentPrice_)
            << "\n";

        std::cout
            << "Cash: $"
            << portfolio_.getCash()
            << "\n";

        std::cout
            << "================================\n";
    }

private:

    std::string symbol_;

    Price currentPrice_;

    MarketDataFeed marketData_;

    OrderBook orderBook_;

    Portfolio portfolio_;

    RiskManager riskManager_;

    ExecutionEngine execution_;
};

// ============================================================
// MAIN
// ============================================================

int main() {

    std::cout
        << "Starting Trading Terminal...\n";

    TradingTerminal terminal;

    terminal.run();

    return 0;
}
