cmake -G "Unix Makefiles" -DPLATFORM_VITA=ON -DOPTION_EMU=pfbneo -DCMAKE_BUILD_TYPE=Release ..
make pfbneo.deps
make pfbneo.vpk -j8

libcross2d/CMakeList.txt: 
find_package(box2d REQUIRED PATHS /usr/local/vitasdk/arm-vita-eabi)
