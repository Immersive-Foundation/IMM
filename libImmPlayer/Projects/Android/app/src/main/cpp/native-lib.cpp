#include <android_native_app_glue.h>

/**
 * Including this so we can load so dependency from native activity.
 *
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main( struct android_app * app )
{
}

