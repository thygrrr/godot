// SPDX-License-Identifier: MIT
// 2dog: this file is part of https://2dog.dev

// GDExtension side modules for static library builds. The host links Godot into a non-relocatable main module (the
// .NET runtime), so emscripten's dynamic linker (RELOCATABLE builds only) is unavailable. This loader instantiates
// a SIDE_MODULE against the main module instead; the host link must export every symbol the side modules import.
const GodotDylink = {
	$GodotDylink__deps: ['$addFunction', '$updateTableMap', '$functionsInTableMap', '$wasmTable', '$alignMemory', '$zeroMemory', '$UTF8ArrayToString', 'malloc'],
	$GodotDylink: {
		libs: [null],
		lastError: '',
		stackPointer: null,

		// The main module's __stack_pointer is internal, so side modules share their own shadow stack. Frames stay
		// LIFO per stack across main <-> side calls; only unwinding through side frames (longjmp) would leak it.
		STACK_SIZE: 1024 * 1024,

		metadata: function (module) {
			const sections = WebAssembly.Module.customSections(module, 'dylink.0');
			if (sections.length === 0) {
				throw new Error('not a side module (no dylink.0 section); build it with -sSIDE_MODULE');
			}
			const bytes = new Uint8Array(sections[0]);
			let offset = 0;
			const leb = () => {
				let ret = 0;
				let mul = 1;
				for (;;) {
					const b = bytes[offset++];
					ret += (b & 0x7f) * mul;
					mul *= 0x80;
					if (!(b & 0x80)) {
						return ret;
					}
				}
			};
			const str = () => {
				const len = leb();
				offset += len;
				return UTF8ArrayToString(bytes, offset - len, len);
			};
			const meta = { memorySize: 0, memoryAlign: 0, tableSize: 0, needed: [], weak: new Set(), tls: new Set() };
			while (offset < bytes.length) {
				const type = bytes[offset++];
				const size = leb();
				const end = offset + size;
				if (type === 1) { // WASM_DYLINK_MEM_INFO
					meta.memorySize = leb();
					meta.memoryAlign = leb();
					meta.tableSize = leb();
				} else if (type === 2) { // WASM_DYLINK_NEEDED
					for (let n = leb(); n > 0; n--) {
						meta.needed.push(str());
					}
				} else if (type === 3) { // WASM_DYLINK_EXPORT_INFO
					for (let n = leb(); n > 0; n--) {
						const name = str();
						if (leb() & 0x100) { // WASM_SYMBOL_TLS
							meta.tls.add(name);
						}
					}
				} else if (type === 4) { // WASM_DYLINK_IMPORT_INFO
					for (let n = leb(); n > 0; n--) {
						str(); // Module name.
						const name = str();
						if ((leb() & 0x3) === 0x1) { // WASM_SYMBOL_BINDING_WEAK
							meta.weak.add(name);
						}
					}
				}
				offset = end;
			}
			return meta;
		},

		has: function (obj, name) {
			return Object.prototype.hasOwnProperty.call(obj, name);
		},

		// A wasm export, a JS library function the main module imports, or a JS function exported to Module.
		mainSymbol: function (name) {
			if (GodotDylink.has(wasmExports, name)) {
				return wasmExports[name];
			}
			if (GodotDylink.has(wasmImports, name)) {
				return wasmImports[name];
			}
			return Module[`_${name}`];
		},

		sharedStackPointer: function () {
			if (!GodotDylink.stackPointer) {
				const low = _malloc(GodotDylink.STACK_SIZE);
				if (!low) {
					throw new Error('out of memory allocating the side module stack');
				}
				const high = (low + GodotDylink.STACK_SIZE) & ~15;
				GodotDylink.stackPointer = new WebAssembly.Global({ 'value': 'i32', 'mutable': true }, high);
				GodotDylink.stackPointer.high = high;
				GodotDylink.stackPointer.low = low;
			}
			return GodotDylink.stackPointer;
		},

		load: function (name, bytes) {
			// Like dlopen, a library that is still open is shared rather than instantiated twice.
			const open = GodotDylink.libs.findIndex((l) => l && l.name === name);
			if (open > 0) {
				GodotDylink.libs[open].refs++;
				return open;
			}

			// Compile before allocating: malloc may grow memory and detach the byte view.
			const module = new WebAssembly.Module(bytes);
			const meta = GodotDylink.metadata(module);
			if (meta.tls.size > 0) {
				throw new Error('thread-local exports are not supported (build the side module with threads=no)');
			}
			const needed = meta.needed.map((lib) => {
				const found = GodotDylink.libs.find((l) => l && l.name === lib);
				if (!found) {
					throw new Error(`needs '${lib}', which is not loaded (list it under [dependencies])`);
				}
				return found;
			});
			const fromNeeded = (sym) => {
				for (const lib of needed) {
					if (GodotDylink.has(lib.exports, sym)) {
						return lib.exports[sym];
					}
				}
				return undefined;
			};

			const memAlign = Math.pow(2, meta.memoryAlign);
			let memoryBase = 0;
			if (meta.memorySize) {
				const raw = _malloc(meta.memorySize + memAlign);
				if (!raw) {
					throw new Error('out of memory allocating the side module data');
				}
				memoryBase = alignMemory(zeroMemory(raw, meta.memorySize + memAlign), memAlign);
			}
			const tableBase = meta.tableSize ? wasmTable.length : 0;
			if (meta.tableSize) {
				wasmTable.grow(meta.tableSize);
			}

			let exports = null;
			const unresolved = [];
			const got = [];
			const env = {};
			const imports = { 'env': env, 'wasi_snapshot_preview1': env, 'GOT.mem': {}, 'GOT.func': {} };
			for (const imp of WebAssembly.Module.imports(module)) {
				const sym = imp.name;
				if (imp.module === 'GOT.mem' || imp.module === 'GOT.func') {
					// Filled after instantiation, before data relocations and constructors read them.
					const entry = new WebAssembly.Global({ 'value': 'i32', 'mutable': true }, 0);
					got.push({ entry, sym, func: imp.module === 'GOT.func' });
					imports[imp.module][sym] = entry;
				} else if (imp.module !== 'env' && imp.module !== 'wasi_snapshot_preview1') {
					throw new Error(`unsupported import module '${imp.module}' (${sym})`);
				} else if (imp.kind === 'memory') {
					env[sym] = wasmMemory;
				} else if (imp.kind === 'table') {
					env[sym] = wasmTable;
				} else if (sym === '__memory_base') {
					env[sym] = new WebAssembly.Global({ 'value': 'i32', 'mutable': false }, memoryBase);
				} else if (sym === '__table_base') {
					env[sym] = new WebAssembly.Global({ 'value': 'i32', 'mutable': false }, tableBase);
				} else if (sym === '__stack_pointer') {
					env[sym] = GodotDylink.sharedStackPointer();
				} else if (imp.kind === 'function') {
					let func = GodotDylink.mainSymbol(sym);
					if (typeof func !== 'function') {
						func = fromNeeded(sym);
					}
					if (typeof func !== 'function') {
						// The module may define it itself, or never call it: fail on call rather than on load.
						if (!meta.weak.has(sym)) {
							unresolved.push(sym);
						}
						func = (...args) => {
							const own = exports && exports[sym];
							if (typeof own !== 'function') {
								throw new Error(`${name}: called '${sym}', which the main module does not export`);
							}
							return own(...args);
						};
					}
					env[sym] = func;
				} else if (imp.kind === 'tag') {
					// Exception tags must be the main module's to interoperate; a private one still works locally.
					const tag = GodotDylink.mainSymbol(sym);
					env[sym] = tag instanceof WebAssembly.Tag ? tag : new WebAssembly.Tag({ 'parameters': ['i32'] });
				} else {
					const value = GodotDylink.mainSymbol(sym);
					if (value === undefined) {
						throw new Error(`unresolved ${imp.kind} import '${sym}' (not exported by the main module)`);
					}
					env[sym] = value;
				}
			}
			if (unresolved.length) {
				err(`GDExtension '${name}': imports not exported by the main module (calls will fail): ${unresolved.join(', ')}`);
			}

			const instance = new WebAssembly.Instance(module, imports);

			// Exported data symbols are relative to the module's memory base.
			exports = {};
			for (const [sym, value] of Object.entries(instance.exports)) {
				exports[sym] = value instanceof WebAssembly.Global ? value.value + memoryBase : value;
			}
			if (functionsInTableMap) {
				updateTableMap(tableBase, meta.tableSize);
			}

			// Own definitions win (like -Bsymbolic): extensions must never bind to each other's data.
			for (const { entry, sym, func } of got) {
				let value = GodotDylink.has(exports, sym) ? exports[sym] : undefined;
				if (value === undefined) {
					value = GodotDylink.mainSymbol(sym);
					if (value instanceof WebAssembly.Global) {
						value = value.value;
					}
				}
				if (value === undefined) {
					value = fromNeeded(sym);
				}
				if (value === undefined) {
					if (meta.weak.has(sym)) {
						continue;
					}
					throw new Error(`unresolved ${func ? 'function' : 'data'} symbol '${sym}' (not exported by the main module)`);
				}
				if (func) {
					if (typeof value !== 'function') {
						throw new Error(`symbol '${sym}' is not a function`);
					}
					// addFunction reuses an existing table slot, keeping function pointers comparable.
					entry.value = addFunction(value);
				} else {
					if (typeof value !== 'number') {
						throw new Error(`symbol '${sym}' is not a data address`);
					}
					entry.value = value;
				}
			}

			if (instance.exports['__set_stack_limits']) {
				const sp = GodotDylink.sharedStackPointer();
				instance.exports['__set_stack_limits'](sp.high, sp.low);
			}
			if (instance.exports['__wasm_apply_data_relocs']) {
				instance.exports['__wasm_apply_data_relocs']();
			}
			if (instance.exports['__wasm_call_ctors']) {
				instance.exports['__wasm_call_ctors']();
			}

			GodotDylink.libs.push({ name, exports, refs: 1 });
			return GodotDylink.libs.length - 1;
		},

		symbol: function (handle, sym) {
			const lib = GodotDylink.libs[handle];
			if (!lib) {
				throw new Error(`invalid library handle ${handle}`);
			}
			if (!GodotDylink.has(lib.exports, sym)) {
				throw new Error(`symbol '${sym}' not found in '${lib.name}'`);
			}
			const value = lib.exports[sym];
			return typeof value === 'function' ? addFunction(value) : value;
		},
	},

	godot_js_dylink_open__proxy: 'sync',
	godot_js_dylink_open__sig: 'ippi',
	godot_js_dylink_open: function (p_name, p_bytes, p_size) {
		const name = UTF8ToString(p_name);
		try {
			return GodotDylink.load(name, HEAPU8.subarray(p_bytes, p_bytes + p_size));
		} catch (e) {
			GodotDylink.lastError = e.message || String(e);
			return 0;
		}
	},

	godot_js_dylink_symbol__proxy: 'sync',
	godot_js_dylink_symbol__sig: 'pip',
	godot_js_dylink_symbol: function (p_handle, p_symbol) {
		try {
			return GodotDylink.symbol(p_handle, UTF8ToString(p_symbol));
		} catch (e) {
			GodotDylink.lastError = e.message || String(e);
			return 0;
		}
	},

	godot_js_dylink_close__proxy: 'sync',
	godot_js_dylink_close__sig: 'vi',
	godot_js_dylink_close: function (p_handle) {
		// Instances cannot be unloaded; the last close drops the handle, so a later open instantiates afresh.
		const lib = GodotDylink.libs[p_handle];
		if (p_handle > 0 && lib && --lib.refs === 0) {
			GodotDylink.libs[p_handle] = null;
		}
	},

	godot_js_dylink_error__deps: ['$stringToNewUTF8'],
	godot_js_dylink_error__proxy: 'sync',
	godot_js_dylink_error__sig: 'p',
	godot_js_dylink_error: function () {
		return stringToNewUTF8(GodotDylink.lastError);
	},
};

autoAddDeps(GodotDylink, '$GodotDylink');
addToLibrary(GodotDylink);
