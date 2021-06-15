/**
 * **********************************************************************************
 *
 * Filename : QuillPlayerVrActivity.java Content : Created : Authors :
 *
 * Copyright : Copyright 2014 Oculus VR, LLC. All Rights reserved.
 */
package com.facebook.arvr.demoquillplayer

import android.Manifest
import android.app.NativeActivity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.*
import android.net.ConnectivityManager
import android.opengl.GLES32
import android.opengl.GLUtils
import android.os.AsyncTask
import android.os.Bundle
import android.text.Layout
import android.text.StaticLayout
import android.text.TextPaint
import android.text.TextUtils.TruncateAt
import android.util.Log
import android.util.TimingLogger
import android.webkit.URLUtil
import androidx.annotation.Keep
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.facebook.arvr.demoquillplayer.QuillPlayer.AssetLoadTimer
import com.facebook.arvr.demoquillplayer.QuillPlayer.ExtractAssetsTask
import java.io.File
import java.io.IOException
import kotlin.coroutines.resume
import kotlin.system.measureTimeMillis
import kotlinx.coroutines.*
import okhttp3.*
import okio.Okio

@Keep
class QuillPlayerVrActivity : NativeActivity(), CoroutineScope by CoroutineScope(Dispatchers.IO) {
  companion object {
    val TAG = QuillPlayerVrActivity::class.java.simpleName
    const val EXTRA_QUILL_PATH = "QUILL_PATH"
    const val EXTRA_QUILL_NAME = "QUILL_NAME"
    const val EXTRA_QUILL_ARTIST_NAME = "QUILL_ARTIST_NAME"
    const val EXTRA_QUILL_CREATION_TIME = "QUILL_CREATION_TIME"
    const val EXTRA_QUILL_TRACKING_LEVEL = "QUILL_TRACKING_LEVEL"
    const val EXTRA_QUILL_PLAYER_SPAWN_LOCATION =
        "QUILL_PLAYER_SPAWN_LOCATION" // Default, Spaces_Player1, Spaces_Player2, Spaces_Player3,
    // Spaces_Player4
    const val EXTRA_QUILL_EYE_BUFFER_SCALE = "QUILL_EYE_BUFFER_SCALE"
    const val EXTRA_QUILL_RENDERING_TECHNIQUE = "QUILL_RENDERING_TECHNIQUE"
    const val PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 0x1

    @JvmStatic external fun nativeSetAssetDirectory(assetsDir: String?)
    @JvmStatic external fun nativeSendMessage(message: String?, messageType: Int)
    @JvmStatic external fun nativeSetQuillRenderingTechnique(renderingTechnique: Int)
    @JvmStatic
    external fun nativeSetEyeBufferScale(
        scaleFactor: Float
    ) // scale factor to be applied to default eye buffers
    @JvmStatic external fun nativeSetPlayerSpawnLocation(spawnLocation: String?)
    @JvmStatic
    external fun nativeSetPlayerMetadata(
        title: String?,
        creatorName: String?,
        creationTime: Int,
        description: String?,
        artistImagePath: String?,
        previewImagePath: String?,
        hideArtistImage: Boolean,
        lowLatencyAudio: Boolean
    )
    @JvmStatic external fun nativeSetTrackingTransformLevel(trackingTransformLevel: String?)
    @JvmStatic
    external fun nativeSetDownloadingInfo(
        bytesRead: Long,
        bytesReadStr: String?,
        progress: Int,
        elapsedDownloadTime: Long
    )

    @JvmStatic
    fun logIntentExtras(intent: Intent) {
      val extras = intent.extras
      if (extras != null) {
        Log.d(TAG, "found extras!")
        for (key in extras.keySet()) {
          val value = extras[key]
          Log.d(TAG, String.format("  %s %s (%s)", key, value.toString(), value?.javaClass?.name))
        }
      }
    }

    fun humanReadableByteCount(bytes: Long): String {
      val unit = 1024
      if (bytes < unit) return "$bytes B"
      val exp = (Math.log(bytes.toDouble()) / Math.log(unit.toDouble())).toInt()
      val pre = "kMGTPE"[exp - 1].toString() + ""
      return String.format("%.1f %sB", bytes / Math.pow(unit.toDouble(), exp.toDouble()), pre)
    }

    init {
      Log.d(TAG, "Loading library libDemoQuillPlayer.so")
      System.loadLibrary("DemoQuillPlayer")
      System.setProperty("kotlinx.coroutines.debug", if (BuildConfig.DEBUG) "on" else "off")
    }
  }

