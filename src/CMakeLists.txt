add_library(SCPass MODULE
    # List your source files here.
    SC.cpp
    CheckersNetwork.cpp
)
add_library(SCPatchPass MODULE
    PatchManifest.cpp
    SCPatch.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
#target_compile_features( PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that.
set_target_properties(SCPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti -std=c++0x"
)

set_target_properties(SCPatchPass PROPERTIES
COMPILE_FLAGS "-fno-rtti -std=c++0x"
)


# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(SCPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    set_target_properties(SCPatchPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )

endif(APPLE)
