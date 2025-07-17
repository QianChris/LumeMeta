package com.huawei.meta_api_test_runner.test;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class NativeTestRunner implements WinManager.SurfaceListener {

    // Load native library
    static {
        System.loadLibrary("LumeMetaAPITestRunner");
    }

    // Native methods
    public native int onSurfaceCreatedNative(Context context, Surface surface, String reportFile, String[] arguments);

    private int mIsSurfaceAlive = -1;
    private WinManager mWinManager;
    private Context mAppContext;
    private String[] mInputArgs;
    private String mReportFile;
    private final ExecutorService mExecutorService;
    private int mTestResults;

    // Constructor
    public NativeTestRunner() {
        // Initialize a single-threaded executor for background tasks (heavy tasks)
        mExecutorService = Executors.newSingleThreadExecutor();
    }

    @Override
    public void onSurfaceCreated(SurfaceHolder holder) {

        // Surface is created, call the native function in a separate thread to not block the main
        // UI thread
        mExecutorService.submit(() -> {
            mTestResults = onSurfaceCreatedNative(mAppContext, holder.getSurface(), mReportFile, mInputArgs);

            new Handler(Looper.getMainLooper()).post(() -> {
                // Update UI here
                mIsSurfaceAlive = 1;

                // Destroy surface
                mWinManager.releaseSurface();
            });
        });
    }

    @Override
    public void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

    @Override
    public void onSurfaceDestroyed(SurfaceHolder holder) {
        mIsSurfaceAlive = 0;
    }

    /**
     * Runs native tests with given arguments.
     * NOTE: should not be called multiple times during the process lifetime.
     *
     * @param appContext a valid app context
     * @param args arguments that are passed to google test env.
     */
    public void runNativeTests(Context appContext, String[] args) {

        // Get application or service context
        mAppContext = appContext;

        // Using the app external storage to save report
        String outputDir = mAppContext.getExternalFilesDir(null).toString();
        Path outputTxtFile = Paths.get(outputDir,"report-All.txt");
        mReportFile = outputTxtFile.toString();

        // COpy input arguments
        mInputArgs = args.clone();

        // Create window and a surface using main UI thread
        mWinManager = new WinManager(mAppContext, this);

        // Block this thread and wait for the tests to run
        while(isSurfaceDestroyed() != 0) {}

        // Destroy single-threaded executor
        shutdown();

        // Display report
        String reportTxt;
        try {
            reportTxt = "\n" + new String(Files.readAllBytes(outputTxtFile), StandardCharsets.UTF_8);
        } catch (IOException e) {
            reportTxt = "Report not found from: x" + outputTxtFile.toString();
        }
        // Write to System.out instead of Log (buffer limitations)
        System.out.println(reportTxt);

        if (mTestResults != 0) {
            // We are not interested in this Java stack trace so trying to fail with minimal clutter.
            // something like assertEquals(reportTxt, 0, result); would print a large Java stack trace
            // that is not useful. Erasing most lines from the exception.
            RuntimeException exception = new RuntimeException(reportTxt);
            StackTraceElement elem1[] = new StackTraceElement[] {exception.getStackTrace()[0]};
            exception.setStackTrace(elem1);
            throw exception;
        }
    }

    /** Check if Surface is alive:<br>
     * Return "-1" - it was not created! <br>
     * Return "0" - it was destroyed! <br>
     * Return "1" - it is alive!
     */
    public int isSurfaceDestroyed() {
        return mIsSurfaceAlive;
    }

    /** Shutdown single-threaded executor */
    private void shutdown() {
        mExecutorService.shutdown();
    }
}
