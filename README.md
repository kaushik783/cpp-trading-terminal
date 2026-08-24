# C++ Trading Terminal

A clean, single-file C++ trading terminal prototype.

## Features

- Simulated market data (random walk)
- Simple order book (bid/ask levels)
- Market orders (Buy / Sell 100 shares)
- Portfolio tracking (cash, position, average price)
- Realized & Unrealized P&L
- Risk Manager:
  - Max order size
  - Max position size
  - Cash check on buy
  - No shorting allowed
- Interactive terminal UI

## How to Compile & Run

```bash
g++ -std=c++20 -o trading_terminal trading_terminal.cpp
./trading_terminal
```

## Commands

| Command | Action              |
|---------|---------------------|
| `b`     | Buy 100 shares      |
| `s`     | Sell 100 shares     |
| `p`     | Show portfolio      |
| `o`     | Show order book     |
| `q`     | Quit                |

## Starting Capital

- Cash: **$100,000**
- Symbol: **AAPL**
- Starting price: **~$200**

## Notes

This is a teaching / prototype project.  
It is **not** a real trading system.

Enjoy experimenting!
