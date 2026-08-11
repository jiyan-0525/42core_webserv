#include "signals.hpp"

volatile sig_atomic_t g_running = 1;

void handleSigint(int signum)
{
    (void)signum;
    g_running = 0;
}
