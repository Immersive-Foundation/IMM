package com.facebook.arvr.demoquillplayer;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.AsyncTask;
import android.util.Log;
import android.util.TimingLogger;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.lang.ref.WeakReference;
import okhttp3.internal.io.FileSystem;
import okio.BufferedSource;
import okio.Okio;

public class QuillPlayer {
  private static final String TAG = QuillPlayer.class.getSimpleName();

  static final String assetVersionFile = "asset-version.txt";

  // Need to modify the value in |assetVersionFile| every time this is incremented.
  // If this doesn't match the value in |assetVersionFile| then we need to delete and recopy assets.
  // Note the below line is referred in drop_fetch_quill_theater.py in case of change
  private static final int currentAssetVersion = 16;

  // Path to Quill we show when there's an error loading a Quill path.
  static String quillErrorPath = "quill-error.imm";

  static String quillDemoPath = "retail-demo.imm";

  static String[] controllerAssets = {"controllers/controller_new.obj", "controllers/pointer.obj"};

  static String[] uiLibAssets = {
    // Loading animation frames
    "uiLib/loadingSequence/qt_loading_64x64_00.png",
    "uiLib/loadingSequence/qt_loading_64x64_01.png",
    "uiLib/loadingSequence/qt_loading_64x64_02.png",
    "uiLib/loadingSequence/qt_loading_64x64_03.png",
    "uiLib/loadingSequence/qt_loading_64x64_04.png",
    "uiLib/loadingSequence/qt_loading_64x64_05.png",
    "uiLib/loadingSequence/qt_loading_64x64_06.png",
    "uiLib/loadingSequence/qt_loading_64x64_07.png",
    "uiLib/loadingSequence/qt_loading_64x64_08.png",
    "uiLib/loadingSequence/qt_loading_64x64_09.png",
    "uiLib/loadingSequence/qt_loading_64x64_10.png",
    "uiLib/loadingSequence/qt_loading_64x64_11.png",
    "uiLib/loadingSequence/qt_loading_64x64_12.png",
    "uiLib/loadingSequence/qt_loading_64x64_13.png",
    "uiLib/loadingSequence/qt_loading_64x64_14.png",
    "uiLib/loadingSequence/qt_loading_64x64_15.png",
    "uiLib/loadingSequence/qt_loading_64x64_16.png",
    "uiLib/loadingSequence/qt_loading_64x64_17.png",
    "uiLib/loadingSequence/qt_loading_64x64_18.png",
    "uiLib/loadingSequence/qt_loading_64x64_19.png",
    "uiLib/loadingSequence/qt_loading_64x64_20.png",
    "uiLib/loadingSequence/qt_loading_64x64_21.png",
    "uiLib/loadingSequence/qt_loading_64x64_22.png",
    "uiLib/loadingSequence/qt_loading_64x64_23.png",
    "uiLib/loadingSequence/qt_loading_64x64_24.png",
    "uiLib/loadingSequence/qt_loading_64x64_25.png",
    "uiLib/loadingSequence/qt_loading_64x64_26.png",
    "uiLib/loadingSequence/qt_loading_64x64_27.png",
    "uiLib/loadingSequence/qt_loading_64x64_28.png",
    "uiLib/loadingSequence/qt_loading_64x64_29.png",
    "uiLib/loadingSequence/qt_loading_64x64_30.png",
    "uiLib/loadingSequence/qt_loading_64x64_31.png",
    "uiLib/loadingSequence/qt_loading_64x64_32.png",
    "uiLib/loadingSequence/qt_loading_64x64_33.png",
    "uiLib/loadingSequence/qt_loading_64x64_34.png",
    "uiLib/loadingSequence/qt_loading_64x64_35.png",
    "uiLib/loadingSequence/qt_loading_64x64_36.png",
    "uiLib/loadingSequence/qt_loading_64x64_37.png",
    "uiLib/loadingSequence/qt_loading_64x64_38.png",
    "uiLib/loadingSequence/qt_loading_64x64_39.png",
    "uiLib/loadingSequence/qt_loading_64x64_40.png",
    "uiLib/loadingSequence/qt_loading_64x64_41.png",
    "uiLib/loadingSequence/qt_loading_64x64_42.png",
    "uiLib/loadingSequence/qt_loading_64x64_43.png",
    "uiLib/loadingSequence/qt_loading_64x64_44.png",
    "uiLib/loadingSequence/qt_loading_64x64_45.png",
    "uiLib/loadingSequence/qt_loading_64x64_46.png",
    "uiLib/loadingSequence/qt_loading_64x64_47.png",
    "uiLib/loadingSequence/qt_loading_64x64_48.png",
    "uiLib/loadingSequence/qt_loading_64x64_49.png",
    "uiLib/loadingSequence/qt_loading_64x64_50.png",
    "uiLib/loadingSequence/qt_loading_64x64_51.png",
    // End loading animation frames

    "uiLib/quill_logo_512.png",
    "uiLib/profile_128.png",
    "uiLib/unit_cylinder.obj",
    "uiLib/Quill_Icons_Master_64x64.tga",
    "uiLib/sounds/UI_Button_01.wav",
    "uiLib/sounds/UI_Button_02.wav",
    "uiLib/sounds/UI_Button_03.wav",
    "uiLib/fontsSDF/fonts.png",
    "uiLib/fontsSDF/Noto Sans JP-Medium.json"
  };

