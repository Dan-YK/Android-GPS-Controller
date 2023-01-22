cd build

export ABI=arm64-v8a
export MINSDKVERSION=25.1.8937393
export NDK=/home/dev/Android/Sdk/ndk/25.1.8937393

cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$ABI -DANDROID_PLATFORM=android-33

make
