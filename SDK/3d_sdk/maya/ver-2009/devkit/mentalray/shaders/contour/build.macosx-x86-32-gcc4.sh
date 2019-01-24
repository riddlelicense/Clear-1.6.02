gcc -c -O3 -mtune=pentiumpro -fexpensive-optimizations -fforce-mem -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -fPIC -dynamic -fno-common -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DMACOSX -D_REENTRANT -DMI_LITTLE_ENDIAN -DX86 -DHYPERTHREAD -D__NO_CTYPE -DHYPERTHREAD -D_STLP_LITTLE_ENDIAN=1 -D_STLP_SHORT_STRING_SZ=32 -D_STLP_NATIVE_INCLUDE_PATH=/usr/include/c++/4.0.0 -I/h/misrc/ray/3.7.1/src/base/system/stlport -I/h/misrc/ray/3.7.1/src/shaders/.. -I../.. -I.  ./contourshade.c
gcc -c -O3 -mtune=pentiumpro -fexpensive-optimizations -fforce-mem -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -fPIC -dynamic -fno-common -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DMACOSX -D_REENTRANT -DMI_LITTLE_ENDIAN -DX86 -DHYPERTHREAD -D__NO_CTYPE -DHYPERTHREAD -D_STLP_LITTLE_ENDIAN=1 -D_STLP_SHORT_STRING_SZ=32 -D_STLP_NATIVE_INCLUDE_PATH=/usr/include/c++/4.0.0 -I/h/misrc/ray/3.7.1/src/base/system/stlport -I/h/misrc/ray/3.7.1/src/shaders/.. -I../.. -I.  ./outimgshade.c
gcc -c -O3 -mtune=pentiumpro -fexpensive-optimizations -fforce-mem -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -fPIC -dynamic -fno-common -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DMACOSX -D_REENTRANT -DMI_LITTLE_ENDIAN -DX86 -DHYPERTHREAD -D__NO_CTYPE -DHYPERTHREAD -D_STLP_LITTLE_ENDIAN=1 -D_STLP_SHORT_STRING_SZ=32 -D_STLP_NATIVE_INCLUDE_PATH=/usr/include/c++/4.0.0 -I/h/misrc/ray/3.7.1/src/base/system/stlport -I/h/misrc/ray/3.7.1/src/shaders/.. -I../.. -I.  ./outpsshade.c
g++ -flat_namespace -undefined suppress -dynamiclib -o contour.so build-contourshade.o.sh build-outimgshade.o.sh build-outpsshade.o.sh
