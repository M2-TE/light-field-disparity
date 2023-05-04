#include "app.hpp"
#include "pch.hpp"

int main(int argc, char *argv[])
{
    DEBUG_ONLY(VMI_LOG("Running in Debug mode."));
    Application app;
    app.run();
}