#pragma once

struct BufferInfo
{
	DeviceWrapper& deviceWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	vk::ShaderStageFlags stageFlags; 
	uint32_t binding;
	size_t nBuffers = 1;
};

template<class T>
class UniformBufferBase
{
protected:
	UniformBufferBase() = default;
	~UniformBufferBase() = default;

public:
	void init(BufferInfo& info)
	{
		create_buffer(info);
		create_descriptor_sets(info);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		destroy_buffer(allocator);
	}
	static vk::DescriptorSetLayout get_temp_desc_set_layout(DeviceWrapper& deviceWrapper, uint32_t binding, vk::ShaderStageFlags stageFlags)
	{
		vk::DescriptorSetLayoutBinding layoutBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(binding)
			.setStageFlags(stageFlags)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setPImmutableSamplers(nullptr);

		// create descriptor set layout from all the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setBindings(layoutBinding);
		return deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}

protected:
	virtual void destroy_buffer(vma::Allocator& allocator) = 0;
	virtual void create_buffer(BufferInfo& info) = 0;
	virtual void create_descriptor_sets(BufferInfo& info) = 0;

public:
	T data;
};

template<class T>
class UniformBufferStatic : public UniformBufferBase<T>
{
public:
	UniformBufferStatic() = default;
	~UniformBufferStatic() = default;

public:
	void write_buffer_slow()
	{

	}
private:
	std::pair<vk::Buffer, vma::Allocation> buffer;
	vk::DescriptorSet descSet;
};

template<class T>
class UniformBufferDynamic : public UniformBufferBase<T>
{
public:
	UniformBufferDynamic() = default;
	~UniformBufferDynamic() = default;

public:
	inline vk::DescriptorSet& get_desc_set()
	{
		return bufferFrames.get_current().descSet;
	}
	inline void write_buffer()
	{
		// already mapped, so just copy over
		// additionally, advance frame by one, so the next free buffer frame gets written to
		memcpy(bufferFrames.get_next().allocInfo.pMappedData, &(UniformBufferBase<T>::data), sizeof(T));
	}

private:
	void destroy_buffer(vma::Allocator& allocator) override
	{
		bufferFrames.destroy(allocator);
	}
	void create_buffer(BufferInfo& info) override
	{
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(sizeof(T))
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

		bufferFrames.set_size((uint32_t)info.nBuffers);
		for (size_t i = 0; i < info.nBuffers; i++) {
			auto& frame = bufferFrames[i];
			vk::Result result = info.allocator.createBuffer(&bufferInfo, &allocCreateInfo, &frame.buffer, &frame.alloc, &frame.allocInfo);
			// TODO: check res
		}
	}
	void create_descriptor_sets(BufferInfo& info) override
	{
		// temp layout
		vk::DescriptorSetLayout descSetLayout = UniformBufferBase<T>::get_temp_desc_set_layout(
			info.deviceWrapper, info.binding, info.stageFlags);

		// duplicate layout to have one for each frame in flight
		std::vector<vk::DescriptorSetLayout> layouts(info.nBuffers, descSetLayout);

		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(info.descPool)
			.setSetLayouts(layouts);
		auto descSetArr = info.deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo);
		for (size_t i = 0; i < info.nBuffers; i++) {
			bufferFrames[i].descSet = descSetArr[i];
		}

		std::vector<vk::DescriptorBufferInfo> descBufferInfos(info.nBuffers);
		std::vector<vk::WriteDescriptorSet> descBufferWrites(info.nBuffers);
		for (size_t i = 0; i < info.nBuffers; i++) {
			descBufferInfos[i] = vk::DescriptorBufferInfo()
				.setBuffer(bufferFrames[i].buffer)
				.setOffset(0)
				.setRange(sizeof(T));

			descBufferWrites[i] = vk::WriteDescriptorSet()
				.setDstSet(bufferFrames[i].descSet)
				.setDstBinding(info.binding)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				//
				.setPBufferInfo(&descBufferInfos[i])
				.setPImageInfo(nullptr)
				.setPTexelBufferView(nullptr);

		}
		info.deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
		info.deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}

	struct BufferFrame
	{
		void destroy(vma::Allocator& allocator)
		{
			allocator.destroyBuffer(buffer, alloc);
		}

		vk::Buffer buffer;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
		vk::DescriptorSet descSet;
	};
	RingBuffer<BufferFrame> bufferFrames;
};