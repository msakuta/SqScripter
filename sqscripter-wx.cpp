// SqScripter scripting widget Program

#include "sqscripter.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/stc/stc.h>
#include <wx/splitter.h>
#include "wx/dynlib.h"

#include <process.h> // for _beginthreadex()


struct ScripterWindowImpl : ScripterWindow{
};


class AddErrorEvent : public wxThreadEvent{
public:
	wxString desc;
	wxString source;
	int line;
	int column;

	AddErrorEvent(wxEventType type, int id, const char *desc, const char *source, int line, int column) : wxThreadEvent(type, id),
		desc(desc), source(source), line(line), column(column)
	{
	}
};

class SqScripterFrame;

class SqScripterApp: public wxApp
{
public:
	SqScripterApp() : handle(NULL){}
	virtual bool OnInit();
	void OnShowWindow(wxThreadEvent& event);
	void OnTerminate(wxThreadEvent&);
	void OnSetLexer(wxThreadEvent&);
	void OnAddError(wxThreadEvent&);
	void OnClearError(wxThreadEvent&);
	void OnPrint(wxThreadEvent&);

	SqScripterFrame *frame;
	ScripterWindowImpl *handle;
};


class SqScripterFrame: public wxFrame
{
public:
	SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~SqScripterFrame()override;
private:
	void OnRun(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnClear(wxCommandEvent& event);
	void OnEnterCmd(wxCommandEvent&);
	wxDECLARE_EVENT_TABLE();

	void SetLexer();
	void AddError(AddErrorEvent&);
	void ClearError();
	void RecalcLineNumberWidth();
	void SaveScriptFile(const char *fileName);
	void LoadScriptFile(wxString fileName);
	void UpdateTitle();
	void SetFileName(wxString fileName, bool dirty = false);

	wxStyledTextCtrl *stc;
	wxTextCtrl *log;
	wxLog *logger;
	wxTextCtrl *cmd;
	wxString fileName;
	bool dirty;

	friend class SqScripterApp;
};



enum
{
	ID_Run = 1,
	ID_Open,
	ID_Save,
	ID_Clear,
	ID_Command
};


wxBEGIN_EVENT_TABLE(SqScripterFrame, wxFrame)
EVT_MENU(ID_Run,   SqScripterFrame::OnRun)
EVT_MENU(ID_Open,  SqScripterFrame::OnOpen)
EVT_MENU(ID_Save,  SqScripterFrame::OnSave)
EVT_MENU(ID_Clear,  SqScripterFrame::OnClear)
EVT_MENU(wxID_EXIT,  SqScripterFrame::OnExit)
EVT_MENU(wxID_ABOUT, SqScripterFrame::OnAbout)
EVT_TEXT_ENTER(ID_Command, SqScripterFrame::OnEnterCmd)
wxEND_EVENT_TABLE()




#ifdef _DLL
wxIMPLEMENT_APP_NO_MAIN(SqScripterApp);

static const int CMD_SHOW_WINDOW = wxNewId();
static const int CMD_TERMINATE = wxNewId();
static const int CMD_SETLEXER = wxNewId();
static const int CMD_ADDERROR = wxNewId();
static const int CMD_CLEARERROR = wxNewId();
static const int CMD_PRINT = wxNewId();


// Critical section that guards everything related to wxWidgets "main" thread
// startup or shutdown.
wxCriticalSection gs_wxStartupCS;
// Handle of wx "main" thread if running, NULL otherwise
HANDLE gs_wxMainThread = NULL;

//  wx application startup code -- runs from its own thread
unsigned wxSTDCALL MyAppLauncher(void* event)
{
	// Note: The thread that called run_wx_gui_from_dll() holds gs_wxStartupCS
	//       at this point and won't release it until we signal it.

	// We need to pass correct HINSTANCE to wxEntry() and the right value is
	// HINSTANCE of this DLL, not of the main .exe, use this MSW-specific wx
	// function to get it. Notice that under Windows XP and later the name is
	// not needed/used as we retrieve the DLL handle from an address inside it
	// but you do need to use the correct name for this code to work with older
	// systems as well.
	const HINSTANCE
		hInstance = wxDynamicLibrary::MSWGetModuleHandle("my_dll",
			&gs_wxMainThread);
	if ( !hInstance )
		return 0; // failed to get DLL's handle

				  // wxIMPLEMENT_WXWIN_MAIN does this as the first thing
	wxDISABLE_DEBUG_SUPPORT();

	// We do this before wxEntry() explicitly, even though wxEntry() would
	// do it too, so that we know when wx is initialized and can signal
	// run_wx_gui_from_dll() about it *before* starting the event loop.
	wxInitializer wxinit;
	if ( !wxinit.IsOk() )
		return 0; // failed to init wx

				  // Signal run_wx_gui_from_dll() that it can continue
	HANDLE hEvent = *(static_cast<HANDLE*>(event));
	if ( !SetEvent(hEvent) )
		return 0; // failed setting up the mutex

				  // Run the app:
	wxEntry(hInstance);

	return 1;
}

static void PrintProc(ScripterWindow *, const char *text){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_PRINT);
	event->SetString(text);
	wxQueueEvent(wxApp::GetInstance(), event);
}

