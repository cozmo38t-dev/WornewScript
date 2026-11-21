#ifndef WS_LIB_MONEY_C
#define WS_LIB_MONEY_C
/* money.c — money */
static double ws_c_discount(double price, double pct) { return price * (1.0 - pct / 100.0); }
static double ws_c_vat(double price, double pct) { return price * (1.0 + pct / 100.0); }
#endif
