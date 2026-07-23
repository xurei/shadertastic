#ifdef _WIN32
#include "win_dll_overrides.h"
#include <obs-module.h>
#include <string>
#include <filesystem>

FARPROC WINAPI delay_load_hook(const unsigned dliNotify, PDelayLoadInfo pdli) {
    if (dliNotify == dliNotePreLoadLibrary) {
        HMODULE module = GetModuleHandleA("shadertastic.dll");
        char dll_path[MAX_PATH];
        GetModuleFileNameA(module, dll_path, MAX_PATH);

        std::filesystem::path module_dir = dll_path;
        module_dir = module_dir.parent_path();
        std::filesystem::path onnx_dll_path = module_dir / "shadertastic_lib" / "onnxruntime.dll";
        blog(LOG_INFO, "LOADING ONNX RUNTIME : %s", onnx_dll_path.c_str());
        const HMODULE h = LoadLibraryW(onnx_dll_path.c_str());
        if (!h) {
            DWORD err = GetLastError();
            blog(LOG_ERROR, "ONNX LoadLibrary failed: %lu", err);
        }
        return reinterpret_cast<FARPROC>(h);
    }
    return nullptr;
}

ExternC const PfnDliHook __pfnDliNotifyHook2 = delay_load_hook;
#endif