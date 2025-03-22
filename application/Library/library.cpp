#include library.h

// float and doubles are tricky to compare with and do calculations, the next two methods provide some workarounds to 
// avoid bugs related to lack of precision of floating point values.

/** approximatelyEqualRelative tells if two doubles are equal by comparing their absolute difference to the maximum relative "NULL" value. */
/** Do not use this method if the two floating point numbers tend towards 0. The math breaks there, better to use approximatelyEqualAbsoluteRelative  */

bool approximatelyEqualRelative(double a, double b, double relativeEpsilon) {
    return ( std::abs(a-b) <= (std::max(std::abs(a),std::abs(b)) * relativeEpsilon));
}

/** same as above. But provides a workaround if the numbers tend to zero. i.e: we then compare them to the absoluteEpsilon  */
bool approximatelyEqualAbsoluteRelative(double a, double b, double absoluteEpsilon, double relativeEpsilon) {
    if (std::abs(a-b) <= absoluteEpsilon) {
        return true;
    }
    return approximatelyEqualRelative(a,b,relativeEpsilon);
}