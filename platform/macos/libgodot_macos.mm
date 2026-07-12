/**************************************************************************/
/*  libgodot_macos.mm                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "os_macos.h"

#include "core/core_globals.h"
#include "core/extension/godot_instance.h"
#include "core/extension/libgodot.h"
#include "main/main.h"

static OS_MacOS *os = nullptr;

static GodotInstance *instance = nullptr;

GDExtensionObjectPtr libgodot_create_godot_instance(int p_argc, char *p_argv[], GDExtensionInitializationFunction p_init_func) {
	ERR_FAIL_COND_V_MSG(instance != nullptr, nullptr, "Only one Godot Instance may be created.");

	CoreGlobals::global_init_func_libgodot = p_init_func;
	CoreGlobals::engine_reinit_enabled = true;

	// Check argv for headless flags before creating the OS instance,
	// mirroring the logic in godot_main_macos.mm. OS_MacOS_NSApp
	// creates NSApplication which requires the main thread; the
	// headless variant avoids AppKit entirely.
	bool is_headless = false;
	for (int i = 1; i < p_argc; i++) {
		for (size_t j = 0; j < std::size(OS_MacOS::headless_args); j++) {
			if (strcmp(OS_MacOS::headless_args[j], p_argv[i]) == 0) {
				is_headless = true;
				break;
			}
		}
		if (i < p_argc - 1 && strcmp("--display-driver", p_argv[i]) == 0 && strcmp("headless", p_argv[i + 1]) == 0) {
			is_headless = true;
		}
		if (is_headless) {
			break;
		}
	}

	uint32_t remaining_args = p_argc - 1;
	if (is_headless) {
		os = new OS_MacOS_Headless(p_argv[0], remaining_args, remaining_args > 0 ? &p_argv[1] : nullptr);
	} else {
		os = new OS_MacOS_NSApp(p_argv[0], remaining_args, remaining_args > 0 ? &p_argv[1] : nullptr);
	}

	@autoreleasepool {
		Error err = Main::setup(p_argv[0], remaining_args, remaining_args > 0 ? &p_argv[1] : nullptr, false);
		if (err != OK) {
			return nullptr;
		}

		instance = memnew(GodotInstance);
		if (!instance->initialize()) {
			memdelete(instance);
			instance = nullptr;
			return nullptr;
		}

		return (GDExtensionObjectPtr)instance;
	}
}

void libgodot_destroy_godot_instance(GDExtensionObjectPtr p_godot_instance) {
	GodotInstance *godot_instance = (GodotInstance *)p_godot_instance;
	if (instance == godot_instance) {
		godot_instance->stop();
		memdelete(godot_instance);
		instance = nullptr;
		Main::cleanup();
		delete os;
		os = nullptr;
	}
}

int libgodot_import_project(const char *p_project_path, int p_extra_argc, const char *p_extra_argv[]) {
#ifndef TOOLS_ENABLED
	return -1; // Editor builds only.
#else
	ERR_FAIL_NULL_V(p_project_path, EXIT_FAILURE);
	ERR_FAIL_COND_V_MSG(instance != nullptr || os != nullptr, EXIT_FAILURE,
			"libgodot_import_project cannot run while a Godot instance exists in this process.");

	Vector<char *> argv;
	argv.push_back(const_cast<char *>("--headless"));
	argv.push_back(const_cast<char *>("--import"));
	argv.push_back(const_cast<char *>("--path"));
	argv.push_back(const_cast<char *>(p_project_path));
	for (int i = 0; i < p_extra_argc; i++) {
		argv.push_back(const_cast<char *>(p_extra_argv[i]));
	}

	// Import always runs headless, so the AppKit-free OS variant is safe
	// regardless of the calling thread (OS_MacOS_NSApp requires the main
	// thread; see libgodot_create_godot_instance above).
	OS_MacOS_Headless import_os("libgodot", argv.size(), argv.ptrw());

	int exit_code;
	@autoreleasepool {
		// Mirrors godot_main_macos.mm: full second-phase setup, then run()
		// pumps Main::iteration until the first scan finishes (--import
		// implies wait_for_import + quit_after=1).
		Error err = Main::setup("libgodot", argv.size(), argv.ptrw());
		if (err != OK) {
			return (err == ERR_HELP) ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		if (Main::start() == EXIT_SUCCESS) {
			import_os.run();
		} else {
			import_os.set_exit_code(EXIT_FAILURE);
		}
		Main::cleanup();
		exit_code = import_os.get_exit_code();
	}
	return exit_code;
#endif
}