ScripterWindow *scripter_init(const ScripterConfig *sc){
	// In order to prevent conflicts with hosting app's event loop, we
	// launch wx app from the DLL in its own thread.
	//
	// We can't even use wxInitializer: it initializes wxModules and one of
	// the modules it handles is wxThread's private module that remembers
	// ID of the main thread. But we need to fool wxWidgets into thinking that
	// the thread we are about to create now is the main thread, not the one
	// from which this function is called.
	//
	// Note that we cannot use wxThread here, because the wx library wasn't
	// initialized yet. wxCriticalSection is safe to use, though.

	wxCriticalSectionLocker lock(gs_wxStartupCS);

	if ( !gs_wxMainThread )
	{
		HANDLE hEvent = CreateEvent
			(
				NULL,  // default security attributes
				FALSE, // auto-reset
				FALSE, // initially non-signaled
				NULL   // anonymous
				);
		if ( !hEvent )
			return NULL; // error

					// NB: If your compiler doesn't have _beginthreadex(), use CreateThread()
		gs_wxMainThread = (HANDLE)_beginthreadex
			(
				NULL,           // default security
				0,              // default stack size
				&MyAppLauncher,
				&hEvent,        // arguments
				0,              // create running
				NULL
				);

		if ( !gs_wxMainThread )
		{
			CloseHandle(hEvent);
			return NULL; // error
		}

		// Wait until MyAppLauncher signals us that wx was initialized. This
		// is because we use wxMessageQueue<> and wxString later and so must
		// be sure that they are in working state.
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);
	}

	// The scripting window object does mean nothing now since the wnidow is managed by
	// global object in the DLL, but return it anyway in order to keep the API unchanged.
	ScripterWindowImpl *ret = new ScripterWindowImpl;
	ret->config = *sc;
	wxGetApp().handle = ret;
	*sc->printProc = PrintProc;

	return ret;
}

int scripter_show(ScripterWindow *){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_WINDOW);
	wxQueueEvent(wxApp::GetInstance(), event);

	return 1;
}

int scripter_lexer_squirrel(ScripterWindow *){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_SETLEXER);
	wxQueueEvent(wxApp::GetInstance(), event);

	return 1;
}

/// \brief Add a script error indicator to specified line in a specified source file.
void scripter_adderror(ScripterWindow*, const char *desc, const char *source, int line, int column){
	wxThreadEvent *event =
		new AddErrorEvent(wxEVT_THREAD, CMD_ADDERROR, desc, source, line, column);
	wxQueueEvent(wxApp::GetInstance(), event);
}

void scripter_clearerror(ScripterWindow*){
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_CLEARERROR);
	wxQueueEvent(wxApp::GetInstance(), event);
}

void scripter_delete(ScripterWindow *sw){
	SqScripterApp& app = wxGetApp();
	if(sw == app.handle)
		app.handle = NULL;
	delete sw;
}
#else
wxIMPLEMENT_APP(SqScripterApp);
#endif


