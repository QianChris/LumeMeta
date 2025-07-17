package com.huawei.meta_api_test_runner.test;

import android.os.Bundle;
import android.content.Context;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.platform.app.InstrumentationRegistry;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class MainTestCases {

    private String[] mInputArgs;
    private Context mAppContext;

    // Add element to an array
    private <T> T[] append(T[] arr, T element) {
        final int N = arr.length;
        arr = Arrays.copyOf(arr, N + 1);
        arr[N] = element;
        return arr;
    }
    private void parseInputArgs() {
        // Get target app context
        mAppContext = androidx.test.core.app.ApplicationProvider.getApplicationContext();

        // Check that we have the correct context.
        Assert.assertEquals("com.huawei.meta_api_test_runner", mAppContext.getPackageName());

        // Using the app external storage to save report and gtest files
        String outputDir = mAppContext.getExternalFilesDir(null).toString();
        Path outputGtestFile = Paths.get(outputDir,"gtestReport_Android.xml");

        // Parsing arguments (<key>,<value>)
        Bundle arguments = InstrumentationRegistry.getArguments();
        String gtestFilterValue = arguments.getString("gtest_filter");
        String gtestOutputValue = arguments.getString("gtest_output");

        // Initialize args array
        mInputArgs = new String[0];

        // If no arguments to gtest_filter, run all tests (default)
        if(gtestFilterValue == null) {
            mInputArgs = append(mInputArgs, "--gtest_filter=*");
        } else {
            mInputArgs = append(mInputArgs, "--gtest_filter=" + gtestFilterValue);
        }
        // If no arguments to gtest_output, set default path
        if(gtestOutputValue == null) {
            mInputArgs = append(mInputArgs, "--gtest_output=xml:" + outputGtestFile.toString());
        } else {
            mInputArgs = append(mInputArgs, "--gtest_output=" + gtestOutputValue);
        }
    }

    /** Run tests by parsing arguments to "--gtest_filter" or/and "--gtest_output" using "-e" flag
     * and "gtest_filter" key or/and "gtest_output" key. <br><br>
     * Cmd example: <br>
     * adb shell am instrument
     * -e class com.huawei.render_src_test_runner.test.MainTestCases#runTests
     * -e gtest_filter SRC_RenderNodeGraphLoader.*
     * -e gtest_output xml:/sdcard/Android/data/com.huawei.render_src_test_runner/files/gtestReport_Android.xml
     * -w com.huawei.render_src_test_runner.test/androidx.test.runner.AndroidJUnitRunner <br><br>
     * Default value for "--gtest_filter" is "*" <br>
     * Default value for "--gtest_output" is target app's external storage
     * */
    @Test
    public void runTests() {
        parseInputArgs();
        NativeTestRunner testRun = new NativeTestRunner();
        testRun.runNativeTests(mAppContext, mInputArgs);
    }
}


