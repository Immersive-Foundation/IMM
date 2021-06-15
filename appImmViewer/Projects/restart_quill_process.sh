launch_package_name=com.facebook.arvr.quillplayer
package_name=com.facebook.arvr.demoquillplayer
launch_activity=QuillPlayerVrActivity

function akill_process {
    process=`adb shell ps | grep $1 | awk '{print $2}' | sed -e 's/  *$//'`

    if [ -n "$process" ]; then
        echo "Killing $1 process id $process"
        adb shell kill $process
    fi
} 

akill_process $package_name
adb shell am start -a android.intent.action.VIEW -n "$launch_package_name/$package_name.$launch_activity" -e QUILL_PATH "$1" -e QUILL_TRACKING_LEVEL "$2"
