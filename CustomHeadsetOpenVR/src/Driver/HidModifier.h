#pragma once
#include <map>
#include <string>


class HidModifier {
public:
	class HeadsetStarter {
	public:
		// this function will be called every frame if the fist call results in true indicating that the headset connected
		virtual bool StartIfNeeded() = 0;
	};

	// hook the hid functions in the lighthouse driver
	void InjectHooks();
	// called every frame to allow starters to run in the main thread if needed
	void RunFrame();
	bool hasHooked = false;
	
	// map of HeadsetStarters to call every frame, they are stored by an identifier string to avoid duplicate registrations
	std::map<std::string, HeadsetStarter*> starters;
	
	// opaque hidapi structure
	typedef struct hid_device_ hid_device;
	// this is a bit sussy as this technically could change in different hidapi versions
	struct hid_device_info {
		/** Platform-specific device path */
		char *path;
		/** Device Vendor ID */
		unsigned short vendor_id;
		/** Device Product ID */
		unsigned short product_id;
		/** Serial Number */
		wchar_t *serial_number;
		/** Device Release Number in binary-coded decimal,
			also known as Device Version Number */
		unsigned short release_number;
		/** Manufacturer String */
		wchar_t *manufacturer_string;
		/** Product string */
		wchar_t *product_string;
		/** Usage Page for this Device/Interface
			(Windows/Mac/hidraw only) */
		unsigned short usage_page;
		/** Usage for this Device/Interface
			(Windows/Mac/hidraw only) */
		unsigned short usage;
		/** The USB interface which this logical device
			represents.

			Valid only if the device is a USB HID device.
			Set to -1 in all other cases.
		*/
		int interface_number;

		/** Pointer to the next device */
		struct hid_device_info *next;

		/** Underlying bus type
			Since version 0.13.0, @ref HID_API_VERSION >= HID_API_MAKE_VERSION(0, 13, 0)
		*/
		int bus_type;
	};
	// info about device
	class HidDeviceInfo {
	public:
		hid_device* device;
		std::string deviceName;
		// std::string lighthouseConfig;
		unsigned char* newLighthouseConfig;
		int newLighthouseConfigLength;
		int newLighthouseConfigOffset;
		void Delete(){
			if(newLighthouseConfig) {
				delete[] newLighthouseConfig;
			}
		}
	};
	// map of devices that have been opened
	std::map<hid_device*, HidDeviceInfo> deviceMap;
	
	// add a device to the map
	void AddDevice(hid_device* device);
	
	// remove a device from the map
	void RemoveDevice(hid_device* device);
	
	// read the lighthouse config from a device and modify it for the lighthouse driver to read
	std::string ReadLighthouseConfig(HidDeviceInfo &info);
	
	// hooks of hidapi lib
	static int HidWriteHook(hid_device* device, const unsigned char* data, size_t length);
	static int HidReadTimeoutHook(hid_device* device, unsigned char* data, size_t length, int milliseconds);
	static int HidGetFeatureReportHook(hid_device* device, unsigned char* data, size_t length);
	static void HidCloseHook(hid_device* device);
	
	// original hidapi functions
	static int (*origHidWrite)(hid_device* device, const unsigned char* data, size_t length);
	static int (*origHidReadTimeout)(hid_device* device, unsigned char* data, size_t length, int milliseconds);
	static int (*origHidGetFeatureReport)(hid_device* device, unsigned char* data, size_t length);
	static void (*origHidClose)(hid_device* device);
	// extra non-hooked functions
	// why compile hidapi when there is a perfectly good one exposed on the lighthouse driver?
	static hid_device* (*origHidOpen)(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number);
	static hid_device* (*origHidOpenPath)(const char* path);
	static const wchar_t* (*origHidError)(hid_device* device);
	static hid_device_info * (*origHidEnumerate)(unsigned short vendor_id, unsigned short product_id);
	static void (*origHidFreeEnumeration)(hid_device_info* devs);
	
	
	
	// the device we are currently
};

extern HidModifier hidModifier;

#if __has_include("HidModifierPrivate.cpp")
#define HAS_PRIVATE 1
#endif