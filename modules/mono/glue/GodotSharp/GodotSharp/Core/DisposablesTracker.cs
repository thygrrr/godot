using System;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;
using Godot.NativeInterop;

#nullable enable

namespace Godot
{
    internal static class DisposablesTracker
    {
        [UnmanagedCallersOnly]
        internal static void OnGodotShuttingDown(godot_bool preserveStringNames)
        {
            try
            {
                OnGodotShuttingDownImpl(preserveStringNames.ToBool());
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
            }
        }

        private static void OnGodotShuttingDownImpl(bool preserveStringNames)
        {
            bool isStdoutVerbose;

            try
            {
                isStdoutVerbose = OS.IsStdOutVerbose();
            }
            catch (ObjectDisposedException)
            {
                // OS singleton already disposed. Maybe OnUnloading was called twice.
                isStdoutVerbose = false;
            }

            if (isStdoutVerbose)
                GD.Print("Unloading: Disposing tracked instances...");

            try
            {
                // Dispose Godot Objects first, and only then dispose other disposables
                // like StringName, NodePath, Godot.Collections.Array/Dictionary, etc.
                // The Godot Object Dispose() method may need any of the later instances.

                foreach (WeakReference<GodotObject> item in GodotObjectInstances.Keys)
                {
                    if (item.TryGetTarget(out GodotObject? self))
                        self.Dispose();
                }

                foreach (WeakReference<IDisposable> item in OtherInstances.Keys)
                {
                    if (item.TryGetTarget(out IDisposable? self))
                    {
                        // 2dog: native StringNames survive libgodot restart cleanup.
                        if (preserveStringNames && self is StringName)
                            continue;

                        self.Dispose();
                    }
                }
            }
            finally
            {
                EndEngineLifetime(preserveStringNames);
            }

            if (isStdoutVerbose)
                GD.Print("Unloading: Finished disposing tracked instances.");
        }

        // 2dog: registration means the running engine owns the native side. Whatever the loops above missed (created
        // meanwhile, or skipped after a throwing Dispose) is dropped, so its later release is skipped; the wait lets
        // finalizers that claimed their registration first finish while the engine is still alive.
        private static void EndEngineLifetime(bool preserveStringNames)
        {
            GodotObjectInstances.Clear();

            foreach (WeakReference<IDisposable> item in OtherInstances.Keys)
            {
                if (preserveStringNames && item.TryGetTarget(out IDisposable? self) && self is StringName)
                    continue;

                OtherInstances.TryRemove(item, out _);
            }

            GC.WaitForPendingFinalizers();
        }

        private static ConcurrentDictionary<WeakReference<GodotObject>, byte> GodotObjectInstances { get; } =
            new();

        private static ConcurrentDictionary<WeakReference<IDisposable>, byte> OtherInstances { get; } =
            new();

        // 2dog: long weak references still reach instances awaiting finalization, so shutdown disposes those while the
        // engine is alive. The Unregister claim keeps a finalizer running concurrently from releasing twice.
        public static WeakReference<GodotObject> RegisterGodotObject(GodotObject godotObject)
        {
            var weakReferenceToSelf = new WeakReference<GodotObject>(godotObject, trackResurrection: true);
            GodotObjectInstances.TryAdd(weakReferenceToSelf, 0);
            return weakReferenceToSelf;
        }

        public static WeakReference<IDisposable> RegisterDisposable(IDisposable disposable)
        {
            var weakReferenceToSelf = new WeakReference<IDisposable>(disposable, trackResurrection: true);
            OtherInstances.TryAdd(weakReferenceToSelf, 0);
            return weakReferenceToSelf;
        }

        // 2dog: both return false once the instance's engine has shut down (EndEngineLifetime); the caller must then
        // leave the native side alone.
        public static bool UnregisterGodotObject(GodotObject godotObject, WeakReference<GodotObject> weakReferenceToSelf)
            => GodotObjectInstances.TryRemove(weakReferenceToSelf, out _);

        public static bool UnregisterDisposable(WeakReference<IDisposable> weakReference)
            => OtherInstances.TryRemove(weakReference, out _);
    }
}
