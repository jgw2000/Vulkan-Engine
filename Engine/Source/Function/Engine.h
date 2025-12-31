#pragma once

#include <Platform/Platform.h>

#include <Core/DriverEnums.h>
#include <Core/PrivateImplementation-impl.h>

namespace VE {

namespace backend {
class Driver;
} // namespace driver

class Engine {
    struct BuilderDetails;
public:
    using Platform = backend::Platform;
    using Backend = backend::Backend;
    using FeatureLevel = backend::FeatureLevel;

    /**
     * Engine::Builder is used to create a new Engine.
     */
    class Builder : public PrivateImplementation<BuilderDetails> {
        friend struct BuilderDetails;
        friend class FEngine;
    public:
        Builder() noexcept;
        Builder(const Builder& rhs) noexcept;
        Builder(Builder&& rhs) noexcept;
        ~Builder() noexcept;
        Builder& operator=(const Builder& rhs) noexcept;
        Builder& operator=(Builder&& rhs) noexcept;

        /**
         * @param backend Which driver backend to use
         * @return A reference to this Builder for chaining calls.
         */
        Builder& Backend(Backend backend) noexcept;

        /**
         * @param platform A pointer to an object that implements Platform. If this is
         *                 provided, then this object is used to create the hardware
         *                 context and expose platform features to it.
         * 
         *                 All methods of this interface are called from render thread,
         *                 which is different from the main thread.
         * 
         *                 The lifetime of \p platform must exceed the lifetime of the
         *                 Engine object.
         * 
         * @return A reference to this Builder for chaining calls.
         */
        Builder& Platform(Platform* platform) noexcept;

        /**
         * @param feature_level The feature level at which initialize the engine.
         * @return A reference to this Builder for chaining calls.
         */
        Builder& FeatureLevel(FeatureLevel featureLevel) noexcept;

        /**
         * @param shared_context A platform-dependant context used as a shared context
         *                       when creating engine's internal context.
         * 
         * @return A reference to this Builder for chaining calls.
         */
        Builder& SharedContext(void* sharedContext) noexcept;

        /**
         * @param paused Whether to start the rendering thread paused.
         * @return A reference to this Builder for chaining calls.
         */
        Builder& Paused(bool paused) noexcept;

        /**
         * Creates an instance of Engine.
         * 
         * @return A pointer to the newly created Engine, or nullptr if the Engine couldn't be
         *         created.
         *         nullptr if the GPU driver couldn't be initialized, for instance if it doesn't
         *         support the right version of OpenGL or OpenGL ES.
         */
        Engine* Build() const;
    };

protected:
    Engine() noexcept = default;
    ~Engine() = default;

public:
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;
};

/*
 * Concrete implementation of the Engine interface. This keeps track of all hardware resources
 * for a given context.
 */
class FEngine : public Engine {
public:
    static Engine* Create(const Builder& builder);
    static void Destory(FEngine* engine);

    bool Execute();

    backend::Driver& GetDriver() const noexcept { return *driver; }

private:
    explicit FEngine(const Builder& builder);
    void init();
    void shutdown();

    backend::Driver* driver = nullptr;
};

} // namespace VE