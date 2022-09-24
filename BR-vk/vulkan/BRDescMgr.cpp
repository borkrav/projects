#include <BRAppState.h>
#include <BRDescMgr.h>
#include <BRUtil.h>

using namespace BR;

/*

Decriptor explanation

* Descriptors are like pointer arguments - for referencing data that shaders can read and write
* Descriptors can reference sub-ranges in a buffer

* Descriptor is the vk::Buffer we're pointing to
* Descriptor set contains a set of descriptors
* Descriptor pool is what sets are allocated from
* Descriptor set layout is the function signature


In this example
    * Layout says we're connecting a uniform buffer to slot 0 ( Can see this in the vertex shader ) - function sig
    * Create a pool that allows for 2 UBO connections and 2 sets (function sigs) total
    * Create 2 sets with the layout
    * Write the buffer into each set, thereby connecting them
    * For each frame, bind the decriptor set to the pipeline
    
* Can use same pipeline layout with different pipelines
* Therefore, the same set/buffer binding can be used in different pipelines
* Can bind multiple decriptor sets at the same time, to the same pipeline
* Can also use device addresses here

*/

DescMgr::DescMgr() : m_device( nullptr )
{
}

DescMgr::~DescMgr()
{
    assert( m_layouts.empty() && m_pools.empty() );
}

vk::DescriptorSetLayout DescMgr::createLayout(
    std::string name, const std::vector<Binding>& params )
{
    if ( !m_device )
        m_device = AppState::instance().getLogicalDevice();

    assert( !params.empty() );

    vk::DescriptorSetLayout result = nullptr;
    std::vector<vk::DescriptorSetLayoutBinding> binding;

    for ( auto paramSet : params )
    {
        vk::DescriptorSetLayoutBinding uboLayoutBinding;
        uboLayoutBinding.binding = paramSet.bind;
        uboLayoutBinding.descriptorType = paramSet.type;
        uboLayoutBinding.descriptorCount = paramSet.count;
        uboLayoutBinding.stageFlags = paramSet.stage;

        binding.push_back( std::move( uboLayoutBinding ) );
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = binding.size();
    layoutInfo.pBindings = binding.data();

    try
    {
        result = m_device.createDescriptorSetLayout( layoutInfo );
        DEBUG_NAME( result, name );
        m_layouts.push_back( result );
        return result;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create descriptor set layout" );
    }
}

vk::DescriptorPool DescMgr::createPool( std::string name, int numSets,
                                        const std::vector<PoolSize>& sizes )
{
    if ( !m_device )
        m_device = AppState::instance().getLogicalDevice();

    assert( !sizes.empty() && numSets > 0 );

    vk::DescriptorPool result = nullptr;
    std::vector<vk::DescriptorPoolSize> poolSizes;

    for ( auto size : sizes )
    {
        vk::DescriptorPoolSize poolSize;
        poolSize.type = size.type;
        poolSize.descriptorCount = size.count;

        poolSizes.push_back( std::move( poolSize ) );
    }

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = numSets;

    try
    {
        result = m_device.createDescriptorPool( poolInfo );
        DEBUG_NAME( result, name );
        m_pools.push_back( result );
        return result;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create descriptor pool" );
    }
}

vk::DescriptorSet DescMgr::createSet( std::string name,
                                      vk::DescriptorSetLayout layout,
                                      vk::DescriptorPool pool )
{
    if ( !m_device )
        m_device = AppState::instance().getLogicalDevice();

    assert( layout && pool );

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    try
    {
        auto sets = m_device.allocateDescriptorSets( allocInfo );
        auto result = sets[0];
        DEBUG_NAME( result, name );
        return result;
    }
    catch ( vk::SystemError err )
    {
        throw std::runtime_error( "failed to create descriptor sets" );
    }
}

void DescMgr::destroy()
{
    for ( auto pool : m_pools )
        m_device.destroyDescriptorPool( pool );

    for ( auto layout : m_layouts )
        m_device.destroyDescriptorSetLayout( layout );

    m_layouts.clear();
    m_pools.clear();
}