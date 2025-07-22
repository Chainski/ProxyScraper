@echo off
cd /d "%~dp0"
windres assets\ProxyScraper.rc -O coff -o assets\ProxyScraper.o
g++ ProxyScraper.cpp assets\ProxyScraper.o -w -O2 -static -lpsapi -lwinhttp -lws2_32 -fexceptions -Wl,--gc-sections -pipe -s -o ProxyScraper.exe