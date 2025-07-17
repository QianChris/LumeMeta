package com.huawei.meta_api_test_runner.test;

import android.content.Context;
import android.graphics.PixelFormat;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.util.Log;

/** Create a window and add a surface using UI thread */
public class WinManager {

    /** Layout flags for window:
     * @see WindowManager.LayoutParams
     */
    static class WindowFlag {

        /** Prevents the window from receiving input focus */
        public static final int NOT_FOCUSABLE = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

        /** Keeps the screen on while the window is visible */
        public static final int KEEP_SCREEN_ON = WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;

        /** Hides all screen decorations (e.g., status bar) */
        public static final int FULLSCREEN = WindowManager.LayoutParams.FLAG_FULLSCREEN;

        /** Allows the window to extend outside of the screen's bounds (useful for overlays) */
        public static final int LAYOUT_IN_SCREEN = WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;

        /** Extends the layout to the full area, including the status bar area */
        public static final int LAYOUT_NO_LIMITS = WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

        /** Dim everything behind the window */
        public static final int DIM_BEHIND = WindowManager.LayoutParams.FLAG_DIM_BEHIND;

        /** Prevents the contents of the window from being captured (e.g., screenshots) */
        public static final int SECURE = WindowManager.LayoutParams.FLAG_SECURE;
    }

    /** Layout type for window:
     * @see WindowManager.LayoutParams
     */
    static class WindowType {

        /** Standard application window - This is the default type for application windows. It can
         * be used for typical activity windows in your app */
        public static final int APPLICATION = WindowManager.LayoutParams.TYPE_APPLICATION;

        /** Panels or side menus - Used for secondary windows, such as context menus or side panels.
         *  It is typically used for smaller windows */
        public static final int APPLICATION_PANEL = WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;

        /** Media windows - Designed for media-related windows (e.g., videos) that are supposed to
         * appear on top of application windows, such as media controls */
        public static final int APPLICATION_MEDIA = WindowManager.LayoutParams.TYPE_APPLICATION_MEDIA;

        /** Floating windows or overlays - Allows an application to create a window that floats
         * above other application windows, but is still behind system windows. This is often used
         * for chat heads or other floating tools
         * Requires Manifest.permission.SYSTEM_ALERT_WINDOW permission*/
        public static final int APPLICATION_OVERLAY = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;

        /** Status bar window - Used to create windows that can display system status, but this
         * type is generally used internally by the system and not for third-party apps */
        public static final int STATUS_BAR = WindowManager.LayoutParams.TYPE_STATUS_BAR;

        /** The search bar - There can be only one search bar window; it is placed at the top of
         * the screen. In multiuser systems shows on all users' windows */
        public static final int SEARCH_BAR  = WindowManager.LayoutParams.TYPE_SEARCH_BAR;

        /** Windows that are overlaid only by a connected AccessibilityService for interception of
         * user interactions without changing the windows an accessibility service can introspect.
         * In particular, an accessibility service can introspect only windows that a sighted user
         * can interact with which is they can touch these windows or can type into these windows.
         * For example, if there is a full screen accessibility overlay that is touchable, the
         * windows below it will be introspectable by an accessibility service even though they are
         * covered by a touchable window */
        public static final int ACCESSIBILITY_OVERLAY  = WindowManager.LayoutParams.TYPE_ACCESSIBILITY_OVERLAY;

        /** Like APPLICATION_PANEL, but layout of the window happens as that of a top-level window,
         * not as a child of its container */
        public static final int APPLICATION_ATTACHED_DIALOG  = WindowManager.LayoutParams.TYPE_APPLICATION_ATTACHED_DIALOG;

        /** Special application window that is displayed while the application is starting. Not for
         *  use by applications themselves; this is used by the system to display something until
         *  the application can show its own windows. In multiuser systems shows on all users'
         *  windows*/
        public static final int APPLICATION_STARTING  = WindowManager.LayoutParams.TYPE_APPLICATION_STARTING;

        /** Window for Presentation on top of private virtual display */
        public static final int PRIVATE_PRESENTATION  = WindowManager.LayoutParams.TYPE_PRIVATE_PRESENTATION;

        /** Panel that slides out from the status bar In multiuser systems shows on all users' windows */
        public static final int SYSTEM_DIALOG  = WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG;

        /** Wallpaper window - placed behind any window that wants to sit on top of the wallpaper.
         *  In multiuser systems shows only on the owning user's window */
        public static final int WALLPAPER  = WindowManager.LayoutParams.TYPE_WALLPAPER;

