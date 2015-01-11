#ifndef _BBUWINDOW_H
#define _BBUWINDOW_H

#define SOUND_BUTTON          'mSND'
#define SAVE_SETTINGS         'mSST'
#define RELOAD_SETTINGS       'mRLS'

#include <Window.h>

class BBUWindow : public BWindow
{
public:
  BBUWindow( BMessenger * );
  bool QuitRequested();
void MessageReceived(BMessage *);
private:
  BMessenger   *msngr;

  BTextControl *poll_rate;
  BTextControl *proxy_serv;
  BTextControl *proxy_auth;
  BTextControl *proxy_port;
};



#endif
