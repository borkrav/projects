#include <BRRender.h>

#include <cstdlib>
#include <stdexcept>

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