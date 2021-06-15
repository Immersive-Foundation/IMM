build_variant=$1
symbol_path=app/build/intermediates/cmake/$build_variant/release/obj/armeabi-v7a/
echo $symbol_path
# symbol_path='app/.externalNativeBuild/cmake/release/armeabi-v7a/'
./quilllogcat.sh | ndk-stack -sym $symbol_path
