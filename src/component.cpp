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

logprintf_t logprintf = nullptr;

static ICore* g_core = nullptr;

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

// SAGround in hex: 0x47726f756e64416e
static const UID GroundAndreasComponent_UID = UID(0x47726f756e64416e);

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
        return SemanticVersion(1, 0, 0, 0);
    }

    void onLoad(ICore* c) override
    {
        core_ = c;
        g_core = c;
        logprintf = ComponentLogprintf;

        core_->printLn("--------------------------------------------------");
        core_->printLn("  GroundAndreas v1.0.0 (open.mp Component)");
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

    void onInit(IComponentList* components) override
    {
        pawn_ = components->queryComponent<IPawnComponent>();
        if (pawn_ != nullptr)
        {
            pAMXFunctions = const_cast<void**>(pawn_->getAmxFunctions().data());
            pawn_->getEventDispatcher().addEventHandler(this);
        }
    }

    void onReady() override
    {
    }

    void onFree(IComponent* component) override
    {
        if (component == pawn_)
        {
            pawn_ = nullptr;
        }
    }

    void free() override
    {
        HeightMap::Instance().Unload();

        if (pawn_ != nullptr)
        {
            pawn_->getEventDispatcher().removeEventHandler(this);
        }
        delete this;
    }

    void reset() override
    {
    }

    // PawnEventHandler
    void onAmxLoad(IPawnScript& script) override
    {
        AMX* amx = script.GetAMX();
        amx_Register(amx, PluginNatives, -1);
    }

    void onAmxUnload(IPawnScript& script) override
    {
    }
};

COMPONENT_ENTRY_POINT()
{
    return new GroundAndreasComponent();
}
