package org.linuxfoundation.imm.player

import android.content.Context
import android.os.AsyncTask
import android.util.Log
import android.util.TimingLogger
import java.io.File
import java.io.RandomAccessFile
import java.lang.ref.WeakReference

object Utils {
  private val TAG = Utils::class.java.simpleName
  const val assetVersionFile = "asset-version.txt"

  // Need to modify the value in |assetVersionFile| every time this is incremented.
  // If this doesn't match the value in |assetVersionFile| then we need to delete and recopy assets.
  // Note the below line is referred in drop_fetch_quill_theater.py in case of change
  private const val currentAssetVersion = 2

  // Path to Quill we show when there's an error loading a Quill path.
  var quillErrorPath = "quill-error.imm"
  var quillDemoPath = "retail-demo.imm"
  /*var controllerAssets = arrayOf(
      "controllers/controller_new.obj",
      "controllers/pointer.obj"
  )*/
  var assetsDirectory: String? = null
  var cacheDirectory: String? = null

  // Called once in each onCreate before extraction tasks.
  fun shouldReloadAssets(): Boolean {
    val version = readStringFromFile(assetsDirectory + assetVersionFile)
    if (version == null) {
      Log.d(TAG, "Could not find " + assetVersionFile + " reloading assets from APK...")
      return true
    }

    // Looks at the existing |assetsVersionFile| in the data directory to see if it matches the
    // compile time |currentAssetVersion|
    val reload = version.isEmpty() || version.toInt() != currentAssetVersion
    if (reload) {
      Log.d(
          TAG,
          "Found " +
              assetVersionFile +
              " version: " +
              version +
              " expected: " +
              currentAssetVersion +
              ". Reloading assets from APK...")
    }
    return reload
  }

  private fun readStringFromFile(filePath: String): String? {
    var file = File(filePath)
    if (file.exists()) {
      return file.readText(Charsets.UTF_8)
    } else {
      return null
    }
  }

  internal class AssetLoadTimer(
      var timingLogger: TimingLogger,
      @field:Volatile var numAssetsPending: Int
  )

  internal class ExtractAssetsTask(
      context: Context,
      path: String,
      timingLogger: AssetLoadTimer?,
      force: Boolean
  ) : AsyncTask<Void?, Void?, Void?>() {
    private val contextRef: WeakReference<Context>
    private val assetPath: String
    private val timings: AssetLoadTimer?
    private val forceExtract: Boolean

    protected override fun doInBackground(vararg params: Void?): Void? {
      extractAsset(assetPath)
      // Note that since we're executing in parallel these split times may not be accurate.
      // The total time till all assets are completed should be correct in the log.
      timings?.timingLogger?.addSplit(assetPath)
      return null
    }

    protected override fun onPostExecute(result: Void?) {
      if (timings != null && --timings.numAssetsPending == 0) {
        timings.timingLogger.dumpToLog()
      }
      contextRef.clear()
    }

    private fun extractAsset(assetName: String) {
      val assetPath =
          if (assetName.startsWith(assetsDirectory!!)) assetName else assetsDirectory + assetName
      val file = File(assetPath)
      if (!forceExtract && file.exists()) {
        Log.d(TAG, "$assetPath already exists.\n")
        return
      }
      if (file.exists()) {
        val result = file.delete()
      }
      Log.d(TAG, "$assetName extraction needed. \n")
      try {
        val context = contextRef.get()
        val result = file.parentFile.mkdirs() // create directories if needed
        val randomAccessFile = RandomAccessFile(assetPath, "rw")
        val assetManager = context!!.applicationContext.resources.assets
        val inputStream = assetManager.open(assetName)
        val buffer = ByteArray(1024)
        var count = inputStream.read(buffer, 0, 1024)
        while (count > 0) {
          randomAccessFile.write(buffer, 0, count)
          count = inputStream.read(buffer, 0, 1024)
        }
        randomAccessFile.close()
        inputStream.close()
      } catch (e: Exception) {
        Log.e(TAG, "Failure in extractAssets(): $e $assetPath")
      }
      if (file.exists()) {
        Log.d(TAG, "File extracted successfully")
      }
    }

    init {
      contextRef = WeakReference(context)
      assetPath = path
      timings = timingLogger
      forceExtract = force
    }
  }
}
