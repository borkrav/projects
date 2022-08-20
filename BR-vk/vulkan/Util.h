#pragma once

#include <BRAppState.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define CHECK_SUCCESS( result ) checkSuccess( result )

#ifdef NDEBUG
#define DEBUG_NAME( object, name )
#else
#define DEBUG_NAME( object, name ) \
    AppState::instance().getDebug().setName( object, name );
#endif

static std::vector<char> readFile( const std::string& filename )
{
    std::ifstream file( filename, std::ios::ate | std::ios::binary );

    assert( file.is_open() );

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer( fileSize );
    file.seekg( 0 );
    file.read( buffer.data(), fileSize );
    file.close();

    return buffer;
}

static void checkSuccess( VkResult result )
{
    assert( result == VK_SUCCESS );
}

static void checkSuccess( vk::Result result )
{
    assert( (VkResult)result == VK_SUCCESS );
}
