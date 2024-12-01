#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define DEBUG_MODE
// #define LOG_PACKET_RAW

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#include <Windows.h>

#include <string>
using namespace std;

#include "resource.h"

std::string byteArrayToHexString(const char* byteArray, size_t length);

#include "Socket_Utils.h"