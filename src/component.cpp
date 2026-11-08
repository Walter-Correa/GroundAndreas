#include <exception>
#include <stdexcept>
#include <cstdarg>
#include <cstdio>
#include <iostream>

#if defined(_WIN32) || defined(WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>

#include "common.h"
#include "natives.h"
#include "heightmap.h"
#include "version.h"

logprintf_t logprintf = nullptr;

IVehiclesComponent* g_vehiclesComponent = nullptr;

ICore* g_core = nullptr;

static void ComponentLogprintf(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (g_core != nullptr)
    {
        g_core->printLn("%s", buffer);
    }
    else
    {
        std::cout << buffer << std::endl;
    }
}

// T13 (B10): Proper random 64-bit UID for open.mp component
static const UID GroundAndreasComponent_UID = UID(0x7a8c4e512b930d64ULL);

class GroundAndreasComponent final : public IComponent, public PawnEventHandler
{
private:
    ICore* core_ = nullptr;
    IPawnComponent* pawn_ = nullptr;

public:
    PROVIDE_UID(GroundAndreasComponent_UID);

    StringView componentName() const override
    {
        return "GroundAndreas";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(GROUNDANDREAS_VERSION_MAJOR, GROUNDANDREAS_VERSION_MINOR, GROUNDANDREAS_VERSION_PATCH, 0);
    }

    void onLoad(ICore* c) override
    {
        try
        {
            core_ = c;
            g_core = c;
            logprintf = ComponentLogprintf;

            // T13 (B9): Process environment sentinel check
#if defined(_WIN32) || defined(WIN32)
            char envBuf[8] = { 0 };
            if (GetEnvironmentVariableA("GROUNDANDREAS_LOADED", envBuf, sizeof(envBuf)) > 0)
            {
                core_->printLn("  [GroundAndreas] WARNING: Another GroundAndreas instance (plugin or component) is already active in this process!");
            }
            SetEnvironmentVariableA("GROUNDANDREAS_LOADED", "1");
#else
            if (getenv("GROUNDANDREAS_LOADED") != nullptr)
            {
                core_->printLn("  [GroundAndreas] WARNING: Another GroundAndreas instance (plugin or component) is already active in this process!");
            }
            setenv("GROUNDANDREAS_LOADED", "1", 1);
#endif

            core_->printLn("--------------------------------------------------");
            core_->printLn("  GroundAndreas v%s (open.mp Component)", GROUNDANDREAS_VERSION_STRING);
            core_->printLn("  Ultra-Fast Memory-Mapped Heightmap Engine");
            core_->printLn("--------------------------------------------------");

            if (HeightMap::Instance().Load("scriptfiles/SAfull.hmap") ||
                HeightMap::Instance().Load("SAfull.hmap"))
            {
                core_->printLn("  [GroundAndreas] Map loaded: %s", HeightMap::Instance().GetLoadedPath().c_str());
            }
            else
            {
                core_->printLn("  [GroundAndreas] Notice: SAfull.hmap not found in scriptfiles/. Call GroundAndreas_Init() from Pawn if needed.");
            }
        }
        catch (const std::exception& e)
        {
            if (core_ != nullptr) core_->printLn("[GroundAndreas] Exception in onLoad: %s", e.what());
        }
        catch (...)
        {
        }
    }

    void onInit(IComponentList* components) override
    {
        try
        {
            pawn_ = components->queryComponent<IPawnComponent>();
            g_vehiclesComponent = components->queryComponent<IVehiclesComponent>();
            
            if (pawn_ != nullptr)
            {
                pAMXFunctions = const_cast<void**>(pawn_->getAmxFunctions().data());
                pawn_->getEventDispatcher().addEventHandler(this);
            }
        }
        catch (const std::exception& e)
        {
            if (core_ != nullptr) core_->printLn("[GroundAndreas] Exception in onInit: %s", e.what());
        }
        catch (...)
        {
        }
    }

    void onReady() override
    {
    }

    void onFree(IComponent* component) override
    {
        try
        {
            if (component == pawn_)
            {
                pawn_ = nullptr;
            }
            else if (component == g_vehiclesComponent)
            {
                g_vehiclesComponent = nullptr;
            }
        }
        catch (...)
        {
        }
    }

    void free() override
    {
        try
        {
#if defined(_WIN32) || defined(WIN32)
            SetEnvironmentVariableA("GROUNDANDREAS_LOADED", nullptr);
#else
            unsetenv("GROUNDANDREAS_LOADED");
#endif
            HeightMap::Instance().Unload();

            if (pawn_ != nullptr)
            {
                pawn_->getEventDispatcher().removeEventHandler(this);
            }
        }
        catch (...)
        {
        }
        delete this;
    }

    void reset() override
    {
    }

    // PawnEventHandler
    void onAmxLoad(IPawnScript& script) override
    {
        try
        {
            AMX* amx = script.GetAMX();
            amx_Register(amx, PluginNatives, -1);
        }
        catch (const std::exception& e)
        {
            if (core_ != nullptr) core_->printLn("[GroundAndreas] Exception in onAmxLoad: %s", e.what());
        }
        catch (...)
        {
        }
    }

    void onAmxUnload(IPawnScript& script) override
    {
    }
};

COMPONENT_ENTRY_POINT()
{
    try
    {
        return new GroundAndreasComponent();
    }
    catch (...)
    {
        return nullptr;
    }
}
