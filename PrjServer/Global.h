#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

// #define LOG_PACKET_RAW

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#include <Windows.h>

#include <string>
using namespace std;

std::string byteArrayToHexString(const char* byteArray, size_t length);

#include "Socket_Utils.h"