#!/usr/bin/env python3
import os
import sys

# Get the length of the query body from the environmental variables
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

# get POST-data form stdin
post_data = ""
if content_length > 0:
    post_data = sys.stdin.read(content_length)

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print("<p>Method: " + os.environ.get("REQUEST_METHOD", "?") + "</p>")
print("<p>Query string: " + os.environ.get("QUERY_STRING", "?") + "</p>")
print("<p>POST Body: " + post_data + "</p>")
print("</body></html>")
