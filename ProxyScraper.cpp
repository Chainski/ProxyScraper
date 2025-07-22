// https://github.com/Chainski/ProxyScraper
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define RESET      "\033[0m"
#define RED        "\033[31m"      /* Red */
#define GREEN      "\033[32m"      /* Green */
#define BLUE       "\033[34m"      /* Blue */
#define LIGHT_BLUE "\033[36m"      /* Light Blue */
#define MAGENTA    "\033[35m"      /* Magenta */
#define CYAN       "\033[36m"      /* Cyan */
#define YELLOW     "\033[33m"      /* Yellow */

void enableANSIColors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

mutex fileMutex;
string SendRequest(const wstring& url) {
    string result;
    HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
    DWORD dwBytesRead = 0;
    DWORD dwSize = 0;
    vector<char> buffer(8192);
    URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        return "";
    }
    wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
    wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    INTERNET_PORT port = urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    hSession = WinHttpOpen(L"Proxy Scraper", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        return "";
    }
    hConnect = WinHttpConnect(hSession, hostName.c_str(), port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            break;
        }
        if (dwSize == 0) {
            break;
        }
        if (!WinHttpReadData(hRequest, buffer.data(), std::min(dwSize, static_cast<DWORD>(buffer.size())), &dwBytesRead)) {
            break;
        }
        if (dwBytesRead == 0) {
            break;
        }
        result.append(buffer.data(), dwBytesRead);
    } while (dwSize > 0);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}
void SaveFile(const string& filename, const string& url) {
    try {
        string content = SendRequest(wstring(url.begin(), url.end()));
        if (!content.empty()) {
            lock_guard<mutex> lock(fileMutex);
            ofstream file(filename, ios::binary | ios::app);
            if (file.is_open()) {
                file.write(content.c_str(), content.size());
                file.close();
            }
        }
    }
    catch (...) {
    }
}

