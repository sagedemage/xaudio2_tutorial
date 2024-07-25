#ifndef HRESULT_DEBUGGER_H
#define HRESULT_DEBUGGER_H

/* Third Party Libraries*/
#include <comdef.h>
#include <string>

void verbose_debug_hresult(HRESULT hr, std::string description);

#endif // HRESULT_DEBUGGER_H
