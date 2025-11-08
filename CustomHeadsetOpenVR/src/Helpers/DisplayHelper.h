#pragma once

// find where a display that matches the width and height is on the desktop and set x and y to that location
// currently this does not actually use the edidVendorId and edidProductId
void FindDisplayPosition(int width, int height, int edidVendorId, int edidProductId, int* x, int* y);