  // Text texture data
  var textTextureID: Int = -1
  var textTextureWidth: Int = 0
  var textTextureHeight: Int = 0
  var textMetaDataHeight: Float = 0.0f
  private var textBitmap: Bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888)
  private val textPaint: TextPaint = TextPaint(Paint.ANTI_ALIAS_FLAG)

  @Keep
  fun initTextTexture(textureId: Int, width: Int, height: Int) {
    textTextureID = textureId
    textTextureWidth = width
    textTextureHeight = height

    textBitmap = Bitmap.createBitmap(textTextureWidth, textTextureHeight, Bitmap.Config.ARGB_8888)
    textBitmap.eraseColor(Color.TRANSPARENT)
  }

  private fun uploadTextTexture() {
    // Update the GL texture to GPU
    GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, textTextureID)
    GLUtils.texImage2D(GLES32.GL_TEXTURE_2D, 0, GLES32.GL_RGBA, textBitmap, 0)
    GLES32.glGenerateMipmap(GLES32.GL_TEXTURE_2D)
  }

  @Keep
  fun updateTextTextureMetadata(
      title: String,
      artist: String,
      date: String,
      description: String
  ): Float {
    // Clear the background
    val bitmapCanvas = Canvas(textBitmap)
    // background color
    textPaint.color = Color.rgb(55, 55, 55)
    bitmapCanvas.drawRect(
        0.0f, 0.0f, textTextureWidth.toFloat() * 0.66f, textTextureHeight.toFloat(), textPaint)
    // up next column
    textPaint.color = Color.rgb(35, 35, 35)
    bitmapCanvas.drawRect(
        textTextureWidth * 0.66f,
        0.0f,
        textTextureWidth.toFloat(),
        textTextureHeight.toFloat(),
        textPaint)

    textPaint.color = Color.rgb(204, 204, 204)

    val mainPanelWidth = textTextureWidth * 0.66f
    val upNextWidth = textTextureWidth * 0.33f
    val titleBlockIndent = 150.0f
    val horizontalMargin = 40.0f
    val topMargin = 40.0f
    val verticalSpacing = 5.0f

    val titleBlockWidth =
        mainPanelWidth.toInt() - titleBlockIndent.toInt() - horizontalMargin.toInt()

    // Title
    textPaint.setTextSize(24.0f)
    val titleBuilder =
        StaticLayout.Builder.obtain(title, 0, title.length, textPaint, titleBlockWidth)
            .setAlignment(Layout.Alignment.ALIGN_NORMAL)
            .setEllipsize(TruncateAt.END)
            .setMaxLines(2)
    val titleLayout = titleBuilder.build()
    val titleHeight = titleLayout.height.toFloat()
    bitmapCanvas.translate(titleBlockIndent, topMargin)
    titleLayout.draw(bitmapCanvas)

    // Artist
    val artistBuilder =
        StaticLayout.Builder.obtain(artist, 0, artist.length, textPaint, titleBlockWidth)
            .setAlignment(Layout.Alignment.ALIGN_NORMAL)
            .setEllipsize(TruncateAt.END)
            .setMaxLines(1)
    val artistLayout = artistBuilder.build()
    val artistHeight = artistLayout.height.toFloat()
    bitmapCanvas.translate(0.0f, titleHeight + verticalSpacing)
    artistLayout.draw(bitmapCanvas)

    // Date
    textPaint.setTextSize(20.0f)
    val dateLayout =
        StaticLayout(
            date, textPaint, titleBlockWidth, Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false)
    val dateHeight = dateLayout.height.toFloat()
    bitmapCanvas.translate(0.0f, artistHeight + verticalSpacing)
    dateLayout.draw(bitmapCanvas)

    // Description
    textPaint.setTextSize(22.0f)

    val descBuilder =
        StaticLayout.Builder.obtain(
                description,
                0,
                description.length,
                textPaint,
                mainPanelWidth.toInt() - horizontalMargin.toInt() * 2)
            .setAlignment(Layout.Alignment.ALIGN_NORMAL)
            .setEllipsize(TruncateAt.END)
            .setMaxLines(12)
    val descLayout = descBuilder.build()
    val descriptionHeight = descLayout.height.toFloat()
    bitmapCanvas.translate(
        -titleBlockIndent + horizontalMargin, dateHeight + verticalSpacing * 3.0f)
    descLayout.draw(bitmapCanvas)

    // This exported for positioning in the ovrApp, as a percentage of panel height
    textMetaDataHeight =
        (topMargin +
            titleHeight +
            artistHeight +
            dateHeight +
            descriptionHeight +
            verticalSpacing * 5.0f) / textTextureHeight.toFloat()

    uploadTextTexture()

    return textMetaDataHeight
  }

  private val progressListener: ProgressListener =
      object : ProgressListener {
        var firstUpdate = true
        var listenId: String? = null

        override fun listenFor(id: String) {
          listenId = id
        }

        override fun update(
            startTime: Long,
            progressId: String,
            bytesRead: Long,
            contentLength: Long,
            done: Boolean
        ) {
          if (!isActive) {
            cancelDownload()
          }

          if (progressId != listenId) return

          if (done) {
            Log.d(TAG, "completed download!")
            val elapsedDownloadTime = System.currentTimeMillis() - startTime
            nativeSetDownloadingInfo(
                bytesRead, humanReadableByteCount(bytesRead), 100, elapsedDownloadTime)
            return
          }

          if (firstUpdate) {
            firstUpdate = false
            Log.d(
                TAG,
                "content-length: " +
                    if (contentLength == -1L) "unknown" else humanReadableByteCount(contentLength))
          }

          if (contentLength != -1L) {
            val progress = (100 * bytesRead / contentLength).toInt()
            val elapsedDownloadTime = System.currentTimeMillis() - startTime
            nativeSetDownloadingInfo(bytesRead, "", progress, elapsedDownloadTime)
          }
        }
      }

  private val isGalleryBuild = BuildConfig.FLAVOR == "gallery"
  private val isRetailDemoBuild = BuildConfig.FLAVOR == "retail_demo"

  private val okHttpClient =
      OkHttpClient.Builder()
          .addNetworkInterceptor { chain ->
            val originalResponse = chain.proceed(chain.request())
            val tag = chain.request().tag() as String?
            if (tag != null) {
              originalResponse
                  .newBuilder()
                  .body(ProgressResponseBody(tag, originalResponse.body(), progressListener))
                  .build()
            } else originalResponse
          }
          .build()

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)

    if (intent != null)
        Log.d(TAG, "onCreate intent category: " + intent.categories + " action: " + intent.action)
    else Log.d(TAG, "onCreate null intent")

    if (!requestExternalStoragePermission())
    // finishInit() will be called in onRequestPermissionsResult
    return

    finishInit()
  }

  private fun finishInit() {
    QuillPlayer.assetsDirectory = applicationContext.filesDir.path + "/"
    QuillPlayer.cacheDirectory = filesDir.path + "/cache/"
    val cacheDir = File(QuillPlayer.cacheDirectory)
    if (!cacheDir.exists()) {
      cacheDir.mkdir()
    }
    nativeSetAssetDirectory(QuillPlayer.assetsDirectory)
    unpackQuillAssets()

    if (!handleNewIntent(intent)) {
      if (isRetailDemoBuild) {
        loadImm("${applicationContext.filesDir.path}/retail-demo.imm")
      }
    }
  }

  override fun onNewIntent(intent: Intent) {
    super.onNewIntent(intent)
    Log.d(
        TAG,
        "onNewIntent intent package: ${intent.`package`} scheme: ${intent.scheme} type: ${intent.type} category: ${intent.categories} action: ${intent.action} data: ${intent.dataString} flags: ${intent.flags}")
    handleNewIntent(intent)
  }

  override fun onResume() {
    super.onResume()

    if (intent != null)
        Log.d(
            TAG,
            "onResume intent  package: ${intent.`package`} scheme: ${intent.scheme} type: ${intent.type} category: ${intent.categories} action: ${intent.action} data: ${intent.dataString} flags: ${intent.flags}")
    else Log.d(TAG, "onResume null intent")
  }

  override fun onDestroy() {
    cancelDownload()
    cancel()

    super.onDestroy()
    Log.d(TAG, "onDestroy")
  }

  override fun onPause() {
    Log.d(TAG, "onPause")
    super.onPause()
  }

  private fun handleNewIntent(intent: Intent): Boolean {

    logIntentExtras(intent)

    // Dev Gallery uses intent extras to launch Quillustrations
    if (handleIntentExtras(intent) || isGalleryBuild) {
      return true
    }

    return false
  }

  private fun handleIntentExtras(intent: Intent): Boolean {
    // Check if intent was launched with the Quill disk path
    val extras = intent.extras
    if (extras != null) { // Should be either eye or floor
      if (extras.containsKey(EXTRA_QUILL_TRACKING_LEVEL)) {
        val level = extras.getString(EXTRA_QUILL_TRACKING_LEVEL, "")
        if (!level.isEmpty()) {
          Log.d(TAG, "found tracking level extra $level")
          nativeSetTrackingTransformLevel(level)
        }
      }
      if (extras.containsKey(EXTRA_QUILL_EYE_BUFFER_SCALE)) {
        val scaleFactor = extras.getFloat(EXTRA_QUILL_EYE_BUFFER_SCALE)
        Log.d(TAG, "found eye buffer scale extra $scaleFactor")
        if (scaleFactor <= 0) {
          Log.e(TAG, "invalid eye buffer scale: $scaleFactor")
        } else {
          nativeSetEyeBufferScale(scaleFactor)
        }
      }
      if (extras.containsKey(EXTRA_QUILL_RENDERING_TECHNIQUE)) {
        val renderingTechnique = extras.getInt(EXTRA_QUILL_RENDERING_TECHNIQUE)
        if (renderingTechnique >= 0) {
          Log.d(TAG, "found renderingTechnique extra $renderingTechnique")
          nativeSetQuillRenderingTechnique(renderingTechnique)
        }
      }
      if (extras.containsKey(EXTRA_QUILL_PLAYER_SPAWN_LOCATION)) {
        val spawnLocation = extras.getString(EXTRA_QUILL_PLAYER_SPAWN_LOCATION)
        Log.d(TAG, "found player spawn location extra $spawnLocation")
        nativeSetPlayerSpawnLocation(spawnLocation)
      }
      // getInt failed, so parsing integer from string here
      val quillCreationTime = extras.getString(EXTRA_QUILL_CREATION_TIME, "0").toInt()
      val quillName = extras.getString(EXTRA_QUILL_NAME)
      val quillArtist = extras.getString(EXTRA_QUILL_ARTIST_NAME)
      nativeSetPlayerMetadata(
          quillName, quillArtist, quillCreationTime, null, null, null, false, false)
      if (extras.containsKey(EXTRA_QUILL_PATH)) {
        val path = extras.getString(EXTRA_QUILL_PATH)
        path?.let {
          loadImmPath(it)
          return true
        }
      }
    }
    return false
  }

  private fun hasNetworkConnection(): Boolean {
    var hasConnectedWifi = false
    var hasConnectedMobile = false
    val cm = getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    val netInfo = cm.allNetworkInfo
    for (ni in netInfo) {
      if (ni.typeName.equals("WIFI", ignoreCase = true)) if (ni.isConnected) hasConnectedWifi = true
      if (ni.typeName.equals("MOBILE", ignoreCase = true))
          if (ni.isConnected) hasConnectedMobile = true
    }
    return hasConnectedWifi || hasConnectedMobile
  }

  @Keep
  fun cancelDownload() {
    val dispatcher = okHttpClient.dispatcher()
    val canceledCount = dispatcher.queuedCalls().size + dispatcher.runningCalls().size
    dispatcher.queuedCalls().forEach { it.cancel() }
    dispatcher.runningCalls().forEach { it.cancel() }
    if (canceledCount > 0) Log.d(TAG, "canceled $canceledCount network requests(s).")
  }

  private fun loadImmPath(path: String) {
    if (URLUtil.isNetworkUrl(path)) {
      if (!hasNetworkConnection()) {
        loadErrorImm(Message.Disconnected)
        return
      }
      Log.d(TAG, "network I path: $path")
      loadImm(Imm.Loading.value)
      launch {
        when (val result = downloadImm(path)) {
          is Success -> loadImm(result.data.path)
          is Failure -> loadErrorImm(result.message)
        }
      }
    } else {
      Log.d(TAG, "Loading IMM: $path")
      loadImm(path)
    }
  }

  private fun loadImm(path: String) {
    nativeSendMessage(path, Message.LoadImmPath.value)
  }

  private fun loadErrorImm(message: Message) {
    nativeSetTrackingTransformLevel("floor")
    nativeSetPlayerSpawnLocation("Default")
    nativeSetQuillRenderingTechnique(PaintRenderingTechnique.Static.value)

    if (message == Message.Disconnected) {
      nativeSetPlayerMetadata(
          "No internet connection available", null, 0, null, null, null, true, false)
      nativeSendMessage(Imm.Error.value, message.value)
    } else {
      nativeSendMessage(Imm.Error.value, message.value)
    }
  }

  private suspend fun downloadImm(url: String): Result<Asset> = suspendCancellableCoroutine {
    val request = Request.Builder().url(url).build()

    Log.d(TAG, "Downloading IMM: $url")

    okHttpClient
        .newCall(request)
        .enqueue(
            object : Callback {
              override fun onFailure(call: Call, e: IOException) {
                if (!isActive) {
                  return
                }
                Log.e(TAG, "Failed to download: $url Exception: $e")
                it.resume(Failure(Message.ErrorUnknown))
              }

              @Throws(IOException::class)
              override fun onResponse(call: Call, response: Response) {
                if (!response.isSuccessful) {
                  Log.e(TAG, "Response not successful: \n$response")
                  it.resume(Failure(Message.ErrorUnknown))
                  return
                }

                // If the coroutine is cancelled, abort writing to disk
                if (it.isCancelled) {
                  Log.d(TAG, "Cancel download $url")
                  return
                }

                // Use a unique name to cache IMM's from EntQuillustrations
                val filename = "download.imm"
                val downloadedFile = File(QuillPlayer.cacheDirectory, filename)
                Log.d(TAG, "Writing IMM to " + downloadedFile.absolutePath)

                val time = measureTimeMillis {
                  val sink = Okio.buffer(Okio.sink(downloadedFile))
                  if (response.body() == null) {
                    Log.e(TAG, "Null response body for request: $url")
                    if (downloadedFile.exists()) {
                      downloadedFile.delete()
                    }
                    it.resume(Failure(Message.ErrorUnknown))
                    return
                  }

                  sink.writeAll(response.body()!!.source())
                  sink.close()

                  if (!downloadedFile.exists()) {
                    Log.e(TAG, "File failed to write: $url")
                    it.resume(Failure(Message.ErrorUnknown))
                    return
                  }
                }

                Log.d(TAG, "Writing completed in $time ms")

                it.resume(Success(Asset(downloadedFile.path)))
              }
            })
  }

  private fun unpackQuillAssets() {
    val shouldReload = QuillPlayer.shouldReloadAssets()

    ExtractAssetsTask(this, QuillPlayer.quillErrorPath, null, shouldReload)
        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)

    if (isRetailDemoBuild)
        ExtractAssetsTask(this, QuillPlayer.quillDemoPath, null, shouldReload)
            .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)

    // Extract uiLib data.
    val uiLoadTimer =
        AssetLoadTimer(TimingLogger(TAG, "uiLib asset extraction"), QuillPlayer.uiLibAssets.size)
    for (uiLibAsset in QuillPlayer.uiLibAssets) {
      ExtractAssetsTask(this, uiLibAsset, uiLoadTimer, shouldReload)
          .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
    }

    val controllersLoadTimer =
        AssetLoadTimer(
            TimingLogger(TAG, "controller asset extraction"), QuillPlayer.controllerAssets.size)
    for (controllerAsset in QuillPlayer.controllerAssets) {
      ExtractAssetsTask(this, controllerAsset, controllersLoadTimer, shouldReload)
          .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
    }

    // We should only ovewrite the asset version file here after all assets from Gallery and for
    // player have been loaded.
    val forceExtract = true
    ExtractAssetsTask(this, QuillPlayer.assetVersionFile, null, forceExtract)
        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
  }

  private fun requestExternalStoragePermission(): Boolean {
    if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) !=
        PackageManager.PERMISSION_GRANTED ||
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) !=
            PackageManager.PERMISSION_GRANTED) {

      ActivityCompat.requestPermissions(
          this,
          arrayOf(
              Manifest.permission.READ_EXTERNAL_STORAGE,
              Manifest.permission.WRITE_EXTERNAL_STORAGE,
              Manifest.permission.ACCESS_FINE_LOCATION,
              Manifest.permission.ACCESS_COARSE_LOCATION),
          PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE)
      return false
    }
    return true
  }

  override fun onRequestPermissionsResult(
      requestCode: Int,
      permissions: Array<String>,
      grantResults: IntArray
  ) {
    if (requestCode != PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE) {
      return
    }
    if (grantResults.size <= 0 || grantResults[0] != PackageManager.PERMISSION_GRANTED) {
      Log.e(TAG, "Access not granted for reading local Quill media in external storage")
    }
    finishInit()
  }
}
