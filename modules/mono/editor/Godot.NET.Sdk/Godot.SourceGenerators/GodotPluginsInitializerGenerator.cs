using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.Text;

namespace Godot.SourceGenerators
{
    [Generator]
    public class GodotPluginsInitializerGenerator : ISourceGenerator
    {
        public void Initialize(GeneratorInitializationContext context)
        {
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (context.IsGodotToolsProject() || context.IsGodotSourceGeneratorDisabled("GodotPluginsInitializer"))
                return;

            string source =
                @"using System;
using System.Runtime.InteropServices;
using Godot.Bridge;
using Godot.NativeInterop;

namespace GodotPlugins.Game
{
    internal static partial class Main
    {
        private static bool _resolverRegistered;
        private static bool _scriptsRegistered;

        [UnmanagedCallersOnly(EntryPoint = ""godotsharp_game_main_init"")]
        private static godot_bool InitializeFromGameProject(IntPtr godotDllHandle, IntPtr outManagedCallbacks,
            IntPtr unmanagedCallbacks, int unmanagedCallbacksSize)
        {
            try
            {
                DllImportResolver dllImportResolver = new GodotDllImportResolver(godotDllHandle).OnResolveDllImport;

                var coreApiAssembly = typeof(global::Godot.GodotObject).Assembly;

                if (!_resolverRegistered)
                {
                    NativeLibrary.SetDllImportResolver(coreApiAssembly, dllImportResolver);
                    _resolverRegistered = true;
                }

                NativeFuncs.Initialize(unmanagedCallbacks, unmanagedCallbacksSize);

                ManagedCallbacks.Create(outManagedCallbacks);

                // Register once. The host replays script registrations on restart after
                // this initializer has refreshed the native and managed callbacks.
                if (!_scriptsRegistered)
                {
                    ScriptManagerBridge.LookupScriptsInAssembly(typeof(global::GodotPlugins.Game.Main).Assembly);
                    _scriptsRegistered = true;
                }

                return godot_bool.True;
            }
            catch (Exception e)
            {
                global::System.Console.Error.WriteLine(e);
                return false.ToGodotBool();
            }
        }

#if LIBGODOT_ENABLED
        internal static unsafe IntPtr GetInitializePointer()
        {
            return (IntPtr)(delegate* unmanaged<IntPtr, IntPtr, IntPtr, int, godot_bool>)&InitializeFromGameProject;
        }
#endif
    }
}
";

            context.AddSource("GodotPlugins.Game.generated",
                SourceText.From(source, Encoding.UTF8));
        }
    }
}
