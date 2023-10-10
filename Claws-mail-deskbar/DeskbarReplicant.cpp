/*
 * Copyright 2009-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Weirauch, dev@m-phasis.de
 *		Humdinger, humdingerb@gmail.com
 * Adapted from Clipdinger by Nexus6 in 2023
 */

#include "DeskbarReplicant.h"
#include "BeDC.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"


extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth, float maxHeight);
status_t our_image(image_info& image);
const char* kClassName = "DeskbarReplicant";


DeskbarReplicant::DeskbarReplicant(BRect frame, int32 resizingMode)
	:
	BView(
		frame, kApplicationName, resizingMode, B_WILL_DRAW | B_TRANSPARENT_BACKGROUND | 
		B_FRAME_EVENTS | B_PULSE_NEEDED),
	fAppMessenger(BMessenger(kApplicationSignature))
{
	_Init();
}


DeskbarReplicant::DeskbarReplicant(BMessage* archive)
	:
	BView(archive),
	fAppMessenger(BMessenger(kApplicationSignature))
{
	_Init();
}


DeskbarReplicant::~DeskbarReplicant()
{
}


void
DeskbarReplicant::_Init()
{
	fIcon = NULL;

	image_info info;
	if (our_image(info) != B_OK)
		return;

	BFile file(info.name, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	BResources resources(&file);
	if (resources.InitCheck() < B_OK)
		return;

	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, "tray_icon", &size);
	if (data != NULL) {
		BBitmap* icon = new BBitmap(Bounds(), B_RGBA32);
		if (icon->InitCheck() == B_OK
			&& BIconUtils::GetVectorIcon((const uint8*)data, size, icon) == B_OK)
			fIcon = icon;
		else
			delete icon;
	}
}


DeskbarReplicant*
DeskbarReplicant::Instantiate(BMessage* archive)
{
	if (!be_roster->IsRunning(kApplicationSignature))
		return NULL;

	if (!validate_instantiation(archive, kClassName))
		return NULL;

	return new DeskbarReplicant(archive);
}


status_t
DeskbarReplicant::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kApplicationSignature);
	if (status == B_OK)
		status = archive->AddString("class", kClassName);

	return status;
}


void
DeskbarReplicant::AttachedToWindow()
{
	BView::AttachedToWindow();
	AdoptParentColors();

	if (ViewUIColor() == B_NO_COLOR)
		SetLowColor(ViewColor());
	else
		SetLowUIColor(ViewUIColor());
		
	if (LockLooper()) {
		StartWatching(fAppMessenger, UPDATE_STATISTICS);
		UnlockLooper();
	}
}

void
DeskbarReplicant::DetachedFromWindow()
{
	if (LockLooper()) {
		StopWatching(fAppMessenger, UPDATE_STATISTICS);
		UnlockLooper();
	}

	BView::DetachedFromWindow();
}

void
DeskbarReplicant::Draw(BRect updateRect)
{
	BeDC dc(kApplicationName);

	if (!fIcon) {
		/* At least display something... */
		rgb_color lowColor = LowColor();
		SetLowColor(0, 113, 187, 255);
		FillRoundRect(Bounds().InsetBySelf(3.f, 0.f), 5.f, 7.f, B_SOLID_LOW);
		SetLowColor(lowColor);
	} else {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon);
		SetDrawingMode(B_OP_COPY);
	}

	// calculate the width of the string (and truncate it if needed), the rounded rectangle bounds
	// and their position
	BString badgeText;
	badgeText.SetToFormat("%d", fUnreadMessages);
	BFont font(be_bold_font);
	font.SetSize(be_bold_font->Size()*0.6f);
	auto maxWidth = font.StringWidth("9999");
	TruncateString(&badgeText, B_TRUNCATE_END, maxWidth);
	auto width = font.StringWidth(badgeText);
	// Size() should be enough to calculate the height, we allow only numbers
	font_height fh;
	font.GetHeight(&fh);
	auto height = fh.ascent + fh.descent;
	#ifdef DCLOG
	dc.SendFormat("badgeText size: width = %f, height = %f", width, height);
	#endif
	
	rgb_color lowColor = LowColor();
	SetLowColor(200, 0, 0, 0);
	// calculate the red rounded rect based on the badge text size and adds 
	// a reasonable margin in percentage
	float hMargin = 0.06f, vMargin = 0.06f;
	BRect roundedRect(Bounds().right - width, Bounds().bottom - height, Bounds().right, Bounds().bottom);
	#ifdef DCLOG
	dc.SendFormat("roundedRect: top = %f, left = %f, right = %f, bottom = %f, width = %f, height = %f", 
				roundedRect.top, roundedRect.left, roundedRect.right, roundedRect.bottom, 
				roundedRect.Width(), roundedRect.Height());
	#endif
	roundedRect.InsetBySelf(roundedRect.Width() * -hMargin, roundedRect.Height() * -vMargin);
	#ifdef DCLOG
	dc.SendFormat("roundedRect: top = %f, left = %f, right = %f, bottom = %f, width = %f, height = %f", 
				roundedRect.top, roundedRect.left, roundedRect.right, roundedRect.bottom, 
				roundedRect.Width(), roundedRect.Height());
	#endif
	FillRoundRect(roundedRect, 5.0f, 6.0f, B_SOLID_LOW);
	SetLowColor(lowColor);
	
	auto x = roundedRect.left + ((roundedRect.Width() - width) / 2);
	auto y = roundedRect.bottom - ((roundedRect.Height() - height) / 2);
	BPoint textPoint(x, y);
	#ifdef DCLOG
	dc.SendFormat("textPoint: x = %f, y = %f", textPoint.x, textPoint.y);
	#endif
	MovePenTo(textPoint.x, textPoint.y - fh.descent);
	rgb_color highColor = HighColor();
	SetHighColor(255, 255, 255, 0);
	SetFont(&font);
	
	DrawString(badgeText);
	SetHighColor(highColor);
	
	BString tooltip;
	tooltip.SetToFormat("Unread = %d, Total = %d", fUnreadMessages, fTotalMessages);
	SetToolTip(tooltip);
}