  static String assetsDirectory;
  static String cacheDirectory;

  // Called once in each onCreate before extraction tasks.
  static boolean shouldReloadAssets() {
    String version = readStringFromFile(assetsDirectory + assetVersionFile);
    if (version == null) {
      Log.d(TAG, "Could not find " + assetVersionFile + " reloading assets from APK...");
      return true;
    }

    // Looks at the existing |assetsVersionFile| in the data directory to see if it matches the
    // compile time |currentAssetVersion|
    boolean reload = version.isEmpty() || Integer.parseInt(version) != currentAssetVersion;
    if (reload) {
      Log.d(
          TAG,
          "Found "
              + assetVersionFile
              + " version: "
              + version
              + " expected: "
              + currentAssetVersion
              + ". Reloading assets from APK...");
    }
    return reload;
  }

  private static String readStringFromFile(String filePath) {
    File file = new File(filePath);
    try {
      BufferedSource source = Okio.buffer(FileSystem.SYSTEM.source(file));
      String result = source.readUtf8();
      source.close();
      return result;
    } catch (IOException ignored) {
    }
    return null;
  }

  static class AssetLoadTimer {
    TimingLogger timingLogger;
    volatile int numAssetsPending;

    AssetLoadTimer(TimingLogger timingLogger, int numAssetsPending) {
      this.timingLogger = timingLogger;
      this.numAssetsPending = numAssetsPending;
    }
  }

  static class ExtractAssetsTask extends AsyncTask<Void, Void, Void> {
    private WeakReference<Context> contextRef;
    private String assetPath;
    private AssetLoadTimer timings;
    private boolean forceExtract;

    ExtractAssetsTask(Context context, String path, AssetLoadTimer timingLogger, boolean force) {
      contextRef = new WeakReference<>(context);
      assetPath = path;
      timings = timingLogger;
      forceExtract = force;
    }

    @Override
    protected Void doInBackground(Void... voids) {
      extractAsset(assetPath);
      // Note that since we're executing in parallel these split times may not be accurate.
      // The total time till all assets are completed should be correct in the log.
      if (timings != null) {
        timings.timingLogger.addSplit(assetPath);
      }
      return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
      if (timings != null && --timings.numAssetsPending == 0) {
        timings.timingLogger.dumpToLog();
      }
      contextRef.clear();
    }

    private void extractAsset(String assetName) {
      String assetPath =
          assetName.startsWith(QuillPlayer.assetsDirectory)
              ? assetName
              : assetsDirectory + assetName;

      File file = new File(assetPath);

      if (!forceExtract && file.exists()) {
        Log.d(TAG, assetPath + " already exists.\n");
        return;
      }

      if (file.exists()) {
        boolean result = file.delete();
      }

      Log.d(TAG, assetName + " extraction needed. \n");

      try {
        Context context = contextRef.get();
        boolean result = file.getParentFile().mkdirs(); // create directories if needed
        RandomAccessFile randomAccessFile = new RandomAccessFile(assetPath, "rw");
        AssetManager assetManager = context.getApplicationContext().getResources().getAssets();
        InputStream inputStream = assetManager.open(assetName);

        byte buffer[] = new byte[1024];
        int count = inputStream.read(buffer, 0, 1024);

        while (count > 0) {
          randomAccessFile.write(buffer, 0, count);
          count = inputStream.read(buffer, 0, 1024);
        }

        randomAccessFile.close();
        inputStream.close();
      } catch (Exception e) {
        Log.e(TAG, "Failure in extractAssets(): " + e.toString() + " " + assetPath);
      }

      if (file.exists()) {
        Log.d(TAG, "File extracted successfully");
      }
    }
  }
}
