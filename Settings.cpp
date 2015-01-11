#include "Settings.h"

void LoadSettings(BString *proxy_serv, BString *proxy_auth, uint32 *proxy_port, uint32 *poll_rate)
{
  BIniFile ini;
  ini.Load("/boot/home/config/settings/SlimSOFT");
  if(poll_rate  != NULL) *poll_rate  =      ini.ReadInt   ("BeBitsUpdated","PollInterval", 600 )  ;
  if(proxy_port != NULL) *proxy_port =      ini.ReadInt   ("BeBitsUpdated","ProxyPort"   ,  80 )  ;
  if(proxy_serv != NULL) proxy_serv->SetTo( ini.ReadString("BeBitsUpdated","ProxyServer" , ""  ) );
  if(proxy_auth != NULL) proxy_auth->SetTo( ini.ReadString("BeBitsUpdated","ProxyAuth"   , ""  ) );
}

void SaveSettings(const char *proxy_serv, const char *proxy_auth, uint32 proxy_port, uint32 poll_rate)
{
  BIniFile ini;
  ini.Load("/boot/home/config/settings/SlimSOFT");
  if(poll_rate > 0 && poll_rate <= 0xFFFF) ini.WriteInt   ("BeBitsUpdated","PollInterval", poll_rate  );
  if(poll_rate > 0 && poll_rate <= 0xFFFF) ini.WriteInt   ("BeBitsUpdated","ProxyPort"   , proxy_port );
  ini.WriteString("BeBitsUpdated","ProxyServer" , proxy_serv );
  ini.WriteString("BeBitsUpdated","ProxyAuth"   , proxy_auth );
  ini.Store("/boot/home/config/settings/SlimSOFT");
}