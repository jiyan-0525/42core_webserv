#!/usr/bin/env python3
import datetime
print("Content-Type: text/plain\n")
now = datetime.datetime.now()
print("Current date and time: ", now.strftime("%Y-%m-%d %H:%M:%S"))
