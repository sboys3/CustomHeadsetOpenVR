#include "DisplayHelper.h"
#include "Windows.h"
#include <vector>

class MonitorEnumerator{
public:
	class MonitorInfo{
	public:
		int x, y, width, height;
		int edidVendorId, edidProductId;
	};
	
	std::vector<MonitorInfo> monitors;
private:
	static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData){
		MonitorEnumerator* enumerator = (MonitorEnumerator*)dwData;
		MONITORINFOEX mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hMonitor, &mi);
		enumerator->monitors.push_back({mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top});
		return TRUE;
	}
public:
	void EnumerateMonitors(){
		monitors.clear();
		EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)this);
	}
};
	


void FindDisplayPosition(int width, int height, int edidVendorId, int edidProductId, int* x, int* y){
	MonitorEnumerator enumerator;
	enumerator.EnumerateMonitors();
	for(auto& monitor : enumerator.monitors){
		if(monitor.width == width && monitor.height == height){
			*x = monitor.x;
			*y = monitor.y;
			return;
		}
	}
}
	
	