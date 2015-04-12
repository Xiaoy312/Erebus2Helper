#pragma once
class ServiceBase
{
public:
    virtual void Initialize() = 0;
    virtual void DelayedInitialize() { }
    virtual void Start() { }
    virtual void Stop() { }
    virtual void Pause() { }
};