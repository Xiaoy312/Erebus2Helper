#include "stdafx.h"
#include "ServiceManager.h"

//UI
#include "UI\Console.h"

//Services
#include "Hack\Patch.h"
#include "Hack\MapNuke.h"
#include "Hack\InventoryHelper.h"
#include "Packet\PacketAnalyzer.h"

struct TypeInfoComparer// : public std::binary_function<std::type_info, std::type_info, bool>
{
    bool operator()(const std::type_info* a, const std::type_info* b) const
    {
        return a->hash_code() < b->hash_code();
    }
};
static std::map<const std::type_info*, ServiceBase*, TypeInfoComparer> Services;

template<typename TService>
void RegisterService()
{
    static_assert(std::is_base_of<ServiceBase, TService>::value, "TService must extend ServiceBase");
    Services[&typeid(TService)] = new TService();
}

void InitializeServices()
{
    for(auto it = Services.begin(); it != Services.end(); ++it)
        it->second->Initialize();
}
void InitializeUI()
{
    Console::Initialize();
}
void ServiceManager::Initialize()
{
    RegisterService<Patch>();
    RegisterService<MapNuke>();
    RegisterService<InventoryHelper>();

    InitializeServices();
    InitializeUI();
}
void ServiceManager::StartServices()
{
    for(auto it = Services.begin(); it != Services.end(); ++it)
        it->second->Start();
}
void ServiceManager::PauseServices()
{
    for(auto it = Services.begin(); it != Services.end(); ++it)
        it->second->Pause();
}
void ServiceManager::StopServices()
{
    for(auto it = Services.begin(); it != Services.end(); ++it)
        it->second->Stop();
}

ServiceBase* ServiceManager::GetServiceImpl(const std::type_info *type)
{
    auto result = Services.find(type);
    if (result == Services.end())
        return nullptr;
    else
        return result->second;
}