        /** A sub-panel on top of an application window. These windows are displayed on top their
         * attached window and any TYPE_APPLICATION_PANEL panels */
        public static final int APPLICATION_SUB_PANEL  = WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL;

        /** An application window that serves as the "base" window of the overall application;
         *  all other application windows will appear on top of it. In multiuser systems shows
         *  only on the owning user's window */
        public static final int BASE_APPLICATION  = WindowManager.LayoutParams.TYPE_BASE_APPLICATION;

        /** A variation on TYPE_APPLICATION that ensures the window manager will wait for this
         * window to be drawn before the app is shown. In multiuser systems shows only on the
         * owning user's window */
        public static final int DRAWN_APPLICATION  = WindowManager.LayoutParams.TYPE_DRAWN_APPLICATION;

        /** */
        public static final int CHANGED  = WindowManager.LayoutParams.TYPE_CHANGED;

        /** Internal input methods windows, which appear above the normal UI. Application windows
         *  may be resized or panned to keep the input focus visible while this window is displayed.
         *  In multiuser systems shows only on the owning user's window */
        public static final int INPUT_METHOD  = WindowManager.LayoutParams.TYPE_INPUT_METHOD;

        /** Internal input methods dialog windows, which appear above the current input method
         * window. In multiuser systems shows only on the owning user's window */
        public static final int INPUT_METHOD_DIALOG  = WindowManager.LayoutParams.TYPE_INPUT_METHOD_DIALOG;

        /** Dialogs that the keyguard shows In multiuser systems shows on all users' windows */
        public static final int KEYGUARD_DIALOG  = WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG;
    }

    /** Pixel format for window:
     * @see PixelFormat
     */
    static class WindowPixelFormat {
        /**  Indicates that the pixel format is unknown or not set. This is a placeholder value */
        public static final int UNKNOWN = PixelFormat.UNKNOWN;

        /** Represents a pixel format with alpha channel support (translucency) but without color
         * depth. This format is used when you need to support transparent backgrounds in your
         * views */
        public static final int TRANSLUCENT = PixelFormat.TRANSLUCENT;

        /** Represents a fully transparent pixel format. This format is typically used for
         * completely transparent windows or views that do not display any color information */
        public static final int TRANSPARENT = PixelFormat.TRANSPARENT;

        /** Represents a pixel format without alpha channel (fully opaque). This is used when you
         * do not need transparency and want a solid, non-transparent color */
        public static final int OPAQUE = PixelFormat.OPAQUE;

        /** 32-bit pixel format with 8 bits each for red, green, blue, and alpha channels. This
         * format provides high-quality color representation and alpha blending*/
        public static final int RGBA_8888 = PixelFormat.RGBA_8888;

        /** 32-bit pixel format with 8 bits each for red, green, and blue channels, with an unused
         * alpha channel (x). This format does not support alpha transparency but provides
         * high-quality color */
        public static final int RGBX_8888 = PixelFormat.RGBX_8888;

        /** 16-bit pixel format with 5 bits for red, 6 bits for green, and 5 bits for blue.
         * This format is used for lower color depth and is more memory efficient compared to
         * RGBA_8888 */
        public static final int RGB_565 = PixelFormat.RGB_565;

        /** 64-bit pixel format with 16 bits each for red, green, blue, and alpha channels. This
         * format provides even more high-quality color representation and alpha blending compared
         * to RGBA_8888 */
        public static final int RGBA_F16 = PixelFormat.RGBA_F16;

        /** 32-bit pixel format with 10 bits each for red, green, and blue channels, and 2 bits for
         * alpha channel*/
        public static final int RGBA_1010102 = PixelFormat.RGBA_1010102;
    }

    private Context mContext;
    private SurfaceListener mSurfaceListener;
    private WindowManager mWinMngr;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private WindowManager.LayoutParams mLayoutParams;
    private HandlerThread mHandlerThread;
    private Handler mHandler;

    public WinManager(Context appContext, SurfaceListener listener) {

        // Check if overlay permission is granted to the target application context
        if (!Settings.canDrawOverlays(appContext)) {
            Log.e(appContext.getPackageName(), "Overlay permission is not granted!");
            System.exit(1);
        }

        mContext = appContext;
        mSurfaceListener = listener;

        // Setup window:
        // Get android window manager
        mWinMngr = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);

