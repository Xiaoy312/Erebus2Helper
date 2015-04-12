#pragma once
class ServiceManager
{
public:
    static void Initialize();
    static void StartServices();
    static void PauseServices();
    static void StopServices();

    template<typename TService>
    static TService* GetService()
    {
        static_assert(std::is_base_of<ServiceBase, TService>::value, "TService must extend ServiceBase");

        return (TService*)GetServiceImpl(&typeid(TService));
    }

private:
    static ServiceBase* GetServiceImpl(const std::type_info *type);
};

