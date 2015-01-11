#include "DeskbarView.h"
#include <Beep.h>

int main(void)
{
 add_system_beep_event("BeBits Updated");
 DeskbarView::AddToDeskbar();
}