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
// Taken from https://stackoverflow.com/a/77336 and https://stackoverflow.com/a/26398082
static void segfault_handler(const int sig) {
#ifdef _WIN32
	HANDLE process_handle = GetCurrentProcess();
	SymInitialize(process_handle, nullptr, true);

	constexpr int kMaxBacktraceSize = 62;
	void* array[kMaxBacktraceSize];
	size_t size = CaptureStackBackTrace(0, kMaxBacktraceSize, array, nullptr);

	std::stringstream translated_backtrace;
	translated_backtrace << size << " frames captured:" << std::endl;
	for (size_t i = 0; i < size; ++i) {
		const DWORD64 frame_as_int = reinterpret_cast<DWORD64>(array[i]);
		translated_backtrace << "#" << std::dec << i << " [0x" << std::hex << frame_as_int << "] ";

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO p_symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
		if (SymFromAddr(process_handle, frame_as_int, nullptr, p_symbol)) {
			translated_backtrace << p_symbol->Name << " [0x" << reinterpret_cast<uintptr_t>(p_symbol->Address) << "]";
		} else {
			translated_backtrace << "Error symbolizing frame address";
		}
		translated_backtrace << std::endl;
	}
	const std::string bt_str = translated_backtrace.str();

	SymCleanup(process_handle);
#else
	constexpr int kMaxBacktraceSize = 256;
	void* array[kMaxBacktraceSize];
	size_t size = backtrace(array, kMaxBacktraceSize);
#endif

	std::cout << std::endl
	          << "##############################" << std::endl
	          << "FATAL ERROR: Received signal " << sig;
#ifndef _WIN32
	std::cout << " (" << strsignal(sig) << ")"
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
		   file, "Crash report for Widelands %s at %s, signal %d\n\n**** BEGIN BACKTRACE ****\n",
		   build_ver_details().c_str(), timestr.c_str(), sig);
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

	::exit(sig);
}
#endif

/**
 * Cross-platform entry point for SDL applications.
 */
int main(int argc, char* argv[]) {
	std::cout << "This is Widelands version " << build_ver_details() << std::endl;

#ifdef PRINT_SEGFAULT_BACKTRACE
	/* Handle several types of fatal crashes with a useful backtrace on supporting systems.
	 * We can't handle SIGABRT like this since we have to redirect that one elsewhere to
	 * suppress non-critical errors from Eris.
	 */
	for (int s : {SIGSEGV,
#ifdef SIGBUS
			SIGBUS,  // Not available on all systems
#endif
			SIGFPE, SIGILL}) {
		signal(s, segfault_handler);
	}
#endif

	try {
		WLApplication& g_app = WLApplication::get(argc, const_cast<char const**>(argv));
		// TODO(unknown): handle exceptions from the constructor
		g_app.run();

		return 0;
	} catch (const ParameterError& e) {
		//  handle wrong commandline parameters
		show_usage(build_ver_details(), e.level_);
		if (e.what()[0] != 0) {
			std::cerr << std::string(60, '=') << std::endl << std::endl << e.what() << std::endl;
		}

		return 0;
	}
#ifdef NDEBUG
	catch (const WException& e) {
		std::cerr << "\nCaught exception (of type '" << typeid(e).name()
		          << "') in outermost handler!\nThe exception said: " << e.what()
		          << "\n\nThis should not happen. Please file a bug report on version "
		          << build_ver_details() << ".\n"
		          << "and remember to specify your operating system.\n\n"
		          << std::flush;

		return 1;
	} catch (const std::exception& e) {
		std::cerr << "\nCaught exception (of type '" << typeid(e).name()
		          << "') in outermost handler!\nThe exception said: " << e.what()
		          << "\n\nThis should not happen. Please file a bug report on version "
		          << build_ver_details() << ".\n"
		          << "and remember to specify your operating system.\n\n"
		          << std::flush;

		return 1;
	}
#endif
}
