// #################
// AI GENERATED FILE
// #################

#include "core/Log.h"

#include <cstdio>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#endif



namespace pt::log {

    void write(Level level, std::string_view message) {
        static std::mutex mutex;
        std::scoped_lock lock(mutex);

        const char* tag = level == Level::Info ? "[info] " : level == Level::Warn ? "[warn] " : "[error] ";
        std::FILE* out = level == Level::Error ? stderr : stdout;
        std::fprintf(out, "%s%.*s\n", tag, static_cast<int>(message.size()), message.data());
        std::fflush(out);

#if defined(_WIN32)
        // Also show up in the Visual Studio Output window.
        std::string line = std::string(tag) + std::string(message) + "\n";
        OutputDebugStringA(line.c_str());
#endif
    }

} // namespace pt::log