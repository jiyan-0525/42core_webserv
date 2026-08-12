#ifndef SIGNALS_HPP
# define SIGNALS_HPP

# include <csignal>

extern volatile sig_atomic_t g_running;

void handleSignal(int signum);

#endif