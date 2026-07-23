// 2dog: non-mono library builds have no C main, but Emscripten requires this
// exported runtime symbol. Mono builds use modules/mono/web/callMain_stub.js.
const callMain = function (_args) { // eslint-disable-line no-unused-vars
	throw new Error('"callMain" is not implemented.');
};
