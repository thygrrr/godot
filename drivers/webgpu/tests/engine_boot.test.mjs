import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import vm from 'node:vm';

const configSource = readFileSync(new URL('../../../platform/web/js/engine/config.js', import.meta.url), 'utf8');
const engineSource = readFileSync(new URL('../../../platform/web/js/engine/engine.js', import.meta.url), 'utf8');

function harness(gpu, unavailableReason = '') {
    const modules = [];
    const context = vm.createContext({
        Response, console, window: {}, navigator: { gpu },
        requestAnimationFrame() {},
        Features: {},
        Preloader: class {
            loadPromise() { return Promise.resolve(new Response(new Uint8Array())); }
            setProgressFunc() {}
        },
        Godot(moduleConfig) {
            modules.push(moduleConfig);
            return Promise.resolve({ initFS: () => Promise.resolve() });
        },
    });
    vm.runInContext(configSource + '\n' + engineSource.replace('___GODOT_WEBGPU_UNAVAILABLE_REASON___', JSON.stringify(unavailableReason)), context);
    return { Engine: context.window.Engine, modules };
}

test('existing WebGL init works without navigator.gpu', async () => {
    const { Engine, modules } = harness(undefined);
    await new Engine({ renderingDriver: 'opengl3' }).init('godot');
    assert.equal(modules.length, 1);
    assert.equal('preinitializedWebGPUDevice' in modules[0], false);
});

test('direct init waits for the WebGPU device and requests adapter storage limits', async () => {
    let requestCount = 0;
    const device = { lost: new Promise(() => {}), addEventListener() {} };
    const { Engine, modules } = harness({
        async requestAdapter() {
            return {
                features: new Set(),
                limits: { maxStorageBuffersPerShaderStage: 16 },
                async requestDevice(descriptor) {
                    requestCount++;
                    assert.equal(modules.length, 0);
                    assert.equal(descriptor.requiredLimits.maxStorageBuffersPerShaderStage, 16);
                    return device;
                },
            };
        },
    });
    const engine = new Engine({ renderingDriver: 'webgpu' });
    await Promise.all([engine.init('godot'), engine.init('godot')]);
    assert.equal(requestCount, 1);
    assert.equal(modules.length, 1);
    assert.equal(modules[0].preinitializedWebGPUDevice, device);
});

test('a caller-supplied GPUDevice is passed through without requesting another', async () => {
    const device = {};
    const { Engine, modules } = harness({ requestAdapter() { throw new Error('Unexpected device request'); } });
    await new Engine({ renderingDriver: 'webgpu', preinitializedWebGPUDevice: device }).init('godot');
    assert.equal(modules[0].preinitializedWebGPUDevice, device);
});

test('unsupported WebGPU rejects before instantiating the .NET runtime', async () => {
    const { Engine, modules } = harness(undefined);
    await assert.rejects(new Engine({ renderingDriver: 'webgpu' }).init('godot'), /WebGPU is not supported/);
    assert.equal(modules.length, 0);
});

test('the browser requests the same optional features and all limits as the native backend', async () => {
    let descriptor = null;
    const device = { lost: new Promise(() => {}), addEventListener() {} };
    const { Engine } = harness({
        async requestAdapter() {
            return {
                features: new Set(['indirect-first-instance', 'timestamp-query', 'depth32float-stencil8']),
                info: { vendor: 'intel', device: 'GPU test adapter' },
                limits: { maxComputeWorkgroupSizeX: 256 },
                async requestDevice(value) {
                    descriptor = value;
                    return device;
                },
            };
        },
    });
    await new Engine({ renderingDriver: 'webgpu' }).init('godot');
    assert.deepEqual([...descriptor.requiredFeatures].sort(), ['depth32float-stencil8', 'indirect-first-instance']);
    assert.equal(descriptor.requiredLimits.maxComputeWorkgroupSizeX, 256);
    assert.equal(device.godotAdapterInfo.device, 'GPU test adapter');
});

for (const reason of [
    'WebGPU is unavailable in multithreaded browser runtimes.',
    'WebGPU is unavailable in double-precision runtimes.',
    'WebGPU is not included in this runtime.',
]) {
    test(reason + ' rejects before adapter requests or runtime creation', async () => {
        const { Engine, modules } = harness({ requestAdapter() { throw new Error('Unexpected adapter request'); } }, reason);
        await assert.rejects(Engine.requestWebGPUDevice(), { message: reason });
        await assert.rejects(new Engine({ renderingDriver: 'webgpu', preinitializedWebGPUDevice: {} }).init('godot'), { message: reason });
        await assert.rejects(new Engine({ args: ['--rendering-driver', 'webgpu'] }).init('godot'), { message: reason });
        assert.equal(modules.length, 0);
        await new Engine({ renderingDriver: 'opengl3' }).init('godot');
        assert.equal(modules.length, 1);
    });
}

test('an argv-only WebGPU request initializes the device', async () => {
    const device = { lost: new Promise(() => {}), addEventListener() {} };
    const { Engine, modules } = harness({ async requestAdapter() {
        return { features: new Set(), limits: {}, async requestDevice() { return device; } };
    } });
    await new Engine({ args: ['--rendering-driver', 'opengl3', '--rendering-driver', 'webgpu'] }).init('godot');
    assert.equal(modules[0].preinitializedWebGPUDevice, device);
});
