#!/bin/sh

INCLUDE_DIR="../../../../../../../shared/"

if [ $1 = "Windows" ]
    then
        GENERATOR="-G MSYS Makefiles"
fi	
    
BASEFLAGS='
-DENABLE_LTO=OFF
-DSFIZZ_JACK=OFF
-DSFIZZ_RENDER=OFF
-DSFIZZ_SHARED=ON
-DPLUGIN_AU=OFF
-DPLUGIN_LV2=OFF
-DPLUGIN_LV2_UI=OFF
-DPLUGIN_PUREDATA=ON
-DPLUGIN_VST2=OFF
-DPLUGIN_VST3=OFF
-DSFIZZ_BENCHMARKS=OFF
-DSFIZZ_DEMOS=OFF
-DSFIZZ_DEVTOOLS=OFF
-DSFIZZ_TESTS=OFF
-DSFIZZ_USE_SNDFILE=OFF
-DSFIZZ_SNDFILE_STATIC=OFF
-DSFIZZ_USE_SYSTEM_ABSEIL=OFF
-DSFIZZ_USE_SYSTEM_CATCH=OFF
-DSFIZZ_USE_SYSTEM_CXXOPTS=OFF
-DSFIZZ_USE_SYSTEM_GHC_FS=OFF
-DSFIZZ_USE_SYSTEM_LV2=OFF
-DSFIZZ_USE_SYSTEM_SIMDE=OFF
-DSFIZZ_USE_SYSTEM_KISS_FFT=OFF
-DSFIZZ_USE_SYSTEM_PUGIXML=OFF
-DSFIZZ_USE_SYSTEM_VST3SDK=OFF
-DSFIZZ_PROFILE_BUILD=OFF
-DSFIZZ_RELEASE_ASSERTS=OFF
' 
cp ./sfz~.c ./sfizz/plugins/puredata/sfizz_puredata.c

# Modify the #include statements in the copied source file
# Replace "../../../shared/elsefile.h" with the updated path relative to the include directory
# use -i.bak for macOS compatibility
sed -i.bak "s|#include \"../../../shared/elsefile.h\"|#include \"$INCLUDE_DIR/elsefile.h\"|g" ./sfizz/plugins/puredata/sfizz_puredata.c
rm ./sfizz/plugins/puredata/sfizz_puredata.c.bak

mkdir -p sfizz/build
cd sfizz/build
cmake "$GENERATOR" .. $BASEFLAGS
make
