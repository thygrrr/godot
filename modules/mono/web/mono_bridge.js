// SPDX-License-Identifier: MIT
// 2dog: this file is part of https://2dog.dev

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