bool SqScripterApp::OnInit()
{
	wxInitAllImageHandlers();
	frame = new SqScripterFrame( "SqScripter", wxPoint(50, 50), wxSize(450, 540) );
#ifdef _DLL
	// Keep the wx "main" thread running even without windows. This greatly
	// simplifies threads handling, because we don't have to correctly
	// implement wx-thread restarting.
	//
	// Note that this only works if you don't explicitly call ExitMainLoop(),
	// except in reaction to wx_dll_cleanup()'s message. wx_dll_cleanup()
	// relies on the availability of wxApp instance and if the event loop
	// terminated, wxEntry() would return and wxApp instance would be
	// destroyed.
	//
	// Also note that this is efficient, because if there are no windows, the
	// thread will sleep waiting for a new event. We could safe some memory
	// by shutting the thread down when it's no longer needed, though.
	SetExitOnFrameDelete(false);

	Connect(CMD_SHOW_WINDOW,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnShowWindow));
	Connect(CMD_TERMINATE,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnTerminate));
	Connect(CMD_SETLEXER,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnSetLexer));
	Connect(CMD_ADDERROR,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnAddError));
	Connect(CMD_CLEARERROR,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnClearError));
	Connect(CMD_PRINT,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnPrint));

	frame->Show( false );
#else
	frame->Show( true );
#endif
	return true;
}

void SqScripterApp::OnShowWindow(wxThreadEvent&){
	frame->Show(true);
}

void SqScripterApp::OnTerminate(wxThreadEvent&){
	ExitMainLoop();
}

void SqScripterApp::OnSetLexer(wxThreadEvent&){
	frame->SetLexer();
}

void SqScripterApp::OnAddError(wxThreadEvent& evt){
	AddErrorEvent& ae = static_cast<AddErrorEvent&>(evt);
	frame->AddError(ae);
}

void SqScripterApp::OnClearError(wxThreadEvent&){
	frame->ClearError();
}

void SqScripterApp::OnPrint(wxThreadEvent& evt){
	*frame->log << evt.GetString();
}

