#!/usr/bin/env python3
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.read(length) if length > 0 else ""

print("Content-Type: text/plain")
print()
print("method=POST")
print("length=" + str(length))
print("body=" + body)
