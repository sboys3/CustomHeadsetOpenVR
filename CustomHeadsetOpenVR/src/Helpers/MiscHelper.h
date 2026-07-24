#pragma once

#include <string>
#include <map>
#include <mutex>
#include <thread>

// Compares two version strings and returns true if 'latest' is newer than 'current'.
// Handles optional leading 'v'/'V' prefix, numeric and alphanumeric parts separated by '.' or '-'.
bool IsNewVersion(const std::string& current, const std::string& latest);

// Looks up the DisplayVersion from the Windows registry Uninstall keys for a program matching displayName.
// Searches both 32-bit and 64-bit registry views. Returns empty string if not found.
std::string GetInstalledProgramVersion(const std::string& displayName);


// A lock guard that uses the shared lock on a std::shared_mutex
// This also avoids locking more than once per thread, which can result in deadlocks once non-shared locks are called. This can happen because a thread can be shared locked, and then when another thread requests the exclusive lock, the second shared lock within a thread will be blocked. Therefore, the first lock never gets freed. By preventing more than one lock per thread, this problem is mitigated.
template<typename MutexType>
class shared_lock_guard { // match c++ std style for definition, maybe some day this can be removed in favor of one in std::
	MutexType& _mutex;

private:
	// Map of mutex pointer -> (thread id -> lock count), so each mutex tracks its own per-thread lock counts
	static std::map<void*, std::map<std::thread::id, int>> currentlyLockedThreads;
	static std::mutex lockMapMutex;
	// for easy debugging
	std::map<std::thread::id, int>* currentlyLockedThreadsPtr = nullptr;

public:
	explicit inline shared_lock_guard(MutexType& mutex) : _mutex(mutex) {
		auto threadId = std::this_thread::get_id();
		void* mutexPtr = static_cast<void*>(static_cast<MutexType*>(&_mutex));
		lockMapMutex.lock();
		std::map<std::thread::id, int>& threadMap = currentlyLockedThreads[mutexPtr];
		int currentCount = threadMap[threadId];
		currentlyLockedThreadsPtr = &threadMap;
		lockMapMutex.unlock();
		if(currentCount == 0){
			_mutex.lock_shared();
		}
		lockMapMutex.lock();
		threadMap[threadId] = currentCount + 1;
		lockMapMutex.unlock();
	}
	inline ~shared_lock_guard() {
		auto threadId = std::this_thread::get_id();
		void* mutexPtr = static_cast<void*>(static_cast<MutexType*>(&_mutex));
		lockMapMutex.lock();
		std::map<std::thread::id, int>& threadMap = currentlyLockedThreads[mutexPtr];
		int currentCount = threadMap[threadId];
		lockMapMutex.unlock();
		if(currentCount == 1){
			_mutex.unlock_shared();
		}
		lockMapMutex.lock();
		threadMap[threadId] = currentCount - 1;
		lockMapMutex.unlock();
	}
};

template<typename T>
std::map<void*, std::map<std::thread::id, int>> shared_lock_guard<T>::currentlyLockedThreads;

template<typename T>
std::mutex shared_lock_guard<T>::lockMapMutex;