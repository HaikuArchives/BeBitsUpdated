#include "Globals.h"
#include "DeskbarView.h"
#include "BBUWindow.h"
#include "Settings.h"

#include <E-mail.h>
#include <Beep.h>
#include <String.h>
#include <Entry.h>
#include <Bitmap.h>
#include <Roster.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Deskbar.h>
#include <iostream.h>
#include <NetEndpoint.h>
#include <Messenger.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 RetrieveFromBeBits(void *msngr)
{
  BString ProxyServ, ProxyAuth; uint32 ProxyPort, PollRate;
  LoadSettings(&ProxyServ,&ProxyAuth,&ProxyPort,&PollRate);

  BString reply,Software;
  BMessage *msg = NULL;
  BNetEndpoint endpoint;

  if( 0 != ProxyServ.Compare(NULL) )
  {

    BString Request;
    Request << "GET http://www.bebits.com/backend/recent HTTP/1.1\n";
    if( 0 != ProxyAuth.Compare(NULL) )
    {
      char auth[100];
      encode_base64( (char*)&auth, (char*)ProxyAuth.String(),ProxyAuth.CountChars() );
      Request << "Proxy-Authorization: Basic " << auth << "\n";
    }
    Request << "Host: www.bebits.com\n\n";
    cout << Request.String() << endl;
    if( B_OK != endpoint.Connect(ProxyServ.String(),ProxyPort) )
      return B_ERROR;   
    endpoint.Send(Request.String(), Request.CountChars() );
  } else {
    if( B_OK != endpoint.Connect("www.bebits.com",80) )
      return B_ERROR;   
    endpoint.Send("GET /backend/recent HTTP/1.1\nHost: www.bebits.com\n\n", 51 );
  }

  reply.UnlockBuffer( endpoint.Receive( reply.LockBuffer(BUFFER_SIZE), BUFFER_SIZE ) );
  int32 offset1 = reply.FindLast("%%"); reply.RemoveLast("%%");
  int32 offset2 = reply.FindLast("%%");
  while( offset1 > 0 && offset2 > 0 )
  {
    reply.MoveInto(Software, offset2 + 2, offset1 - offset2 - 2  );
    msg = new BMessage(BEBITS_UPDATE);
    BString name,url;
    Software.RemoveFirst("\n");
    Software.ReplaceFirst("\n"," - ");
    Software.MoveInto(name, 0, Software.FindFirst("\n") ); 
    Software.RemoveFirst("\n");
    Software.MoveInto(url    , 0, Software.FindFirst("\n") ); 
    msg->AddString("name"   ,name   );
    msg->AddString("be:url" ,url    );
    (*(BMessenger*)msngr).SendMessage(msg,msg);
    delete msg;
    offset1 = reply.FindLast("%%"); reply.RemoveLast("%%");
    offset2 = reply.FindLast("%%");
  }
  delete (BMessenger*)msngr;
  endpoint.Close();
  return B_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BBitmap *get_resource_bitmap()
{
  entry_ref	ref;
  BBitmap *bmp = new	BBitmap(BRect(0,0, B_MINI_ICON-1, B_MINI_ICON-1), B_CMAP8);	
  if (be_roster->FindApp(GLOBAL_APP_SIG, &ref)!=B_OK) return NULL;
  BNode      file( &ref );
  BNodeInfo  *info = new BNodeInfo(&file);
  info->GetIcon(bmp, B_MINI_ICON);
  delete info;
  return bmp;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BView *instantiate_deskbar_item()
{ return new DeskbarView(); }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DeskbarView::DeskbarView()
    : BView( BRect(0, 0, 15, 15), VIEW_NAME, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW | B_PULSE_NEEDED)
{}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DeskbarView::DeskbarView(BMessage *archive)
	: BView(archive),
	  menu( new BPopUpMenu("Menu",false,false) ),
	  count( 0 ),
	  mod_value( 600 ),
	  new_item (false)
{}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::AttachedToWindow(void) 
{
    LoadSettings(NULL,NULL,NULL,&mod_value);


	SetViewColor(Parent()->ViewColor());
	SetDrawingMode( B_OP_ALPHA );
	Bitmap = get_resource_bitmap();
	BMessage *menu_msg2 = new BMessage(GOTO_URL);
	BMessage *menu_msg3 = new BMessage(CHECK_NOW);
	BMessage *menu_msg4 = new BMessage(CONFIGURE);
	menu_msg2->AddString("be:url","http://www.bebits.com");
    BMenu *options = new BMenu( "Options" );
    options->SetFontSize(11);
	BMenuItem *item1 = new BMenuItem( "Configure"       , menu_msg4 );
	BMenuItem *item2 = new BMenuItem( "Check Now"       , menu_msg3 );
	BMenuItem *item3 = new BMenuItem( "GoTo BeBits.com" , menu_msg2 );
	BMenuItem *item4 = new BMenuItem( "Quit"            , new BMessage(B_QUIT_REQUESTED) );
	item1->SetTarget(this);
    item2->SetTarget(this);
    item3->SetTarget(this);
    item4->SetTarget(this);
    menu->SetFontSize(11);
    options->AddItem( item1 );
    options->AddItem( item2 );
    options->AddItem( item3 );
    options->AddSeparatorItem();
    options->AddItem( item4 );
    menu->AddItem( options );
    menu->AddSeparatorItem();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::DetachedFromWindow(void) 
{
  delete Bitmap;
  delete menu;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::Pulse()
{
  if( new_item ) { SetDrawingMode(B_OP_INVERT); DrawBitmap( Bitmap ); }
  if( count++ % mod_value == 0  )
  { CheckForUpdates(); }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::Draw(BRect rect)
{
  SetDrawingMode( B_OP_ALPHA );
  DrawBitmap( Bitmap );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::AddToDeskbar()
{
	BDeskbar deskbar;
	entry_ref ref;
	be_roster->FindApp(GLOBAL_APP_SIG, &ref);
	deskbar.AddItem(&ref);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::RemoveFromDeskbar()
{
	BDeskbar deskbar;
	deskbar.RemoveItem(VIEW_NAME);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::MouseDown(BPoint where)
{
  BPoint cursor; uint32 buttons;
  GetMouse(&cursor, &buttons);
  if(B_SECONDARY_MOUSE_BUTTON == buttons)
  {
    BRect bounds = Bounds();
    where = ConvertToScreen(where);
    bounds = ConvertToScreen(bounds);
    menu->Go(where,true,true,bounds);
  }
  if(B_TERTIARY_MOUSE_BUTTON  == buttons)
  { CheckForUpdates(); }
  if( new_item ) { new_item = false; Invalidate(); }  
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BArchivable *DeskbarView::Instantiate(BMessage *data)
{ return new DeskbarView(data); }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
status_t DeskbarView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddString("add_on", GLOBAL_APP_SIG);
	data->AddString("class", VIEW_NAME);
	return B_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::MessageReceived(BMessage *msg)
{
  switch(msg->what)
  {
    case BEBITS_UPDATE:
    {
      BString name, url;
      msg->FindString("name"   , &name   );
      msg->FindString("be:url" , &url    );
      if( NULL == menu->FindItem(name.String() ) )
      {
        system_beep("BeBits Updated");
        new_item = true;
        BMessage *menu_msg = new BMessage();
        *menu_msg = *msg; menu_msg->what = GOTO_URL;
        BMenuItem *item = new BMenuItem( name.String(), menu_msg );
        item->SetTarget( this );    
        menu->AddItem( item, 2 );
        while( menu->CountItems() > LIST_ITEM_COUNT + 2 )
        { delete menu->RemoveItem(LIST_ITEM_COUNT + 2 - 1); }
      }
    }
    break;
    case GOTO_URL:
    {
      BMessage *menu_msg = new BMessage();
      *menu_msg = *msg; menu_msg->what = BROWSER_OPEN_URL;
      be_roster->Launch(BROWSER_APP_SIGNATURE, menu_msg);
    }
    break;
    case CHECK_NOW:
      CheckForUpdates();
    break;
    case RELOAD_SETTINGS:
      LoadSettings(NULL,NULL,NULL,&mod_value); 
    break;
    case CONFIGURE:
      new BBUWindow(new BMessenger(this));
    break;
    case B_QUIT_REQUESTED:
      RemoveFromDeskbar();
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeskbarView::CheckForUpdates()
{ resume_thread( spawn_thread(RetrieveFromBeBits, "Checking BeBits for updates..", B_NORMAL_PRIORITY, new BMessenger(this) ) ); }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