SqScripterFrame::SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size),
	stc(NULL), log(NULL), logger(NULL), dirty(false)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Run, "&Run\tCtrl-R", "Run the program");
	menuFile->AppendSeparator();
	menuFile->Append(ID_Open, "&Open\tCtrl-O", "Open a file");
	menuFile->Append(ID_Save, "&Save\tCtrl-S", "Save and overwrite the file");
	menuFile->AppendSeparator();
	menuFile->Append(ID_Clear, "&Clear Log\tCtrl-C", "Clear output log");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );

	wxSplitterWindow *splitter = new wxSplitterWindow(this);
	// Script editing pane almost follows the window size, while the log pane occupies surplus area.
	splitter->SetSashGravity(0.75);
	splitter->SetMinimumPaneSize(40);

	stc = new wxStyledTextCtrl(splitter);

	log = new wxTextCtrl(splitter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	splitter->SplitHorizontally(stc, log, 200);

	cmd = new wxTextCtrl(this, ID_Command, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(splitter, 1, wxEXPAND | wxALL);
	sizer->Add(cmd, 0, wxEXPAND | wxBOTTOM);
	SetSizer(sizer);

	wxToolBar *toolbar = CreateToolBar();
	wxBitmap bm = wxImage(wxT("../../run.png"));
	toolbar->AddTool(ID_Run, "Run", bm, "Run the program");
	toolbar->AddTool(ID_Open, "Open", wxImage(wxT("../../open.png")), "Open a file");
	toolbar->AddTool(ID_Save, "Save", wxImage(wxT("../../save.png")), "Save a file");
	toolbar->AddTool(ID_Clear, "Clear", wxImage(wxT("../../clear.png")), "Clear output log");
	toolbar->Realize();
	SetMenuBar( menuBar );
	CreateStatusBar();
	SetStatusText( "" );

	logger = new wxLogStream(&std::cout);
}

SqScripterFrame::~SqScripterFrame(){
	delete stc;
	delete log;
	delete logger;
}

void SqScripterFrame::SetLexer(){
	stc->SetLexer(wxSTC_LEX_CPP);
	stc->StyleSetForeground(wxSTC_C_COMMENT, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(0,127,63));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, wxColour(0,63,127));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, wxColour(255,0,0));
	stc->StyleSetForeground(wxSTC_C_NUMBER, wxColour(0,127,255));
	stc->StyleSetForeground(wxSTC_C_WORD, wxColour(0,0,255));
	stc->StyleSetForeground(wxSTC_C_STRING, wxColour(127,0,0));
	stc->StyleSetForeground(wxSTC_C_CHARACTER, wxColour(127,0,127));
	stc->SetKeyWords(0,
		"base break case catch class clone "
		"continue const default delete else enum "
		"extends for foreach function if in "
		"local null resume return switch this "
		"throw try typeof while yield constructor "
		"instanceof true false static ");
	stc->SetKeyWords(2,
		"a addindex addtogroup anchor arg attention author authors b brief bug "
		"c callgraph callergraph category cite class code cond copybrief copydetails "
		"copydoc copyright date def defgroup deprecated details diafile dir docbookonly "
		"dontinclude dot dotfile e else elseif em endcode endcond enddocbookonly enddot "
		"endhtmlonly endif endinternal endlatexonly endlink endmanonly endmsc endparblock "
		"endrtfonly endsecreflist endverbatim enduml endxmlonly enum example exception extends "
		"f$ f[ f] f{ f} file fn headerfile hideinitializer htmlinclude htmlonly idlexcept if "
		"ifnot image implements include includelineno ingroup internal invariant interface "
		"latexinclude latexonly li line link mainpage manonly memberof msc mscfile n name "
		"namespace nosubgrouping note overload p package page par paragraph param parblock "
		"post pre private privatesection property protected protectedsection protocol public "
		"publicsection pure ref refitem related relates relatedalso relatesalso remark remarks "
		"result return returns retval rtfonly sa secreflist section see short showinitializer "
		"since skip skipline snippet startuml struct subpage subsection subsubsection "
		"tableofcontents test throw throws todo tparam typedef union until var verbatim "
		"verbinclude version vhdlflow warning weakgroup xmlonly xrefitem $ @ \\ & ~ < > # %");
	bool state = true;
	stc->SetViewWhiteSpace(state ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
	stc->SetWhitespaceForeground(true, wxColour(0x7f, 0xbf, 0xbf));
}

void SqScripterFrame::AddError(AddErrorEvent &ae){
	if(stc){
		if(strcmp(ae.source, "scriptbuf") /*&& strcmp(ae.source, p->GetCurrentBuffer().fileName.c_str())*/)
			LoadScriptFile(ae.source);

		int pos = stc->PositionFromLine(ae.line - 1);
		stc->IndicatorSetStyle(0, wxSTC_INDIC_SQUIGGLE);
		stc->IndicatorSetForeground(0, wxColour(255,0,0));
		stc->SetIndicatorCurrent(0);
		stc->IndicatorFillRange(pos + ae.column - 1, stc->LineLength(0) - (ae.column - 1));
		stc->MarkerDefine(0, wxSTC_MARK_CIRCLE);
		stc->MarkerSetForeground(0, wxColour(255,0,0));
		stc->MarkerAdd(ae.line - 1, 0);
	}
}

void SqScripterFrame::ClearError(){
	if(stc){
		stc->IndicatorClearRange(0, stc->GetLength());
		stc->MarkerDeleteAll(0);
	}
}

/// Calculate width of line numbers margin by contents
void SqScripterFrame::RecalcLineNumberWidth(){
//	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
/*	for(int i = 0; i < p->buffers.size(); i++){
		HWND hScriptEdit = p->buffers[i].hEdit;
		if(!hScriptEdit)
			return;*/
	{
		bool showState = true;//(GetMenuState(GetMenu(hDlg), IDM_LINENUMBERS, MF_BYCOMMAND) & MF_CHECKED) != 0;

		// Make sure to set type of the first margin to be line numbers
		stc->SetMarginType(0, wxSTC_MARGIN_NUMBER);

		// Obtain width needed to display all line count in the buffer
		int lineCount = stc->GetLineCount();
		wxString lineText = "_";
		for(int i = 0; pow(10, i) <= lineCount; i++)
			lineText += "9";

		int width = showState ? stc->TextWidth(wxSTC_STYLE_LINENUMBER, lineText) : 0;
		stc->SetMarginWidth(0, width);
	}
}

