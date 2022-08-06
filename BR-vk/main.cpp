#include <cstdlib>
#include <stdexcept>

#include <BRRender.h>

int main()
{
    BR::BRRender app;

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