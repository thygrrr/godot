// SPDX-License-Identifier: MIT
// 2dog: this file is part of https://2dog.dev

// 2dog: .NET resource names are file names, but its default URLs do not escape
// URL delimiters such as '#' before fetch interprets them.
const encodeDotnetResourceUrl = (defaultUri, name) => {
	const uriName = encodeURI(name);
	const matchedName = defaultUri.includes(name) ? name : uriName;
	const nameIndex = defaultUri.lastIndexOf(matchedName);
	if (nameIndex < 0) {
		return defaultUri;
	}
	const encodedName = name.split('/').map(encodeURIComponent).join('/');
	return `${defaultUri.slice(0, nameIndex)}${encodedName}${defaultUri.slice(nameIndex + matchedName.length)}`;
};

// Wrap the .NET runtime module as Godot's Emscripten module.
const Godot = async (moduleConfig) => { // eslint-disable-line no-unused-vars
	// Required for JSImport, JSExport, and multithreading.
	delete moduleConfig['instantiateWasm'];

	// Depends on "dotnet.native.wasm" being in place of "godot.wasm".
	const loadPath = moduleConfig['locateFile']('dotnet.native.wasm');
	// Get preloaded wasm.
	let preloadedWasm = moduleConfig['getPreloadedWasm']();

	const perfMark = (name) => {
		if (typeof performance !== 'undefined') {
			performance.mark(name);
		}
	};

	// 2dog: optional precompressed-sibling fallback ('.gz' + in-page inflate,
	// for hosts that serve everything uncompressed) and cumulative download
	// reporting for the _framework payload the engine preloader never sees.
	// Only binary resources are taken over - script modules must stay URLs so
	// the runtime can import() them.
	const gzSuffix = moduleConfig['godotPrecompressedSuffix'] || '';
	const onDotnetBytes = moduleConfig['godotOnDotnetProgress'];
	let dotnetLoaded = 0;
	const countedResponse = (response) => {
		if (!onDotnetBytes || !response.body) {
			return response;
		}
		return new Response(response.body.pipeThrough(new TransformStream({
			transform(chunk, controller) {
				dotnetLoaded += chunk.byteLength;
				onDotnetBytes(dotnetLoaded);
				controller.enqueue(chunk);
			},
		})), { 'headers': response.headers });
	};
	const loadBinaryResource = (url) => {
		const plain = () => fetch(url).then(countedResponse);
		if (!gzSuffix || typeof DecompressionStream === 'undefined') {
			return plain();
		}
		return fetch(url + gzSuffix).then((response) => {
			if (!response.ok || !response.body) {
				return plain();
			}
			return countedResponse(new Response(response.body.pipeThrough(new DecompressionStream('gzip'))));
		}, plain);
	};

	// dynamic module import.
	perfMark('2dog:dotnet-import');
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
			const resourceUrl = encodeDotnetResourceUrl(_defaultUri, name);
			if ((gzSuffix || onDotnetBytes) && /\.(wasm|dat|pdb)$/.test(name)) {
				return loadBinaryResource(resourceUrl);
			}
			// Preserve the default loader path unless the resource name needed escaping.
			return resourceUrl === _defaultUri ? null : resourceUrl;
		});

	await dotnet.download();
	perfMark('2dog:dotnet-downloaded');
	const { setModuleImports, getAssemblyExports, getConfig, runMain, Module } = await dotnet.create();
	perfMark('2dog:dotnet-created');

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
