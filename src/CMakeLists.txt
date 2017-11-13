add_library(SCPass MODULE
    SC.cpp
    CheckersNetwork.cpp
    Stats.cpp
)
add_library(Utils MODULE
	FunctionInfo.cpp
	FunctionMarker.cpp
	FunctionFilter.cpp
)
add_library(SCPatchPass MODULE
    PatchManifest.cpp
    SCPatch.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(SCPatchPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(SCPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(Utils PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that.
set_target_properties(SCPass PROPERTIES
    COMPILE_FLAGS "-lInputdependency -fno-rtti -g -std=c++11"
)

set_target_properties(SCPatchPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti -std=c++11 -g"
)
set_target_properties(Utils PROPERTIES
    COMPILE_FLAGS "-fno-rtti -std=c++11 -g"
)



# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
#if(APPLE)
#    set_target_properties(SCPass PROPERTIES
#        LINK_FLAGS "-undefined dynamic_lookup"
#    )
#    set_target_properties(SCPatchPass PROPERTIES
#        LINK_FLAGS "-undefined dynamic_lookup"
#    )

#endif(APPLE)
