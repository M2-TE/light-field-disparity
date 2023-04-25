#pragma once

// temporary solution for logging
#define CODE_LOCATION __FUNCTION__ << " on line " << __LINE__ << " in file " << __FILE__
#define VMI_LOG(msg) std::cout << msg << std::endl
#define VMI_LOG_MUL(msg) std::cout << msg
#define VMI_WARN(msg) std::cout << "\n--> WARNING: " << msg << std::endl
#define VMI_ERR(msg) std::cout << "\n--> ERROR: " << msg << std::endl
#define VMI_SDL_ERR() VMI_LOG("Error: " << std::string(SDL_GetError()) << std::endl << CODE_LOCATION)

#ifndef NDEBUG
class Logging
{
public:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) VMI_WARN(pCallbackData->pMessage);
		else VMI_LOG(pCallbackData->pMessage);

		return VK_FALSE;
	}

	static vk::DebugUtilsMessengerCreateInfoEXT SetupDebugMessenger(std::vector<const char*>& extensions)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = vk::DebugUtilsMessengerCreateInfoEXT()
			.setMessageSeverity(
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				//vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			.setMessageType(
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
				vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
				vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
			.setPfnUserCallback(DebugCallback)
			.setPUserData(nullptr);


		return messengerInfo;
	}
};
	#define DEBUG_ONLY(x) x
#else
	#define DEBUG_ONLY(x)
#endif