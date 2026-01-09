#pragma once

#include "../DriverLog.h"

// #include "../../../../ThirdParty/minhook/include/MinHook.h"
#include "./vtablehook.h"
#include <map>
#include <string>


class IHook
{
public:
	const std::string name;

	IHook(const std::string &name) : name(name) { }
	virtual ~IHook() { }

	virtual void Destroy() = 0;

	static bool Exists(const std::string &name);
	static void Register(IHook *hook);
	static void Unregister(IHook *hook);
	static void DestroyAll();

private:
	static std::map<std::string, IHook *> hooks;
};

template<class FuncType> class Hook : public IHook
{
public:
	FuncType originalFunc = nullptr;
	int originalVTableOffset = -1;
	void* originalObject = nullptr;
	Hook(const std::string &name) : IHook(name) { }

	bool CreateHookInObjectVTable(void *object, int vtableOffset, void *detourFunction)
	{
		/*
		// For virtual objects, VC++ adds a pointer to the vtable as the first member.
		// To access the vtable, we simply dereference the object.
		void **vtable = *((void ***)object);

		// The vtable itself is an array of pointers to member functions,
		// in the order they were declared in.
		targetFunc = vtable[vtableOffset];

		auto err = MH_CreateHook(targetFunc, detourFunction, (LPVOID *)&originalFunc);
		if (err != MH_OK)
		{
			DriverLog("Failed to create hook for %s, error: %s", name.c_str(), MH_StatusToString(err));
			return false;
		}

		err = MH_EnableHook(targetFunc);
		if (err != MH_OK)
		{
			DriverLog("Failed to enable hook for %s, error: %s", name.c_str(), MH_StatusToString(err));
			MH_RemoveHook(targetFunc);
			return false;
		}
		*/
		if(originalFunc){
			DriverLog("Hook for %s already exists", name.c_str());
			return false;
		}
		
		originalFunc = (FuncType)vtablehook_hook(object, detourFunction, vtableOffset);
		originalObject = object;
		originalVTableOffset = vtableOffset;
		
		DriverLog("Enabled hook for %s", name.c_str());
		enabled = true;
		return true;
	}

	void Destroy()
	{
		if (enabled)
		{
			// MH_RemoveHook(targetFunc);
			if(originalFunc){
				vtablehook_hook(originalObject, originalFunc, originalVTableOffset);
				originalFunc = nullptr;
			}
			enabled = false;
		}
	}

private:
	bool enabled = false;
	void* targetFunc = nullptr;
};