        if(mWinMngr == null) {
            Log.e(appContext.getPackageName(), "Couldn't get Window service!");
            System.exit(1);
        }

        // Configure default layout parameters
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,   // Width
                WindowManager.LayoutParams.MATCH_PARENT,   // Height
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,  // TYPE_APPLICATION_OVERLAY (Android API 26+)
                WindowManager.LayoutParams.FLAG_FULLSCREEN |  // Hide the status bar
                        WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS |  // Allow the window to extend beyond the screen limits
                        WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN |  // Allow the window to appear within the entire screen
                        WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |  // Prevent the window from receiving input focus
                        WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |  // Keep the screen on while the window is visible
                        WindowManager.LayoutParams.FLAG_DIM_BEHIND,  // Dim the screen behind the window
                PixelFormat.RGB_565  // Pixel format
        );

        // Create a surfaceView
        mSurfaceView = new SurfaceView(mContext);

        // Set the window to cover the entire screen and hide the navigation bar
        layoutParams.systemUiVisibility = mSurfaceView.getSystemUiVisibility() |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;  // For an immersive full-screen experience

        // Position the view at the center of the screen
        layoutParams.gravity = Gravity.CENTER;
        mLayoutParams = layoutParams;

        // Handle to the main UI thread
        mHandler = new Handler(Looper.getMainLooper());

        //Add the SurfaceView to the Window
        mHandler.post(() -> {
            try {
                mWinMngr.addView(mSurfaceView, mLayoutParams);
            } catch (Exception e) {
                e.printStackTrace();  // Check the logcat for detailed error information
            }

            // Get the SurfaceHolder from the SurfaceView
            mSurfaceHolder = mSurfaceView.getHolder();

            // Set SurfaceHolder callbacks
            mSurfaceHolder.addCallback(new SurfaceHolder.Callback() {
                // Surface is created
                @Override
                public void surfaceCreated(SurfaceHolder holder) {
                    // Surface is created, ready for drawing
                    if (mSurfaceListener != null) {
                        mSurfaceListener.onSurfaceCreated(holder); // Notify the listener
                    }
                }

                // Surface changed (dimensions or format)
                @Override
                public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                    mSurfaceListener.onSurfaceChanged(holder, format, width, height); // Notify the listener
                }

                // Surface is destroyed
                @Override
                public void surfaceDestroyed(SurfaceHolder holder) {
                    mSurfaceListener.onSurfaceDestroyed(holder); // Notify the listener
                }
            });
        });
    }

    /** Interface to retrieve the state of a surface*/
    public interface SurfaceListener {
        void onSurfaceCreated(SurfaceHolder holder);
        void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height);
        void onSurfaceDestroyed(SurfaceHolder holder);
    }

    /** Get window manager obj */
    public WindowManager getWindowManager() {
        return mWinMngr;
    }

    /** Set window flags and update view layout: {@link WindowFlag} */
    public void setWindowFlag(int winFlag) {

       WindowManager.LayoutParams params = (WindowManager.LayoutParams) mSurfaceView.getLayoutParams();
       params.flags = winFlag;

        // Update the layout parameters
        updateViewLayout(params);
    }

    /** Set window pixel format and update view layout: {@link WindowPixelFormat} */
    public void setWindowPixelFormat(int winPixelFormat) {

        WindowManager.LayoutParams params = (WindowManager.LayoutParams) mSurfaceView.getLayoutParams();
        params.format = winPixelFormat;

        // Update the layout parameters
        updateViewLayout(params);
    }

    /** Set window type and update view layout: {@link WindowType} */
    public void setWindowType(int winType) {

        WindowManager.LayoutParams params = (WindowManager.LayoutParams) mSurfaceView.getLayoutParams();
        params.type = winType;

        // Update the layout parameters
        updateViewLayout(params);
    }

    /** Release surface */
    public void releaseSurface() {
        if (mSurfaceView != null) {
            mHandler.post(() ->
                    mWinMngr.removeView(mSurfaceView)
            );
        }
    }

    /** Get surface holder */
    public SurfaceHolder getSurfaceHandle() {
        if (mSurfaceView != null) {
            return mSurfaceView.getHolder();
        }
        return null;
    }

    /** Update view layout with new params */
    private void updateViewLayout(WindowManager.LayoutParams params) {
        mWinMngr.updateViewLayout(mSurfaceView, params);
    }

}


