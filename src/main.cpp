#include "Application.h"
#include <exception>
#include <iostream>

int main()
{
    std::cout << "Rafedit" << std::endl;
    try
    {
        
        Application app;
        return app.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}