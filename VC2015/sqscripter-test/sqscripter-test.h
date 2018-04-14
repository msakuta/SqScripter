#ifndef SQSCRIPTER_TEST_H
#define SQSCRIPTER_TEST_H

#include <wx/wx.h>
#include <wx/glcanvas.h>

struct ScripterWindow;

extern void(*PrintProc)(ScripterWindow *, const char *);

/// ScripterWindow object handle.  It represents single SqScripter window.
extern ScripterWindow *sw;

class MainFrame : public wxFrame{
	void OnClose(wxCloseEvent&);
public:
	using wxFrame::wxFrame;
	~MainFrame()override;
	void update();

protected:
	DECLARE_EVENT_TABLE()
};


class MyApp : public wxApp
{
	bool OnInit()override;
	int OnExit()override;
	wxGLCanvas * MyGLCanvas;
public:
	MainFrame* mainFrame;
	wxTimer animTimer;
	wxMutex mutex;
	void timer(wxTimerEvent&);
protected:
	DECLARE_EVENT_TABLE()
};


DECLARE_APP(MyApp)

#endif
