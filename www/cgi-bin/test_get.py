#!/usr/bin/env python3
import os
import urllib.parse

query = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query)

print("Content-Type: text/plain")
print()
print("method=GET")
print("query=" + query)
print("name=" + ",".join(params.get("name", [""])))
