
#pragma once


#include "paretofront.h"


////////////////////////////////////////////////////////////
// 2D and 3D hypervolume computation in N log(N) operations.
//
double hypervolume(Vector const& reference, ParetoFront const& front);