void ProcessUrls(const vector<string>& urls, const string& filename, size_t start, size_t end) {
    for (size_t i = start; i < end && i < urls.size(); ++i) {
        SaveFile(filename, urls[i]);
    }
}
void TWE(const string& text, int delayMilliseconds) {
    for (char c : text) {
        cout << YELLOW << c << RESET << flush;  
        this_thread::sleep_for(chrono::milliseconds(delayMilliseconds));  
    }
    cout << endl;  
}
void printBanner() {
    cout << LIGHT_BLUE << R"(
                    ╔═══╗╔═══╗╔═══╗╔═╗╔═╗╔╗  ╔╗    ╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗╔═══╗
                    ║╔═╗║║╔═╗║║╔═╗║╚╗╚╝╔╝║╚╗╔╝║    ║╔═╗║║╔═╗║║╔═╗║║╔═╗║║╔═╗║║╔══╝║╔═╗║
                    ║╚═╝║║╚═╝║║║ ║║ ╚╗╔╝ ╚╗╚╝╔╝    ║╚══╗║║ ╚╝║╚═╝║║║ ║║║╚═╝║║╚══╗║╚═╝║
                    ║╔══╝║╔╗╔╝║║ ║║ ╔╝╚╗  ╚╗╔╝     ╚══╗║║║ ╔╗║╔╗╔╝║╚═╝║║╔══╝║╔══╝║╔╗╔╝
                    ║║   ║║║╚╗║╚═╝║╔╝╔╗╚╗  ║║      ║╚═╝║║╚═╝║║║║╚╗║╔═╗║║║   ║╚══╗║║║╚╗
                    ╚╝   ╚╝╚═╝╚═══╝╚═╝╚═╝  ╚╝      ╚═══╝╚═══╝╚╝╚═╝╚╝ ╚╝╚╝   ╚═══╝╚╝╚═╝        
                                https://github.com/Chainski/ProxyScraper  
                                   PROTOCOLS: HTTP/S | SOCKS4 | SOCKS5  
    )" << RESET << endl;
}
int main() {
    SetConsoleOutputCP(CP_UTF8);
    enableANSIColors();
    system("mode con: cols=120 lines=50");
    SetConsoleTitleA("Proxy Scraper - Made By: Chainski");
	printBanner(); 
    string line;
    TWE("[+] This program will autoscrape proxies into separate files", 10);
    cout << CYAN << "[+] Scraping Proxies Please Wait . . ." << RESET << endl;
    const size_t numThreads = 4;
    vector<string> http_urls = {
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/online-proxies/txt/proxies-https.txt",
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/online-proxies/txt/proxies-http.txt",
        "https://api.proxyscrape.com/v2/?request=getproxies&protocol=http&timeout=10000&country=all",
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/archive/txt/proxies-https.txt",
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/archive/txt/proxies-http.txt",
        "https://raw.githubusercontent.com/roosterkid/openproxylist/main/HTTPS_RAW.txt",
        "https://raw.githubusercontent.com/monosans/proxy-list/main/proxies/http.txt",
        "https://raw.githubusercontent.com/TheSpeedX/PROXY-List/master/http.txt",
        "https://www.proxy-list.download/api/v1/get?type=http",
        "https://www.proxy-list.download/api/v1/get?type=https",
        "https://api.openproxylist.xyz/http.txt",
        "https://raw.githubusercontent.com/mmpx12/proxy-list/master/http.txt",
        "https://raw.githubusercontent.com/ShiftyTR/Proxy-List/master/http.txt",
        "https://api.proxyscrape.com/v2/?request=getproxies&protocol=http&timeout=10000&country=all&ssl=all&anonymity=all",
        "https://raw.githubusercontent.com/proxy4parsing/proxy-list/main/http.txt"
    };
    vector<thread> threads;
    size_t urlsPerThread = max(size_t(1), http_urls.size() / numThreads);
    for (size_t i = 0; i < http_urls.size(); i += urlsPerThread) {
        size_t end = min(i + urlsPerThread, http_urls.size());
        threads.emplace_back(ProcessUrls, cref(http_urls), "http.txt", i, end);
    }
    for (auto& t : threads) t.join();
    threads.clear();
    cout << RED << "[!] Successfully Scraped And Saved HTTP Proxies!" << RESET << endl;
    this_thread::sleep_for(chrono::seconds(1));
    vector<string> socks4_urls = {
        "https://api.proxyscrape.com/v2/?request=getproxies&protocol=socks4&timeout=10000&country=all",
        "https://raw.githubusercontent.com/roosterkid/openproxylist/main/SOCKS4_RAW.txt",
        "https://raw.githubusercontent.com/monosans/proxy-list/main/proxies/socks4.txt",
        "https://raw.githubusercontent.com/TheSpeedX/PROXY-List/master/socks4.txt",
        "https://www.proxy-list.download/api/v1/get?type=socks4",
        "https://api.openproxylist.xyz/socks4.txt",
        "https://raw.githubusercontent.com/mmpx12/proxy-list/master/socks4.txt",
        "https://raw.githubusercontent.com/ShiftyTR/Proxy-List/master/socks4.txt",
        "https://raw.githubusercontent.com/rdavydov/proxy-list/main/proxies/socks4.txt",
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/online-proxies/txt/proxies-socks4.txt"
    };
    urlsPerThread = max(size_t(1), socks4_urls.size() / numThreads);
    for (size_t i = 0; i < socks4_urls.size(); i += urlsPerThread) {
        size_t end = min(i + urlsPerThread, socks4_urls.size());
        threads.emplace_back(ProcessUrls, cref(socks4_urls), "socks4.txt", i, end);
    }
    for (auto& t : threads) t.join();
    threads.clear();
    cout << LIGHT_BLUE << "[!] Successfully Scraped And Saved SOCKS4 Proxies!" << RESET << endl;
    this_thread::sleep_for(chrono::seconds(1));
    vector<string> socks5_urls = {
        "https://api.proxyscrape.com/v2/?request=getproxies&protocol=socks5&timeout=10000&country=all",
        "https://raw.githubusercontent.com/roosterkid/openproxylist/main/SOCKS5_RAW.txt",
        "https://raw.githubusercontent.com/monosans/proxy-list/main/proxies/socks5.txt",
        "https://raw.githubusercontent.com/TheSpeedX/PROXY-List/master/socks5.txt",
        "https://raw.githubusercontent.com/hookzof/socks5_list/master/proxy.txt",
        "https://www.proxy-list.download/api/v1/get?type=socks5",
        "https://api.openproxylist.xyz/socks5.txt",
        "https://raw.githubusercontent.com/mmpx12/proxy-list/master/socks5.txt",
        "https://raw.githubusercontent.com/ShiftyTR/Proxy-List/master/socks5.txt",
        "https://raw.githubusercontent.com/jetkai/proxy-list/main/online-proxies/txt/proxies-socks5.txt"
    };
    urlsPerThread = max(size_t(1), socks5_urls.size() / numThreads);
    for (size_t i = 0; i < socks5_urls.size(); i += urlsPerThread) {
        size_t end = min(i + urlsPerThread, socks5_urls.size());
        threads.emplace_back(ProcessUrls, cref(socks5_urls), "socks5.txt", i, end);
    }
    for (auto& t : threads) t.join();
    threads.clear();
    cout << RED << "[!] Successfully Scraped And Saved SOCKS5 Proxies!" << RESET << endl;
    cout << GREEN << "Press any key to continue . . .";
    cin.get();
    return 0;
}