void SqScripterFrame::LoadScriptFile(wxString fileName){

	// Reset status to create an empty buffer if file name is absent
	if(fileName.empty()){
		SetFileName("");
		RecalcLineNumberWidth();
		return;
	}

	HANDLE hFile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		DWORD textLen = GetFileSize(hFile, NULL);
		char *text = (char*)malloc((textLen+1) * sizeof(char));
		ReadFile(hFile, text, textLen, &textLen, NULL);
		wchar_t *wtext = (wchar_t*)malloc((textLen+1) * sizeof(wchar_t));
		text[textLen] = '\0';
		::MultiByteToWideChar(CP_UTF8, 0, text, textLen, wtext, textLen);
		// SetWindowTextA() seems to convert given string into unicode string prior to calling message handler.
		//						SetWindowTextA(GetDlgItem(hDlg, IDC_SCRIPTEDIT), text);
		// SCI_SETTEXT seems to pass the pointer verbatum to the message handler.
		stc->SetText(text);
		CloseHandle(hFile);
		free(text);
		free(wtext);

		// Clear undo buffer instead of setting a savepoint because we don't want to undo to empty document
		// if it's opened from a file.
		stc->EmptyUndoBuffer();

		SetFileName(fileName);
		RecalcLineNumberWidth();
	}
}

void SqScripterFrame::SaveScriptFile(const char *fileName){
	HANDLE hFile = CreateFileA(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		wxString text = stc->GetText();
		DWORD textLen;
		WriteFile(hFile, text, text.length(), &textLen, NULL);
		CloseHandle(hFile);
		// Set savepoint for buffer dirtiness management
		stc->SetSavePoint();
		SetFileName(fileName); // Remember the file name for the next save operation
	}
}


void SqScripterFrame::UpdateTitle(){
	wxString title = "Scripting Window " + fileName;
	SetTitle(title);
}

void SqScripterFrame::SetFileName(wxString fileName, bool dirty){
	this->fileName = fileName;
	this->dirty = dirty;

	UpdateTitle();
}

void SqScripterFrame::OnExit(wxCommandEvent& event)
{
	Close( true );
}

void SqScripterFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox( "SqScripter powered by wxWidgets & Scintilla",
		"About SqScripter", wxOK | wxICON_INFORMATION );
}

void SqScripterFrame::OnClear(wxCommandEvent& event){
	log->Clear();
}

void SqScripterFrame::OnEnterCmd(wxCommandEvent& event){
	wxLog* oldLogger = wxLog::SetActiveTarget(logger);
	wxStreamToTextRedirector redirect(log);
	if(wxGetApp().handle && wxGetApp().handle->config.commandProc)
		wxGetApp().handle->config.commandProc(cmd->GetValue());
	else
		wxLogMessage("Execute the command");
	wxLog::SetActiveTarget(oldLogger);
}

void SqScripterFrame::OnRun(wxCommandEvent& event)
{
	wxLog* oldLogger = wxLog::SetActiveTarget(logger);
	wxStreamToTextRedirector redirect(log);
	if(wxGetApp().handle && wxGetApp().handle->config.runProc)
		wxGetApp().handle->config.runProc(fileName, stc->GetText());
	else
		wxLogMessage("Run the program");
	wxLog::SetActiveTarget(oldLogger);
}

void SqScripterFrame::OnOpen(wxCommandEvent& event)
{
	wxFileDialog openFileDialog(this, _("Open NUT file"), "", "",
			"Squirrel source files (*.nut)|*.nut", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	LoadScriptFile(openFileDialog.GetPath());
}

void SqScripterFrame::OnSave(wxCommandEvent& event)
{
	wxFileDialog openFileDialog(this, _("Save NUT file"), "", "",
		"Squirrel source files (*.nut)|*.nut", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	SaveScriptFile(openFileDialog.GetPath());
}
