#pragma once

#include <cassert>
#include <fstream>

#define CHECK_SUCCESS( result ) checkSuccess( result )

#ifdef NDEBUG
#define DEBUG_NAME( object, name )
#else
#define DEBUG_NAME( object, name ) \
    AppState::instance().getDebug().setName( object, name );
#endif

namespace BR
{

inline std::vector<char> readFile( const std::string& filename )
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

inline void checkSuccess( VkResult result )
{
    assert( result == VK_SUCCESS );
}

inline void checkSuccess( vk::Result result )
{
    assert( (VkResult)result == VK_SUCCESS );
}

inline void imageBarrier( vk::CommandBuffer commandBuffer, vk::Image image,
                          vk::ImageSubresourceRange& subresourceRange,
                          vk::AccessFlags srcAccessMask,
                          vk::AccessFlags dstAccessMask,
                          vk::ImageLayout oldLayout, vk::ImageLayout newLayout )
{
    vk::ImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(
        commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
        reinterpret_cast<VkImageMemoryBarrier*>( &imageMemoryBarrier ) );
}

}  // namespace BR