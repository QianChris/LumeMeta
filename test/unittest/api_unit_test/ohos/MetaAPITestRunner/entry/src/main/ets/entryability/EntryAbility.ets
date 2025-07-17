import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';

export default class EntryAbility extends UIAbility {
    onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
    }

    onDestroy(): void {
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    }

    onWindowStageCreate(windowStage: window.WindowStage): void {
        // Main window is created, set main page for this ability
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

        // Obtain the main window of the application
        let windowClass: window.Window | null = null;
        windowStage.getMainWindow((err: BusinessError, data) => {
            let errCode: number = err.code;
            if (errCode) {
                hilog.error(0x0000, 'testTag', 'Failed to obtain the main window. Cause: ' + JSON.stringify(err));
                return;
            }
            windowClass = data;
            hilog.info(0x0000, 'testTag', 'Succeeded in obtaining the main window. Data: ' + JSON.stringify(data));

            // Implement the immersive effect by hiding the status bar and navigation bar
            let names: Array<'status' | 'navigation'> = [];
            windowClass.setWindowSystemBarEnable(names)
                .then(() => {
                    hilog.info(0x0000, 'testTag', 'Succeeded in setting the system bar to be visible.');
                })
                .catch((err: BusinessError) => {
                    hilog.error(0x0000, 'testTag', 'Failed to set the system bar to be visible. Cause:' + JSON.stringify(err));
                });
            // Setting properties of the status bar and navigation bar
            /*let isLayoutFullScreen = true;
            windowClass.setWindowLayoutFullScreen(isLayoutFullScreen)
                .then(() => {
                    hilog.info(0x0000, 'testTag', 'Succeeded in setting the window layout to full-screen mode.');
                })
                .catch((err: BusinessError) => {
                    hilog.error(0x0000, 'testTag', 'Failed to set the window layout to full-screen mode. Cause:' + JSON.stringify(err));
                });
            let sysBarProps: window.SystemBarProperties = {
                statusBarColor: '#ff00ff',
                navigationBarColor: '#00ff00',
                // The following properties are supported since API version 8.
                statusBarContentColor: '#ffffff',
                navigationBarContentColor: '#ffffff'
            };
            windowClass.setWindowSystemBarProperties(sysBarProps)
                .then(() => {
                    hilog.info(0x0000, 'testTag', 'Succeeded in setting the system bar properties.');
                })
                .catch((err: BusinessError) => {
                    hilog.error(0x0000, 'testTag', 'Failed to set the system bar properties. Cause: ' + JSON.stringify(err));
                });*/
        })

        // Load content to the immersive window
        windowStage.loadContent('pages/Index', (err) => {
            if (err.code) {
                hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
                return;
            }
            hilog.info(0x0000, 'testTag', 'Succeeded in loading the content.');
        });
    }

    onWindowStageDestroy(): void {
        // Main window is destroyed, release UI related resources
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
    }

    onForeground(): void {
        // Ability has brought to foreground
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
    }

    onBackground(): void {
        // Ability has back to background
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
    }
};
