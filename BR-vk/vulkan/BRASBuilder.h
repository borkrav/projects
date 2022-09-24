#pragma once

#include <BRBufferAllocator.h>

namespace BR
{
class ASBuilder
{
   public:
    ASBuilder();
    ~ASBuilder();

    void create( BufferAllocator& mainAlloc );
    void destroy();

    vk::AccelerationStructureKHR buildBlas( std::string name,
                                            vk::Buffer vertexBuffer,
                                            vk::Buffer indexBuffer,
                                            int maxVertex, int numIndex );

    vk::AccelerationStructureKHR buildTlas( std::string name,
                                            vk::AccelerationStructureKHR blas );

    uint64_t getAddress( vk::AccelerationStructureKHR structure );

   private:
    vk::Device m_device;
    BufferAllocator* m_alloc;
    CommandPool m_pool;

    std::vector<vk::AccelerationStructureKHR> m_structures;
    std::map<vk::AccelerationStructureKHR, uint64_t> m_addresses;
};

}  // namespace BR
