#ifndef SQSCRIPTER_H
#define SQSCRIPTER_H

#ifdef __cplusplus
extern "C"{
#endif

struct ScripterWindow;

/// \brief Parameter set for a scripter window
struct ScripterConfig{
	void (*commandProc)(const char*);
	void (**printProc)(ScripterWindow *, const char*);
	void (*runProc)(const char *fileName, const char *content);
	const char *sourceFilters;
};

/// \brief A Scripter Window handle.  Don't rely on this internal structure.
struct ScripterWindow{
	ScripterConfig config;
};

/// \brief Initialize the scripter system with command and print procedures
ScripterWindow *scripter_init(const ScripterConfig *);


/// \brief Show the scripter window
int scripter_show(ScripterWindow *);


#ifdef __cplusplus
}
#endif

#endif
