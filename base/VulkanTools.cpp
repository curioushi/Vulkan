/*
* Assorted commonly used Vulkan helper functions
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanTools.h"

const std::string getAssetPath()
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	return "";
#elif defined(VK_EXAMPLE_DATA_DIR)
	return VK_EXAMPLE_DATA_DIR;
#else
	return "./../data/";
#endif
}

namespace vks
{
	namespace tools
	{
		bool errorModeSilent = false;

		std::string errorString(VkResult errorCode)
		{
			switch (errorCode)
			{
#define STR(r) case VK_ ##r: return #r
				STR(NOT_READY);
				STR(TIMEOUT);
				STR(EVENT_SET);
				STR(EVENT_RESET);
				STR(INCOMPLETE);
				STR(ERROR_OUT_OF_HOST_MEMORY);
				STR(ERROR_OUT_OF_DEVICE_MEMORY);
				STR(ERROR_INITIALIZATION_FAILED);
				STR(ERROR_DEVICE_LOST);
				STR(ERROR_MEMORY_MAP_FAILED);
				STR(ERROR_LAYER_NOT_PRESENT);
				STR(ERROR_EXTENSION_NOT_PRESENT);
				STR(ERROR_FEATURE_NOT_PRESENT);
				STR(ERROR_INCOMPATIBLE_DRIVER);
				STR(ERROR_TOO_MANY_OBJECTS);
				STR(ERROR_FORMAT_NOT_SUPPORTED);
				STR(ERROR_SURFACE_LOST_KHR);
				STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
				STR(SUBOPTIMAL_KHR);
				STR(ERROR_OUT_OF_DATE_KHR);
				STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
				STR(ERROR_VALIDATION_FAILED_EXT);
				STR(ERROR_INVALID_SHADER_NV);
#undef STR
			default:
				return "UNKNOWN_ERROR";
			}
		}

		std::string physicalDeviceTypeString(VkPhysicalDeviceType type)
		{
			switch (type)
			{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
				STR(OTHER);
				STR(INTEGRATED_GPU);
				STR(DISCRETE_GPU);
				STR(VIRTUAL_GPU);
				STR(CPU);
#undef STR
			default: return "UNKNOWN_DEVICE_TYPE";
			}
		}

        std::string formatString(VkFormat format) {
          switch (format)
          {
            case VK_FORMAT_UNDEFINED:return "VK_FORMAT_UNDEFINED";
            case VK_FORMAT_R4G4_UNORM_PACK8:return "VK_FORMAT_R4G4_UNORM_PACK8";
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
            case VK_FORMAT_R5G6B5_UNORM_PACK16:return "VK_FORMAT_R5G6B5_UNORM_PACK16";
            case VK_FORMAT_B5G6R5_UNORM_PACK16:return "VK_FORMAT_B5G6R5_UNORM_PACK16";
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
            case VK_FORMAT_R8_UNORM:return "VK_FORMAT_R8_UNORM";
            case VK_FORMAT_R8_SNORM:return "VK_FORMAT_R8_SNORM";
            case VK_FORMAT_R8_USCALED:return "VK_FORMAT_R8_USCALED";
            case VK_FORMAT_R8_SSCALED:return "VK_FORMAT_R8_SSCALED";
            case VK_FORMAT_R8_UINT:return "VK_FORMAT_R8_UINT";
            case VK_FORMAT_R8_SINT:return "VK_FORMAT_R8_SINT";
            case VK_FORMAT_R8_SRGB:return "VK_FORMAT_R8_SRGB";
            case VK_FORMAT_R8G8_UNORM:return "VK_FORMAT_R8G8_UNORM";
            case VK_FORMAT_R8G8_SNORM:return "VK_FORMAT_R8G8_SNORM";
            case VK_FORMAT_R8G8_USCALED:return "VK_FORMAT_R8G8_USCALED";
            case VK_FORMAT_R8G8_SSCALED:return "VK_FORMAT_R8G8_SSCALED";
            case VK_FORMAT_R8G8_UINT:return "VK_FORMAT_R8G8_UINT";
            case VK_FORMAT_R8G8_SINT:return "VK_FORMAT_R8G8_SINT";
            case VK_FORMAT_R8G8_SRGB:return "VK_FORMAT_R8G8_SRGB";
            case VK_FORMAT_R8G8B8_UNORM:return "VK_FORMAT_R8G8B8_UNORM";
            case VK_FORMAT_R8G8B8_SNORM:return "VK_FORMAT_R8G8B8_SNORM";
            case VK_FORMAT_R8G8B8_USCALED:return "VK_FORMAT_R8G8B8_USCALED";
            case VK_FORMAT_R8G8B8_SSCALED:return "VK_FORMAT_R8G8B8_SSCALED";
            case VK_FORMAT_R8G8B8_UINT:return "VK_FORMAT_R8G8B8_UINT";
            case VK_FORMAT_R8G8B8_SINT:return "VK_FORMAT_R8G8B8_SINT";
            case VK_FORMAT_R8G8B8_SRGB:return "VK_FORMAT_R8G8B8_SRGB";
            case VK_FORMAT_B8G8R8_UNORM:return "VK_FORMAT_B8G8R8_UNORM";
            case VK_FORMAT_B8G8R8_SNORM:return "VK_FORMAT_B8G8R8_SNORM";
            case VK_FORMAT_B8G8R8_USCALED:return "VK_FORMAT_B8G8R8_USCALED";
            case VK_FORMAT_B8G8R8_SSCALED:return "VK_FORMAT_B8G8R8_SSCALED";
            case VK_FORMAT_B8G8R8_UINT:return "VK_FORMAT_B8G8R8_UINT";
            case VK_FORMAT_B8G8R8_SINT:return "VK_FORMAT_B8G8R8_SINT";
            case VK_FORMAT_B8G8R8_SRGB:return "VK_FORMAT_B8G8R8_SRGB";
            case VK_FORMAT_R8G8B8A8_UNORM:return "VK_FORMAT_R8G8B8A8_UNORM";
            case VK_FORMAT_R8G8B8A8_SNORM:return "VK_FORMAT_R8G8B8A8_SNORM";
            case VK_FORMAT_R8G8B8A8_USCALED:return "VK_FORMAT_R8G8B8A8_USCALED";
            case VK_FORMAT_R8G8B8A8_SSCALED:return "VK_FORMAT_R8G8B8A8_SSCALED";
            case VK_FORMAT_R8G8B8A8_UINT:return "VK_FORMAT_R8G8B8A8_UINT";
            case VK_FORMAT_R8G8B8A8_SINT:return "VK_FORMAT_R8G8B8A8_SINT";
            case VK_FORMAT_R8G8B8A8_SRGB:return "VK_FORMAT_R8G8B8A8_SRGB";
            case VK_FORMAT_B8G8R8A8_UNORM:return "VK_FORMAT_B8G8R8A8_UNORM";
            case VK_FORMAT_B8G8R8A8_SNORM:return "VK_FORMAT_B8G8R8A8_SNORM";
            case VK_FORMAT_B8G8R8A8_USCALED:return "VK_FORMAT_B8G8R8A8_USCALED";
            case VK_FORMAT_B8G8R8A8_SSCALED:return "VK_FORMAT_B8G8R8A8_SSCALED";
            case VK_FORMAT_B8G8R8A8_UINT:return "VK_FORMAT_B8G8R8A8_UINT";
            case VK_FORMAT_B8G8R8A8_SINT:return "VK_FORMAT_B8G8R8A8_SINT";
            case VK_FORMAT_B8G8R8A8_SRGB:return "VK_FORMAT_B8G8R8A8_SRGB";
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32:return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32:return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_UINT_PACK32:return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SINT_PACK32:return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_SNORM_PACK32:return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_USCALED_PACK32:return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_UINT_PACK32:return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
            case VK_FORMAT_A2R10G10B10_SINT_PACK32:return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_SNORM_PACK32:return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_USCALED_PACK32:return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_UINT_PACK32:return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
            case VK_FORMAT_A2B10G10R10_SINT_PACK32:return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
            case VK_FORMAT_R16_UNORM:return "VK_FORMAT_R16_UNORM";
            case VK_FORMAT_R16_SNORM:return "VK_FORMAT_R16_SNORM";
            case VK_FORMAT_R16_USCALED:return "VK_FORMAT_R16_USCALED";
            case VK_FORMAT_R16_SSCALED:return "VK_FORMAT_R16_SSCALED";
            case VK_FORMAT_R16_UINT:return "VK_FORMAT_R16_UINT";
            case VK_FORMAT_R16_SINT:return "VK_FORMAT_R16_SINT";
            case VK_FORMAT_R16_SFLOAT:return "VK_FORMAT_R16_SFLOAT";
            case VK_FORMAT_R16G16_UNORM:return "VK_FORMAT_R16G16_UNORM";
            case VK_FORMAT_R16G16_SNORM:return "VK_FORMAT_R16G16_SNORM";
            case VK_FORMAT_R16G16_USCALED:return "VK_FORMAT_R16G16_USCALED";
            case VK_FORMAT_R16G16_SSCALED:return "VK_FORMAT_R16G16_SSCALED";
            case VK_FORMAT_R16G16_UINT:return "VK_FORMAT_R16G16_UINT";
            case VK_FORMAT_R16G16_SINT:return "VK_FORMAT_R16G16_SINT";
            case VK_FORMAT_R16G16_SFLOAT:return "VK_FORMAT_R16G16_SFLOAT";
            case VK_FORMAT_R16G16B16_UNORM:return "VK_FORMAT_R16G16B16_UNORM";
            case VK_FORMAT_R16G16B16_SNORM:return "VK_FORMAT_R16G16B16_SNORM";
            case VK_FORMAT_R16G16B16_USCALED:return "VK_FORMAT_R16G16B16_USCALED";
            case VK_FORMAT_R16G16B16_SSCALED:return "VK_FORMAT_R16G16B16_SSCALED";
            case VK_FORMAT_R16G16B16_UINT:return "VK_FORMAT_R16G16B16_UINT";
            case VK_FORMAT_R16G16B16_SINT:return "VK_FORMAT_R16G16B16_SINT";
            case VK_FORMAT_R16G16B16_SFLOAT:return "VK_FORMAT_R16G16B16_SFLOAT";
            case VK_FORMAT_R16G16B16A16_UNORM:return "VK_FORMAT_R16G16B16A16_UNORM";
            case VK_FORMAT_R16G16B16A16_SNORM:return "VK_FORMAT_R16G16B16A16_SNORM";
            case VK_FORMAT_R16G16B16A16_USCALED:return "VK_FORMAT_R16G16B16A16_USCALED";
            case VK_FORMAT_R16G16B16A16_SSCALED:return "VK_FORMAT_R16G16B16A16_SSCALED";
            case VK_FORMAT_R16G16B16A16_UINT:return "VK_FORMAT_R16G16B16A16_UINT";
            case VK_FORMAT_R16G16B16A16_SINT:return "VK_FORMAT_R16G16B16A16_SINT";
            case VK_FORMAT_R16G16B16A16_SFLOAT:return "VK_FORMAT_R16G16B16A16_SFLOAT";
            case VK_FORMAT_R32_UINT:return "VK_FORMAT_R32_UINT";
            case VK_FORMAT_R32_SINT:return "VK_FORMAT_R32_SINT";
            case VK_FORMAT_R32_SFLOAT:return "VK_FORMAT_R32_SFLOAT";
            case VK_FORMAT_R32G32_UINT:return "VK_FORMAT_R32G32_UINT";
            case VK_FORMAT_R32G32_SINT:return "VK_FORMAT_R32G32_SINT";
            case VK_FORMAT_R32G32_SFLOAT:return "VK_FORMAT_R32G32_SFLOAT";
            case VK_FORMAT_R32G32B32_UINT:return "VK_FORMAT_R32G32B32_UINT";
            case VK_FORMAT_R32G32B32_SINT:return "VK_FORMAT_R32G32B32_SINT";
            case VK_FORMAT_R32G32B32_SFLOAT:return "VK_FORMAT_R32G32B32_SFLOAT";
            case VK_FORMAT_R32G32B32A32_UINT:return "VK_FORMAT_R32G32B32A32_UINT";
            case VK_FORMAT_R32G32B32A32_SINT:return "VK_FORMAT_R32G32B32A32_SINT";
            case VK_FORMAT_R32G32B32A32_SFLOAT:return "VK_FORMAT_R32G32B32A32_SFLOAT";
            case VK_FORMAT_R64_UINT:return "VK_FORMAT_R64_UINT";
            case VK_FORMAT_R64_SINT:return "VK_FORMAT_R64_SINT";
            case VK_FORMAT_R64_SFLOAT:return "VK_FORMAT_R64_SFLOAT";
            case VK_FORMAT_R64G64_UINT:return "VK_FORMAT_R64G64_UINT";
            case VK_FORMAT_R64G64_SINT:return "VK_FORMAT_R64G64_SINT";
            case VK_FORMAT_R64G64_SFLOAT:return "VK_FORMAT_R64G64_SFLOAT";
            case VK_FORMAT_R64G64B64_UINT:return "VK_FORMAT_R64G64B64_UINT";
            case VK_FORMAT_R64G64B64_SINT:return "VK_FORMAT_R64G64B64_SINT";
            case VK_FORMAT_R64G64B64_SFLOAT:return "VK_FORMAT_R64G64B64_SFLOAT";
            case VK_FORMAT_R64G64B64A64_UINT:return "VK_FORMAT_R64G64B64A64_UINT";
            case VK_FORMAT_R64G64B64A64_SINT:return "VK_FORMAT_R64G64B64A64_SINT";
            case VK_FORMAT_R64G64B64A64_SFLOAT:return "VK_FORMAT_R64G64B64A64_SFLOAT";
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
            case VK_FORMAT_D16_UNORM:return "VK_FORMAT_D16_UNORM";
            case VK_FORMAT_X8_D24_UNORM_PACK32:return "VK_FORMAT_X8_D24_UNORM_PACK32";
            case VK_FORMAT_D32_SFLOAT:return "VK_FORMAT_D32_SFLOAT";
            case VK_FORMAT_S8_UINT:return "VK_FORMAT_S8_UINT";
            case VK_FORMAT_D16_UNORM_S8_UINT:return "VK_FORMAT_D16_UNORM_S8_UINT";
            case VK_FORMAT_D24_UNORM_S8_UINT:return "VK_FORMAT_D24_UNORM_S8_UINT";
            case VK_FORMAT_D32_SFLOAT_S8_UINT:return "VK_FORMAT_D32_SFLOAT_S8_UINT";
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
            case VK_FORMAT_BC2_UNORM_BLOCK:return "VK_FORMAT_BC2_UNORM_BLOCK";
            case VK_FORMAT_BC2_SRGB_BLOCK:return "VK_FORMAT_BC2_SRGB_BLOCK";
            case VK_FORMAT_BC3_UNORM_BLOCK:return "VK_FORMAT_BC3_UNORM_BLOCK";
            case VK_FORMAT_BC3_SRGB_BLOCK:return "VK_FORMAT_BC3_SRGB_BLOCK";
            case VK_FORMAT_BC4_UNORM_BLOCK:return "VK_FORMAT_BC4_UNORM_BLOCK";
            case VK_FORMAT_BC4_SNORM_BLOCK:return "VK_FORMAT_BC4_SNORM_BLOCK";
            case VK_FORMAT_BC5_UNORM_BLOCK:return "VK_FORMAT_BC5_UNORM_BLOCK";
            case VK_FORMAT_BC5_SNORM_BLOCK:return "VK_FORMAT_BC5_SNORM_BLOCK";
            case VK_FORMAT_BC6H_UFLOAT_BLOCK:return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
            case VK_FORMAT_BC6H_SFLOAT_BLOCK:return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
            case VK_FORMAT_BC7_UNORM_BLOCK:return "VK_FORMAT_BC7_UNORM_BLOCK";
            case VK_FORMAT_BC7_SRGB_BLOCK:return "VK_FORMAT_BC7_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
            case VK_FORMAT_G8B8G8R8_422_UNORM:return "VK_FORMAT_G8B8G8R8_422_UNORM";
            case VK_FORMAT_B8G8R8G8_422_UNORM:return "VK_FORMAT_B8G8R8G8_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
            case VK_FORMAT_R10X6_UNORM_PACK16:return "VK_FORMAT_R10X6_UNORM_PACK16";
            case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
            case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
            case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
            case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_R12X4_UNORM_PACK16:return "VK_FORMAT_R12X4_UNORM_PACK16";
            case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
            case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
            case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
            case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G16B16G16R16_422_UNORM:return "VK_FORMAT_G16B16G16R16_422_UNORM";
            case VK_FORMAT_B16G16R16G16_422_UNORM:return "VK_FORMAT_B16G16R16G16_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
            case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT:return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT";
            case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT:return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT:return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT:return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT";
            case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT:return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT";
            case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT:return "VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT";
            case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:return "VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT";
            case VK_FORMAT_MAX_ENUM:return "VK_FORMAT_MAX_ENUM";
          }
          return "";
        }

        std::string tilingString(VkImageTiling tiling) {
          switch (tiling) {
            case VK_IMAGE_TILING_OPTIMAL : return "VK_IMAGE_TILING_OPTIMAL";
            case VK_IMAGE_TILING_LINEAR : return "VK_IMAGE_TILING_LINEAR";
            case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT : return "VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT";
            case VK_IMAGE_TILING_MAX_ENUM : return "VK_IMAGE_TILING_MAX_ENUM";
          }
          return "";
        }

        std::string usageString(VkImageUsageFlagBits flag) {
          switch (flag) {
            case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
            case VK_IMAGE_USAGE_TRANSFER_DST_BIT:return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
            case VK_IMAGE_USAGE_SAMPLED_BIT:return "VK_IMAGE_USAGE_SAMPLED_BIT";
            case VK_IMAGE_USAGE_STORAGE_BIT:return "VK_IMAGE_USAGE_STORAGE_BIT";
            case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT:return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT:return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT:return "VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT";
            case VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR:return "VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
            case VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM:return "VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM";
          }
          return "";
        }

        VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
		{
			// Since all depth formats may be optional, we need to find a suitable depth format to use
			// Start with the highest precision packed format
			std::vector<VkFormat> depthFormats = {
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D16_UNORM
			};

			for (auto& format : depthFormats)
			{
				VkFormatProperties formatProps;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
				// Format must support depth stencil attachment for optimal tiling
				if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					*depthFormat = format;
					return true;
				}
			}

			return false;
		}

		// Returns if a given format support LINEAR filtering
		VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

			if (tiling == VK_IMAGE_TILING_OPTIMAL)
				return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

			if (tiling == VK_IMAGE_TILING_LINEAR)
				return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

			return false;
		}

		// Create an image memory barrier for changing the layout of
		// an image and put it into an active command buffer
		// See chapter 11.4 "Image Layout" for details

		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		// Fixed sub resource on first mip level and layer
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}

		void insertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void exitFatal(const std::string& message, int32_t exitCode)
		{
#if defined(_WIN32)
			if (!errorModeSilent) {
				MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
			}
#elif defined(__ANDROID__)
            LOGE("Fatal error: %s", message.c_str());
			vks::android::showAlert(message.c_str());
#endif
			std::cerr << message << "\n";
#if !defined(__ANDROID__)
			exit(exitCode);
#endif
		}

		void exitFatal(const std::string& message, VkResult resultCode)
		{
			exitFatal(message, (int32_t)resultCode);
		}

#if defined(__ANDROID__)
		// Android shaders are stored as assets in the apk
		// So they need to be loaded via the asset manager
		VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device)
		{
			// Load shader from compressed asset
			AAsset* asset = AAssetManager_open(assetManager, fileName, AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);
			assert(size > 0);

			char *shaderCode = new char[size];
			AAsset_read(asset, shaderCode, size);
			AAsset_close(asset);

			VkShaderModule shaderModule;
			VkShaderModuleCreateInfo moduleCreateInfo;
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.pNext = NULL;
			moduleCreateInfo.codeSize = size;
			moduleCreateInfo.pCode = (uint32_t*)shaderCode;
			moduleCreateInfo.flags = 0;

			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

			delete[] shaderCode;

			return shaderModule;
		}
#else
		VkShaderModule loadShader(const char *fileName, VkDevice device)
		{
			std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

			if (is.is_open())
			{
				size_t size = is.tellg();
				is.seekg(0, std::ios::beg);
				char* shaderCode = new char[size];
				is.read(shaderCode, size);
				is.close();

				assert(size > 0);

				VkShaderModule shaderModule;
				VkShaderModuleCreateInfo moduleCreateInfo{};
				moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleCreateInfo.codeSize = size;
				moduleCreateInfo.pCode = (uint32_t*)shaderCode;

				VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

				delete[] shaderCode;

				return shaderModule;
			}
			else
			{
				std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
				return VK_NULL_HANDLE;
			}
		}
#endif

		bool fileExists(const std::string &filename)
		{
			std::ifstream f(filename.c_str());
			return !f.fail();
		}

		uint32_t alignedSize(uint32_t value, uint32_t alignment)
        {
	        return (value + alignment - 1) & ~(alignment - 1);
        }

	}
}
