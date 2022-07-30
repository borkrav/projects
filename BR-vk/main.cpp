#include <cstdlib>
#include <stdexcept>

#include "VkInstance.h"

int main()
{
    VulkanEngine app;

    try
    {
        app.run();
    }
    catch ( const std::exception& e )
    {
        printf( "%s\n", e.what() );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}