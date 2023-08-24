/*
 * Copyright (C) 2002-2023 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <iostream>
#include <typeinfo>

#ifdef _MSC_VER
// Needed to resolve entry point
#include <SDL.h>
#else
#include <unistd.h>
#endif
#ifdef PRINT_SEGFAULT_BACKTRACE
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif
#endif

#include "base/time_string.h"
#include "base/wexception.h"
#include "build_info.h"
#include "config.h"
#include "logic/filesystem_constants.h"
#include "wlapplication.h"
#include "wlapplication_messages.h"

#ifdef PRINT_SEGFAULT_BACKTRACE
#ifdef _WIN32
static HANDLE g_process_handle;
#endif

/* Taken from https://stackoverflow.com/a/77336, https://stackoverflow.com/a/26398082,
 * and https://stackoverflow.com/a/28115589
 */
#ifdef _WIN32
LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS exception_info) {
#else
static void segfault_handler(const int sig) {
#endif

#ifdef _WIN32
	std::stringstream translated_backtrace;

	CONTEXT context_record = *exception_info->ContextRecord;
	STACKFRAME64 stack_frame;
	memset(&stack_frame, 0, sizeof(stack_frame));
#ifdef _WIN64
	int machine_type = IMAGE_FILE_MACHINE_AMD64;
	stack_frame.AddrPC.Offset = context_record.Rip;
	stack_frame.AddrFrame.Offset = context_record.Rbp;
	stack_frame.AddrStack.Offset = context_record.Rsp;
#else
	int machine_type = IMAGE_FILE_MACHINE_I386;
	stack_frame.AddrPC.Offset = context_record.Eip;
	stack_frame.AddrFrame.Offset = context_record.Ebp;
	stack_frame.AddrStack.Offset = context_record.Esp;
#endif
	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrStack.Mode = AddrModeFlat;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO p_symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
	p_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	p_symbol->MaxNameLen = MAX_SYM_NAME;

	int frame_index = 0;
	while (StackWalk64(machine_type, GetCurrentProcess(), GetCurrentThread(), &stack_frame, &context_record, nullptr, &SymFunctionTableAccess64, &SymGetModuleBase64, nullptr)) {
		const DWORD64 frame_as_int = reinterpret_cast<DWORD64>(stack_frame.AddrPC.Offset);
		translated_backtrace << "#" << std::dec << ++frame_index << " [0x" << std::hex << frame_as_int << "] ";

		DWORD64 displacement = 0;
		if (SymFromAddr(g_process_handle, frame_as_int, &displacement, p_symbol)) {
			IMAGEHLP_MODULE64 module_info;
			memset(&module_info, 0, sizeof(module_info));
			module_info.SizeOfStruct = sizeof(module_info);

			if (::SymGetModuleInfo64(g_process_handle, p_symbol->ModBase, &module_info)) {
				translated_backtrace << module_info.ModuleName << " + 0x" << displacement;
			} else {
				translated_backtrace << "Error symbolizing module info (error code 0x" << GetLastError() << ")";
			}
		} else {
			translated_backtrace << "Error symbolizing frame address (error code 0x" << GetLastError() << ")";
		}
		translated_backtrace << std::endl;
	}

	const std::string bt_str = translated_backtrace.str();
#else
	constexpr int kMaxBacktraceSize = 256;
	void* array[kMaxBacktraceSize];
	size_t size = backtrace(array, kMaxBacktraceSize);
#endif

	std::cout << std::endl
	          << "##############################" << std::endl
#ifdef _WIN32
	std::cout << "FATAL ERROR encountered!";
#else
	std::cout << "FATAL ERROR: Received signal " << sig << " (" << strsignal(sig) << ")";
#endif
	std::cout << std::endl << "Backtrace:" << std::endl;

#ifdef _WIN32
	std::cout << bt_str;
#else
	backtrace_symbols_fd(array, size, STDOUT_FILENO);
#endif

	std::cout
	   << std::endl
	   << "Please report this problem to help us improve Widelands, and provide the complete output."
	   << std::endl
	   << "##############################" << std::endl;

	const std::string timestr = timestring();
	std::string filename;
	if (WLApplication::segfault_backtrace_dir.empty()) {
		filename = "./widelands_crash_report_";
		filename += timestr;
	} else {
		filename = WLApplication::segfault_backtrace_dir;
		filename += "/";
		filename += timestr;
	}
	filename += kCrashExtension;
	FILE* file = fopen(filename.c_str(), "w+");
	if (file == nullptr) {
		std::cout << "The crash report could not be saved to a file." << std::endl << std::endl;
	} else {
#ifdef _WIN32
		fprintf /* NOLINT codecheck */ (
		   file, "Crash report for Widelands %s at %s\n\n**** BEGIN BACKTRACE ****\n",
		   build_ver_details().c_str(), timestr.c_str());
		fputs(bt_str.c_str(), file);
#else
		fprintf /* NOLINT codecheck */ (
		   file, "Crash report for Widelands %s at %s, signal %d (%s)\n\n**** BEGIN BACKTRACE ****\n",
		   build_ver_details().c_str(), timestr.c_str(), sig, strsignal(sig));
		fflush(file);
		backtrace_symbols_fd(array, size, fileno(file));
		fflush(file);
#endif
		fputs("**** END BACKTRACE ****\n", file);

		fclose(file);
		std::cout << "The crash report was also saved to " << filename << std::endl << std::endl;
	}

#ifdef _WIN32
	return EXCEPTION_CONTINUE_SEARCH;
#else
	::exit(sig);
#endif
}
#endif  // PRINT_SEGFAULT_BACKTRACE

/**
 * Cross-platform entry point for SDL applications.
 */
int main(int argc, char* argv[]) {
	std::cout << "This is Widelands version " << build_ver_details() << std::endl;

#ifdef PRINT_SEGFAULT_BACKTRACE
#ifdef _WIN32
	g_process_handle = GetCurrentProcess();
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
	if (!SymInitialize(g_process_handle, nullptr, true)) {
		std::cout << "ERROR: Could not initialize the symbolizer (error code 0x" << std::hex
		          << GetLastError() << std::dec << ")" << std::endl;
	}

	SetUnhandledExceptionFilter(TopLevelExceptionHandler);

#else
	/* Handle several types of fatal crashes with a useful backtrace on supporting systems.
	 * We can't handle SIGABRT like this since we have to redirect that one elsewhere to
	 * suppress non-critical errors from Eris.
	 */
	for (int s : {SIGSEGV, SIGBUS, SIGFPE, SIGILL}) {
		signal(s, segfault_handler);
	}
#endif
#endif  // PRINT_SEGFAULT_BACKTRACE

	int result = 1;
	try {
		WLApplication& g_app = WLApplication::get(argc, const_cast<char const**>(argv));
		// TODO(unknown): handle exceptions from the constructor
		g_app.run();

		result = 0;
	} catch (const ParameterError& e) {
		//  handle wrong commandline parameters
		show_usage(build_ver_details(), e.level_);
		if (e.what()[0] != 0) {
			std::cerr << std::string(60, '=') << std::endl << std::endl << e.what() << std::endl;
		}

		result = 0;
	}
#ifdef NDEBUG
	catch (const WException& e) {
		std::cerr << "\nCaught exception (of type '" << typeid(e).name()
		          << "') in outermost handler!\nThe exception said: " << e.what()
		          << "\n\nThis should not happen. Please file a bug report on version "
		          << build_ver_details() << ".\n"
		          << "and remember to specify your operating system.\n\n"
		          << std::flush;
	} catch (const std::exception& e) {
		std::cerr << "\nCaught exception (of type '" << typeid(e).name()
		          << "') in outermost handler!\nThe exception said: " << e.what()
		          << "\n\nThis should not happen. Please file a bug report on version "
		          << build_ver_details() << ".\n"
		          << "and remember to specify your operating system.\n\n"
		          << std::flush;
	}
#endif

#ifdef PRINT_SEGFAULT_BACKTRACE
#ifdef _WIN32
	SymCleanup(g_process_handle);
#endif
#endif

	return result;
}
