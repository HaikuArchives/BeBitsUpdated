#include "BBUWindow.h"
#include "Settings.h"
#include <Button.h>
#include <Roster.h>
#include <TextControl.h>
#include <iostream.h>
#include <stdlib.h>


char *itoa(uint32 i) 
{ 
        char *buf = new char[12];
        char *pos = buf + sizeof(buf) - 1; 
        unsigned int u; 
        int negative = 0; 
        if (i < 0) { 
                negative = 1; 
                u = ((unsigned int)(-(1+i))) + 1; 
        } else { 
                u = i; 
        } 

        *pos = 0; 

        do { 
                *--pos = '0' + (u % 10); 
                u /= 10; 
        } while (u); 

        if (negative) { 
                *--pos = '-'; 
        } 

        return pos; 
} 


BBUWindow::BBUWindow( BMessenger *msngr )
 : BWindow( BRect (100,100,560,200), "BeBits Updated Prefrences", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL , B_NOT_ZOOMABLE | B_NOT_RESIZABLE ),
   poll_rate ( new BTextControl(BRect (10, 40,180,20), "Poll Rate", "Polling Interval (in seconds)", NULL, NULL ) ),
   proxy_serv( new BTextControl(BRect (200,10,400,20), "Proxy"    , "Proxy Server"                 , NULL, NULL ) ),
   proxy_auth( new BTextControl(BRect (200,40,450,20), "Proxy"    , "Proxy Auth"                   , NULL, NULL ) ),
   proxy_port( new BTextControl(BRect (405,10,450,20), "Proxy"    , "Port"                         , NULL, NULL ) )
{
  this->msngr = msngr;
  BView *main_view = new BView( Bounds(), "Main View", B_FOLLOW_ALL, 0); 
  main_view->SetViewColor(216,216,216); 
  AddChild( main_view );

  BString ProxyServ, ProxyAuth; uint32 ProxyPort, PollRate;
  LoadSettings(&ProxyServ,&ProxyAuth,&ProxyPort,&PollRate);
  

  poll_rate->SetDivider(130); poll_rate->SetText( itoa(PollRate)      ); main_view->AddChild( poll_rate );
  proxy_serv->SetDivider(65); proxy_serv->SetText( ProxyServ.String() ); main_view->AddChild(proxy_serv );
  proxy_auth->SetDivider(65); proxy_auth->SetText( ProxyAuth.String() ); main_view->AddChild(proxy_auth );
  proxy_port->SetDivider(20); proxy_port->SetText( itoa(ProxyPort)    ); main_view->AddChild(proxy_port );
  
  main_view->AddChild( new BButton( BRect (10,10,180,20), "Button1", "Configure Sound", new BMessage(SOUND_BUTTON) ) );
  main_view->AddChild( new BButton( BRect (10,70,450,20), "Button2", "Save Settings"  , new BMessage(SAVE_SETTINGS) ) );

  Show();
}

void BBUWindow::MessageReceived(BMessage *msg)
{
  if( msg->what == SOUND_BUTTON )
  { be_roster->Launch("application/x-vnd.Be.SoundsPrefs"); }
  if( msg->what == SAVE_SETTINGS )
  {
    SaveSettings( proxy_serv->Text(), proxy_auth->Text(), atoi(proxy_port->Text()), atoi(poll_rate->Text()) );
    PostMessage(B_QUIT_REQUESTED);
  }
}

bool BBUWindow::QuitRequested()
{
  cout << "Quit Requested" << endl;
  Hide();
  msngr->SendMessage( RELOAD_SETTINGS );
  Lock();
  Quit();
  delete msngr;
  //delete this;
  return true;
}


