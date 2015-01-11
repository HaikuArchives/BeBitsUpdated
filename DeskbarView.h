#ifndef _DESKBAR_VIEW_H
#define _DESKBAR_VIEW_H

#include <View.h>

class BBitmap;
class BPopUpMenu;

extern "C" _EXPORT BView *instantiate_deskbar_item();

class DeskbarView : public BView
{
public:

				DeskbarView();
				DeskbarView(BMessage *archive);
				
       void		Draw(BRect rect);
       void     AttachedToWindow();
       void     DetachedFromWindow();

static void		AddToDeskbar();
       void		RemoveFromDeskbar();
       void     MouseDown(BPoint);

       void     MessageReceived(BMessage*);
       void     CheckForUpdates();

       void     Pulse();
	
static			BArchivable *Instantiate(BMessage *data);
status_t		Archive(BMessage *data, bool deep = true) const;
private:
 BBitmap       *Bitmap;
 BPopUpMenu    *menu;
 uint32         count;
 uint32         mod_value;
 bool           new_item;
};


#endif