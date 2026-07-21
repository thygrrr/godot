// 2dog: non-mono library builds. Exporting "callMain" via
// EXPORTED_RUNTIME_METHODS requires the symbol to exist, but emscripten only
// generates it when it detects a C main (the library build has none).
// Twin of modules/mono/web/callMain_stub.js, which covers mono builds.
const callMain = function (_args) { // eslint-disable-line no-unused-vars
	throw new Error('"callMain" is not implemented.');
};
