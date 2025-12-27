
#pragma once

namespace VE {

class App {
public:
    static App& get();

    ~App();

    void Run();

private:
    App();
};

} // namespace VE