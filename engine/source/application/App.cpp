#include <application/App.h>

namespace VE {

App& App::get() {
    static App app;
    return app;
}

App::App() {

}

App::~App() {

}

void App::Run() {
    
}

}