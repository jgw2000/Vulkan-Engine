#include "Engine.h"

#include <Platform/Platform.h>

#include <Core/Debug.h>

namespace VE {

using namespace backend;

namespace {

backend::Platform::DriverConfig GetDriverConfig(FEngine* instance) {
    Platform::DriverConfig driverConfig{

    };

    return driverConfig;
}

}

struct Engine::BuilderDetails {
    Backend backend = Backend::DEFAULT;
    Platform* platform = nullptr;
    FeatureLevel feature_level = FeatureLevel::FEATURE_LEVEL_1;
    void* shared_context = nullptr;
    bool paused = false;
};

// ------------------------------------------------------------------------------------------------
// Engine::Builder
// ------------------------------------------------------------------------------------------------
Engine::Builder::Builder() noexcept = default;
Engine::Builder::~Builder() noexcept = default;
Engine::Builder::Builder(const Builder& rhs) noexcept = default;
Engine::Builder::Builder(Builder&& rhs) noexcept = default;
Engine::Builder& Engine::Builder::operator=(const Builder& rhs) noexcept = default;
Engine::Builder& Engine::Builder::operator=(Builder&& rhs) noexcept = default;

Engine::Builder& Engine::Builder::Backend(const Engine::Backend backend) noexcept {
    impl->backend = backend;
    return *this;
}

Engine::Builder& Engine::Builder::Platform(backend::Platform* platform) noexcept {
    impl->platform = platform;
    return *this;
}

Engine::Builder& Engine::Builder::FeatureLevel(const backend::FeatureLevel featureLevel) noexcept {
    impl->feature_level = featureLevel;
    return *this;
}

Engine::Builder& Engine::Builder::SharedContext(void* sharedContext) noexcept {
    impl->shared_context = sharedContext;
    return *this;
}

Engine::Builder& Engine::Builder::Paused(const bool paused) noexcept {
    impl->paused = paused;
    return *this;
}

Engine* Engine::Builder::Build() const {
    return FEngine::Create(*this);
}

// ------------------------------------------------------------------------------------------------
// FEngine
// ------------------------------------------------------------------------------------------------
FEngine::FEngine(const Builder& builder) {

}

Engine* FEngine::Create(const Builder& builder) {
    FEngine* instance = new FEngine(builder);

    // initialize all fields that need an instance of FEngine
    // (this cannot be done safely in the ctor)

    // Normally we launch a thread and create the context and Driver from there (see FEngine::loop).
    // In the single-threaded case, wo do so in the here and now.
#if defined(ENGINE_SUPPORT_THREADING)

#else
    Platform* platform = builder->platform;
    void* const sharedContext = builder->shared_context;

    if (platform == nullptr) {
        LOG("Selected backend not supported in this build.");
        delete instance;
        return nullptr;
    }
    instance->driver = platform->CreateDriver(sharedContext, GetDriverConfig(instance));
#endif

    // now we can initialize the largest part of the engine
    instance->init();

#if !defined(ENGINE_SUPPORT_THREADING)
    instance->Execute();
#endif

    return instance;
}

void FEngine::Destory(FEngine* engine) {
    if (engine) {
        engine->shutdown();
        delete engine;
    }
}

bool FEngine::Execute() {
    return true;
}

void FEngine::init() {

}

void FEngine::shutdown() {

}

} // namespace VE