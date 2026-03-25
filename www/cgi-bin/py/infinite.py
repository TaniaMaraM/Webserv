#!/usr/bin/env python3
# CGI script with infinite loop - tests server timeout handling

import time

print("Content-Type: text/html\r")
print("\r")
print("<h1>Starting infinite loop...</h1>")

# Infinite loop - server should timeout
while True:
    pass
