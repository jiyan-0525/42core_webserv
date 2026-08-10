#!/usr/bin/env python3
with open("sample.txt", "r", encoding="utf-8") as f:
    value = f.read().strip()

print("Content-Type: text/plain")
print()
print("relative_file=" + value)
