#ifdef NO_TO_STRING
#include <sstream>
#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()
#else
#define SSTR( x ) std::to_string( x )
#endif
