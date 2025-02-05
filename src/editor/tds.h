#ifndef __TDS__H_
#define __TDS__H_

#include <stdint.h>

static constexpr const uint32_t TDS_FOURCC = 0x32534446;    // 2SDF
static constexpr const uint16_t TDS_MAJOR = 2;
static constexpr const uint16_t TDS_MINOR = 4;
static constexpr const long TDS_FILE_MAXSIZE = (1<<16);

static inline bool tds_normalizion_support(uint16_t major, uint16_t minor) {return (major>=2) && (minor>=2);}

/*

File version history

2.000 : brute-force serializer of the whole primitive structure
2.001 : dependeding of the type serialize only properties needed
2.002 : normalized value (position, width, ..) between in 0 and 1, to support any final resolution
2.003 : serialize only the points needed for the shape
2.004 : enum for fillmode, introducing outline (filled + contour)


*/


#endif