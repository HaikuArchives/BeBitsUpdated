#ifndef _SETTINGS_H
#define _SETTINGS_H


#include <String.h>
#include "IniFile/BIniFile.h"


void LoadSettings(BString *proxy_serv, BString *proxy_auth, uint32 *proxy_port, uint32 *poll_rate);
void SaveSettings(const char *proxy_serv, const char *proxy_auth, uint32 proxy_port, uint32 poll_rate);

#endif