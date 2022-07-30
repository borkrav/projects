#include <cstdlib>
#include <stdexcept>

#include <VkRender.h>

int main()
{
    VkRender app;

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