/** \file 
 * \brief The main source file for gltestplus Windows client.
 *
 * The entry point for the program, no matter main() or WinMain(), resides in this file.
 * It deals with the main message loop, OpenGL initialization, and event mapping.
 * Some drawing methods are also defined here.
 */

#include "sqscripter.h"

#include <windows.h>
#include "resource.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdint.h> // Exact-width integer types

#include <string>
#include <vector>

static void (*PrintProc)(const char *) = NULL;

void CmdProc(const char *cmd){
	MessageBox(NULL, cmd, "Command Executed", MB_ICONERROR);
	PrintProc((std::string("Echo: ") + cmd).c_str());
}

int main(int argc, char *argv[])
{
	scripter_init(CmdProc, &PrintProc);

	scripter_show();


	do{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(GetMessage(&msg, NULL, 0, 0) <= 0)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
//		else
//			SendMessage(hWnd, WM_TIMER, 0, 0);
	}while (true);

	while(ShowCursor(TRUE) < 0);
	while(0 <= ShowCursor(FALSE));

	return 0;
}
