# Void Overlay v0.2.1
## features
* ratelimit handling
* keyless stats lookup (*1)
* supports antisniper api (getting estimate winstreak)
* autowho without controlling keyboards (checkout *2)
* music player

## (*1) how keyless lookup working?
I created an hypixel api proxy on https://hypixel.voids.top \
It contains 8 accounts and key refreshing automated. \
Everyone can use this but global ratelimit is `300(requests) * 8(accounts)` in each 5 minutes. \
 (2025-12-04) I changed the inside proxy to datacenter proxies. You can lookup more faster. and fixed bugs.

## (*2) about "ext/" 
It contains an injector, so it may be considered cheating. I can't do anything if someone gets punished for using it. \
It connects to 46001/tcp on your localhost. \
You can send chat or get data with tcp connection.
