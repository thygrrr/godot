/**************************************************************************/
/*  mono_bridge.js                                                        */
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

// Wrap the .NET runtime module as Godot's Emscripten module.
const Godot = async (moduleConfig) => { // eslint-disable-line no-unused-vars
	// Required for JSImport, JSExport, and multithreading.
	delete moduleConfig['instantiateWasm'];

	// Depends on "dotnet.native.wasm" being in place of "godot.wasm".
	const loadPath = moduleConfig['locateFile']('dotnet.native.wasm');
	// Get preloaded wasm.
	let preloadedWasm = moduleConfig['getPreloadedWasm']();

	// dynamic module import.
	const dotnetjs = await import('./_framework/dotnet.js');
	const dotnet = dotnetjs.dotnet;

	dotnet
		// Pass emscripten config.
		.withModuleConfig(moduleConfig)
		.withConfig({
			// Let .NET configure the initial thread-pool size.
			pthreadPoolInitialSize: moduleConfig['emscriptenPoolSize'] || 8,
			// Enables synchronous JSImport and JSExport calls with threads.
			jsThreadBlockingMode: 'ThrowWhenBlockingWait',
		})
		.withResourceLoader((_type, name, _defaultUri, _integrity, _behavior) => {
			if (name === 'dotnet.native.wasm') {
				if (preloadedWasm) {
					// Pass the preloaded wasm response to the Godot loader.
					const promise = Promise.resolve(preloadedWasm);
					preloadedWasm = null;
					return promise;
				}
				// Fall back to the resolved wasm path.
				return loadPath;
			}
			// Use the default path.
			return null;
		});

	await dotnet.download();
	const { setModuleImports, getAssemblyExports, getConfig, runMain, Module } = await dotnet.create();

	const dotnetConfig = getConfig();

	if (moduleConfig['godotSharpImports']) {
		const moduleImports = moduleConfig['godotSharpImports'];
		for (const moduleName of Object.keys(moduleImports)) {
			setModuleImports(moduleName, moduleImports[moduleName]);
		}
	}
	Module['getGodotSharpExports'] = getAssemblyExports.bind(null, dotnetConfig.mainAssemblyName);

	// Godot starts wasm through callMain; provide it when Emscripten omits it.
	Module.callMain = (args) => runMain(dotnetConfig.mainAssemblyName, args);
	return Module;
};
