#include "Application/App.h"

int main()
{
    VE::Config config;
    VE::App::Get().Run(config);
    return 0;
}