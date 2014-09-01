#include "sqscripter.h"

#include <windows.h>
#include "resource.h"

#include <string>
#include <vector>



static void (*CmdProc)(const char *buf) = NULL;
static void ScriptDlgPrint(const char*);

void scripter_init(void commandProc(const char*), void (**printProc)(const char*)){
	CmdProc = commandProc;
	*printProc = ScriptDlgPrint;
}

/// Window handle for the scripting window.
static HWND hwndScriptDlg = NULL;

/// Event handler function to receive printed messages in the console for displaying on scripting window.
static void ScriptDlgPrint(const char *line){
	if(IsWindow(hwndScriptDlg)){
		HWND hEdit = GetDlgItem(hwndScriptDlg, IDC_CONSOLE);
		size_t buflen = GetWindowTextLength(hEdit);
		SendMessage(hEdit, EM_SETSEL, buflen, buflen);
		std::string s = line;
		s += "\r\n";
		SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) s.c_str());
	}
}

/// Remembered default edit control's window procedure
static WNDPROC defEditWndProc = NULL;

static HMODULE hSciLexer = NULL;

/// \brief Window procedure to replace edit control for subclassing command edit control
///
/// Handles enter key and up/down arrow keys to constomize behavior of default edit control.
static INT_PTR CALLBACK ScriptCommandProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	static std::vector<char*> cmdHistory;
	static int currentHistory = 0;
	switch (message)
	{
	case WM_GETDLGCODE:
		return (DLGC_WANTALLKEYS |
				CallWindowProc(defEditWndProc, hWnd, message,
								wParam, lParam));

	case WM_CHAR:
		//Process this message to avoid message beeps.
		if ((wParam == VK_RETURN) || (wParam == VK_TAB))
			return 0;
		else
			return (CallWindowProc(defEditWndProc, hWnd,
								message, wParam, lParam));

	case WM_KEYDOWN:
		// Enter key issues command
		if(wParam == VK_RETURN){
			size_t buflen = GetWindowTextLengthA(hWnd);
			char *buf = (char*)malloc(buflen+1);
			GetWindowTextA(hWnd, buf, buflen+1);
			CmdProc(buf);
			// Remember issued command in the history buffer only if the command is not repeated.
			if(cmdHistory.empty() || strcmp(cmdHistory.back(), buf)){
				cmdHistory.push_back(buf);
				currentHistory = -1;
			}
			else // Forget repeated command
				free(buf);
			SetWindowTextA(hWnd, "");
			return FALSE;
		}

		// TODO: command completion
		if(wParam == VK_TAB){
		}

		// Up/down arrow keys navigate through command history.
		if(wParam == VK_UP || wParam == VK_DOWN){
			if(cmdHistory.size() != 0){
				if(currentHistory == -1)
					currentHistory = cmdHistory.size() - 1;
				else
					currentHistory = (currentHistory + (wParam == VK_UP ? -1 : 1) + cmdHistory.size()) % cmdHistory.size();
				SetWindowTextA(hWnd, cmdHistory[currentHistory]);
			}
			return FALSE;
		}

		return CallWindowProc(defEditWndProc, hWnd, message, wParam, lParam);
		break ;

	default:
		break;

	} /* end switch */
	return CallWindowProc(defEditWndProc, hWnd, message, wParam, lParam);
}