void
DeskbarReplicant::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_QUIT_REQUESTED:
		{
			// we ask Claws to quit before dispatching the quit request to the Deskbar replicant app
			_QuitApplicationAndWait(kClawsMailSignature);
			_ForwardMessage(kApplicationSignature, msg);
		} 
		break;
		case B_ABOUT_REQUESTED:
		{
			_LaunchApplicationAndWait(kApplicationSignature);
			_ForwardMessage(kApplicationSignature, msg);
		}
		break;
		case OPEN_CLAWS:
		{
			_LaunchApplicationAndWait(kClawsMailSignature);
		}
		break;
		case COMPOSE_MESSAGE:
		{
			_ForwardMessage(kClawsMailSignature, msg);
		}
		break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {			
				case UPDATE_STATISTICS:
				{
					fTotalMessages = msg->GetUInt16("total", -1);
					fUnreadMessages = msg->GetUInt16("unread", -1);
					fUnreadMarkedMessages = msg->GetUInt16("unread_marked", -1);
					fMarkedMessages = msg->GetUInt16("marked", -1);
					fNewMessages = msg->GetUInt16("new", -1);
					Invalidate();
					#ifdef DCLOG
					BeDC(kApplicationName).SendFormat("New %d, Unread: %d, Total: %d", fNewMessages, fUnreadMessages,	fTotalMessages);
					#endif
				}
				break;
				default:
					break;
			}
		}
		break;
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void 
DeskbarReplicant::Pulse()
{
	BMessenger beam(kClawsMailSignature);
	if (!beam.IsValid()) {
		// Claws probably crashed so we quit, too:
		BDeskbar().RemoveItem(kApplicationName);
	}
}


void
DeskbarReplicant::_QuitApplicationAndWait(const char* signature)
{
	team_id team;
	auto message = new BMessage(B_QUIT_REQUESTED);
	_ForwardMessage(signature, message);
	while ((team = be_roster->TeamFor(signature)) > 0) {
		snooze(1000000);
		_ForwardMessage(signature, message);
	}
}


void
DeskbarReplicant::_LaunchApplicationAndWait(const char* signature)
{
	team_id team;
	team = be_roster->TeamFor(signature);
	if (team < 0) {
		be_roster->Launch(signature);
		while (be_roster->TeamFor(signature) < 0)
			snooze(100000);
	}
}

void
DeskbarReplicant::_ForwardMessage(const char* signature, BMessage *message)
{
	team_id team;
	team = be_roster->TeamFor(signature);
	BMessenger messenger(signature, team);
	if (messenger.IsValid())
		messenger.SendMessage(message);
}

		
void
DeskbarReplicant::MouseDown(BPoint where)
{
	BPoint point;
	uint32 buttons;
	GetMouse(&point, &buttons);

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {

		BPopUpMenu* menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);

		menu->AddItem(new BMenuItem(B_TRANSLATE("Open Claws Mail"), new BMessage(OPEN_CLAWS)));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Compose new message"), new BMessage(COMPOSE_MESSAGE)));
		
		menu->AddSeparatorItem();
		
		menu->AddItem(new BMenuItem(B_TRANSLATE("About..."), new BMessage(B_ABOUT_REQUESTED)));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Quit Claws Mail"), new BMessage(B_QUIT_REQUESTED)));
									
		menu->SetTargetForItems(this);
		ConvertToScreen(&point);
		menu->Go(point, true, true, BRect(where - BPoint(4, 4), point + BPoint(4, 4)));
		delete menu;
	} else if (buttons & B_PRIMARY_MOUSE_BUTTON)
		BMessenger(this).SendMessage(OPEN_CLAWS);
}


extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	return new DeskbarReplicant(BRect(0, 0, maxHeight - 1, maxHeight - 1), B_FOLLOW_NONE);
}


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char*)our_image >= (char*)image.text
			&& (char*)our_image <= (char*)image.text + image.text_size)
			return B_OK;
	}
	BAlert* alert = new BAlert("image", "Image NOT OK", "NOT");
	alert->Show();
	return B_ERROR;
}