// wxWidgets "Hello world" Program
// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/stc/stc.h>
#include <wx/splitter.h>

class SqScripterApp: public wxApp
{
public:
	virtual bool OnInit();
};


class SqScripterFrame: public wxFrame
{
public:
	SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~SqScripterFrame()override;
private:
	void OnRun(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	wxDECLARE_EVENT_TABLE();

	wxTextCtrl *log = nullptr;
	wxLog *logger = nullptr;
};


enum
{
	ID_Run = 1
};


wxBEGIN_EVENT_TABLE(SqScripterFrame, wxFrame)
EVT_MENU(ID_Run,   SqScripterFrame::OnRun)
EVT_MENU(wxID_EXIT,  SqScripterFrame::OnExit)
EVT_MENU(wxID_ABOUT, SqScripterFrame::OnAbout)
wxEND_EVENT_TABLE()


wxIMPLEMENT_APP(SqScripterApp);


bool SqScripterApp::OnInit()
{
	wxInitAllImageHandlers();
	SqScripterFrame *frame = new SqScripterFrame( "SqScripter", wxPoint(50, 50), wxSize(450, 540) );
	frame->Show( true );
	return true;
}

inline int SciRGB(unsigned char r, unsigned char g, unsigned char b){
	return r | (g << 8) | (b << 16);
}

SqScripterFrame::SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Run, "&Run\tCtrl-R",
		"Run the program");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );
	wxSplitterWindow *splitter = new wxSplitterWindow(this);

	wxStyledTextCtrl *stc = new wxStyledTextCtrl(splitter);
	stc->SetLexer(wxSTC_LEX_CPP);
	stc->StyleSetForeground(wxSTC_C_COMMENT, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINE, SciRGB(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOC, SciRGB(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, SciRGB(0,127,63));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, SciRGB(0,63,127));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, SciRGB(255,0,0));
	stc->StyleSetForeground(wxSTC_C_NUMBER, SciRGB(0,127,255));
	stc->StyleSetForeground(wxSTC_C_WORD, SciRGB(0,0,255));
	stc->StyleSetForeground(wxSTC_C_STRING, SciRGB(127,0,0));
	stc->StyleSetForeground(wxSTC_C_CHARACTER, SciRGB(127,0,127));
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
	stc->SetWhitespaceForeground(true, SciRGB(0x7f, 0xbf, 0xbf));

	log = new wxTextCtrl(splitter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	splitter->SplitHorizontally(stc, log, 200);
	wxToolBar *toolbar = CreateToolBar();
	wxBitmap bm = wxImage(wxT("../../run.png"));
	toolbar->AddTool(ID_Run, "Run", bm, "Run the program");
	toolbar->Realize();
	SetMenuBar( menuBar );
	CreateStatusBar();
	SetStatusText( "" );

	logger = new wxLogStream(&std::cout);
}

SqScripterFrame::~SqScripterFrame(){
	delete log;
	delete logger;
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

void SqScripterFrame::OnRun(wxCommandEvent& event)
{
	wxLog* oldLogger = wxLog::SetActiveTarget(logger);
	wxStreamToTextRedirector redirect(log);
	wxLogMessage("Run the program");
	wxLog::SetActiveTarget(oldLogger);
}