/// Dialog handler for scripting window
static INT_PTR CALLBACK ScriptDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	static HWND hwndScintilla = NULL;
	switch(message){
	case WM_INITDIALOG:
		defEditWndProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hDlg, IDC_COMMAND), GWLP_WNDPROC);
		SetWindowLongPtr(GetDlgItem(hDlg, IDC_COMMAND), GWLP_WNDPROC, (LONG_PTR)ScriptCommandProc);
		if(hSciLexer){
			HWND hScriptEdit = GetDlgItem(hDlg, IDC_SCRIPTEDIT);
			RECT r;
			GetWindowRect(hScriptEdit, &r);
			POINT lt = {r.left, r.top};
			ScreenToClient(hDlg, &lt);
			hwndScintilla = CreateWindowExW(0,
				L"Scintilla", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
				lt.x, lt.y, lt.x + r.right - r.left, lt.y + r.bottom - r.top, hDlg,
				(HMENU)IDC_SCRIPTEDIT, GetModuleHandle(NULL), NULL);
			if(!hwndScintilla)
				break;
			// If we could create Scintilla window control, delete normal edit control.
			DestroyWindow(hScriptEdit);
		}
		break;
	case WM_DESTROY:
		hwndScriptDlg = NULL;
		break;
	case WM_SIZE:
		{
			HWND hEdit = GetDlgItem(hDlg, IDC_CONSOLE);
			HWND hScr = GetDlgItem(hDlg, IDC_SCRIPTEDIT);
			HWND hRun = GetDlgItem(hDlg, IDOK);
			RECT scr, runr, rect, cr, wr, comr;
			GetClientRect(hDlg, &cr);

			GetWindowRect(hScr, &scr);
			POINT posScr = {scr.left, scr.top};
			ScreenToClient(hDlg, &posScr);
			GetWindowRect(hRun, &runr);
			SetWindowPos(hScr, NULL, 0, 0, cr.right - posScr.x - (runr.right - runr.left) - 10,
				scr.bottom - scr.top, SWP_NOMOVE);
			SetWindowPos(hRun, NULL, cr.right - (runr.right - runr.left) - 5, 10, 0, 0, SWP_NOSIZE);
			SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL, cr.right - (runr.right - runr.left) - 5, 10 + (runr.bottom - runr.top + 10), 0, 0, SWP_NOSIZE);
			SetWindowPos(GetDlgItem(hDlg, IDC_CLEARCONSOLE), NULL, cr.right - (runr.right - runr.left) - 5, 10 + (runr.bottom - runr.top + 10) * 2, 0, 0, SWP_NOSIZE);

			GetWindowRect(hEdit, &rect);
			POINT posEdit = {rect.left, rect.top};
			ScreenToClient(hDlg, &posEdit);
			GetWindowRect(GetDlgItem(hDlg, IDC_COMMAND), &comr);
			GetWindowRect(hDlg, &wr);
			SetWindowPos(hEdit, NULL, 0, 0, cr.right - posEdit.x - 5,
				cr.bottom - posEdit.y - (comr.bottom - comr.top) - 10, SWP_NOMOVE);
			POINT posCom = {comr.left, comr.top};
			ScreenToClient(hDlg, &posCom);
			SetWindowPos(GetDlgItem(hDlg, IDC_COMMAND), NULL, posCom.x, cr.bottom - (comr.bottom - comr.top) - 5,
				cr.right - posCom.x - 5, comr.bottom - comr.top, 0);
		}
		break;
	case WM_COMMAND:
		{
			UINT id = LOWORD(wParam);
			if(id == IDOK){
				HWND hEdit = GetDlgItem(hDlg, IDC_SCRIPTEDIT);
				size_t buflen = GetWindowTextLengthW(hEdit)+1;
				char *buf = (char*)malloc(buflen * 3 * sizeof*buf);
				wchar_t *wbuf = (wchar_t*)malloc(buflen * sizeof*wbuf);
				SendMessageW(hEdit, WM_GETTEXT, buflen, (LPARAM)wbuf);
				// Squirrel is not compiled with unicode, so we must convert text into utf-8, which is ascii transparent.
				::WideCharToMultiByte(CP_UTF8, 0, wbuf, buflen, buf, buflen * 3, NULL, NULL);
/*				HSQUIRRELVM v = application.clientGame->sqvm;
				if(SQ_FAILED(sq_compilebuffer(v, buf, strlen(buf), "buf", SQTrue))){
					free(buf);
					return TRUE;
				}*/
				free(buf);
				free(wbuf);
/*				sq_pushroottable(v);
				if(SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue)))
					return TRUE;*/
				return TRUE;
			}
			else if(id == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			else if(id == IDC_CLEARCONSOLE){
				SetDlgItemText(hwndScriptDlg, IDC_CONSOLE, "");
				return TRUE;
			}
			else if(id == IDM_SCRIPT_OPEN){
				static char fileBuf[MAX_PATH];
				OPENFILENAME ofn = {
					sizeof(OPENFILENAME), //  DWORD         lStructSize;
					hDlg, //  HWND          hwndOwner;
					NULL, // HINSTANCE     hInstance; ignored
					"Squirrel Script (.nut)\0*.nut\0", //  LPCTSTR       lpstrFilter;
					NULL, //  LPTSTR        lpstrCustomFilter;
					0, // DWORD         nMaxCustFilter;
					0, // DWORD         nFilterIndex;
					fileBuf, // LPTSTR        lpstrFile;
					sizeof fileBuf, // DWORD         nMaxFile;
					NULL, //LPTSTR        lpstrFileTitle;
					0, // DWORD         nMaxFileTitle;
					".", // LPCTSTR       lpstrInitialDir;
					"Select Squirrel Script to Load", // LPCTSTR       lpstrTitle;
					0, // DWORD         Flags;
					0, // WORD          nFileOffset;
					0, // WORD          nFileExtension;
					NULL, // LPCTSTR       lpstrDefExt;
					0, // LPARAM        lCustData;
					NULL, // LPOFNHOOKPROC lpfnHook;
					NULL, // LPCTSTR       lpTemplateName;
				};
				if(GetOpenFileName(&ofn)){
					HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if(hFile){
						DWORD textLen = GetFileSize(hFile, NULL);
						char *text = (char*)malloc((textLen+1) * sizeof(char));
						ReadFile(hFile, text, textLen, &textLen, NULL);
						wchar_t *wtext = (wchar_t*)malloc((textLen+1) * sizeof(wchar_t));
						text[textLen] = '\0';
						::MultiByteToWideChar(CP_UTF8, 0, text, textLen, wtext, textLen);
						SetWindowTextA(GetDlgItem(hDlg, IDC_SCRIPTEDIT), text);
						CloseHandle(hFile);
						free(text);
						free(wtext);
					}
				}
			}
		}
		break;
	}
	return FALSE;
}

/// Toggle scripting window
int cmd_scripter(){
	if(!hSciLexer){
		hSciLexer = LoadLibrary("SciLexer.DLL");
		if(hSciLexer == NULL){
			MessageBox(NULL,
			"The Scintilla DLL could not be loaded.",
			"Error loading Scintilla",
			MB_OK | MB_ICONERROR);
		}
	}

	if(!IsWindow(hwndScriptDlg)){
		hwndScriptDlg = CreateDialogParam(GetModuleHandle(NULL), "ScriptWin", NULL, ScriptDlg, (LPARAM)0);
	}
	else
		ShowWindow(hwndScriptDlg, SW_SHOW);
	return 0;
}
