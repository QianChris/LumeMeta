import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { abilityDelegatorRegistry } from '@kit.TestKit';
import { UIAbility, Want } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { hilog } from '@kit.PerformanceAnalysisKit';

const delegator: abilityDelegatorRegistry.AbilityDelegator = abilityDelegatorRegistry.getAbilityDelegator()
const bundleName = abilityDelegatorRegistry.getArguments().bundleName;
let doneFunction: Function;

function sleep(time: number) {
    return new Promise<void>((resolve: Function) => setTimeout(resolve, time));
}

// Callbacks that monitor the target ability
function onAbilityCreateCallback(data: UIAbility) {
    hilog.info(0x0000, bundleName, `OnAbilityCreateCallback, data: ${JSON.stringify(data)}`);

}

function onAbilityDestroyCallback(data: UIAbility) {
    hilog.info(0x0000, bundleName, `OnAbilityCreateCallback, data: ${JSON.stringify(data)}`);
    doneFunction();
}

export default function abilityTest() {
    describe('AbilityTestSuite', () => {
        it('TestAbility', 0, async (done: Function) => {

            doneFunction = done;

            // Ability info
            const want: Want = {
                bundleName: bundleName,
                abilityName: 'EntryAbility',
                moduleName: 'entry',
            }

            // Add ability monitor to check ability states
            let monitor: abilityDelegatorRegistry.AbilityMonitor = {
                abilityName: 'EntryAbility',
                onAbilityCreate: onAbilityCreateCallback,
                onAbilityDestroy: onAbilityDestroyCallback
            };

            delegator.addAbilityMonitor(monitor, (error: BusinessError) => {
                hilog.error(0x0000, bundleName, `AddAbilityMonitor failed, error: ${JSON.stringify(error)}`);
            });

            // Launch ability
            await delegator.startAbility(want);

            // Wait for ability to launch
            /*await sleep(5000);

            // Check the top display ability
            await delegator.getCurrentTopAbility().then((Ability: UIAbility) => {
                expect(Ability.context.abilityInfo.name).assertEqual('EntryAbility');
            })*/
        })
    })
}