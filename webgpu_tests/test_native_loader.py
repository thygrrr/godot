import importlib.util
import unittest
from pathlib import Path


class NativeLoaderTests(unittest.TestCase):
    def test_forwarders_preserve_pointer_void_and_callback_arguments(self):
        spec = importlib.util.spec_from_file_location(
            "native_loader", Path(__file__).resolve().parents[1] / "drivers/webgpu/native_loader.py"
        )
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        output = module.generate_loader(
            """
WGPU_EXPORT void wgpuRelease(WGPUDevice device) WGPU_FUNCTION_ATTRIBUTE;
WGPU_EXPORT void const * wgpuRead(WGPUBuffer buffer, size_t offset) WGPU_FUNCTION_ATTRIBUTE;
WGPU_EXPORT WGPUFuture wgpuRequest(WGPU_NULLABLE WGPUOptions const * options, WGPUCallbackInfo callbackInfo) WGPU_FUNCTION_ATTRIBUTE;
WGPU_EXPORT WGPUInstance wgpuCreate(void) WGPU_FUNCTION_ATTRIBUTE;
""",
            "WGPUInstance godot_webgpu_create_instance(WGPUInstanceDescriptor const * descriptor);",
        )
        self.assertIn("return dawn_wgpuRelease(device);", output)
        self.assertIn("return dawn_wgpuRead(buffer, offset);", output)
        self.assertIn("return dawn_wgpuRequest(options, callbackInfo);", output)
        self.assertIn("return dawn_wgpuCreate();", output)
        self.assertIn("return dawn_godot_webgpu_create_instance(descriptor);", output)
        self.assertIn("dlclose(library); return false;", output)


if __name__ == "__main__":
    unittest.main()
