build_variant=$1
symbol_path=app/build/intermediates/cmake/$build_variant/debug/obj/armeabi-v7a/
# symbol_path='app/.externalNativeBuild/cmake/debug/armeabi-v7a/'
./quilllogcat.sh | ndk-stack -sym $symbol_path
