package_name=com.facebook.arvr.demoquillplayer
symbol_path='app/.externalNativeBuild/cmake/debug/armeabi-v7a/'
sed -i -e 's~symbol_path=".*"~symbol_path="'"$symbol_path"'"~' /Users/plafayette/fbsource/fbandroid/scripts/ogdb

# Find the process and run ogdb script on quillviewer process
/Users/plafayette/fbsource/fbandroid/scripts/ogdb `adb shell ps -A | grep '$package_name' | awk '{print $2}' | sed -e 's/  *$//'`
