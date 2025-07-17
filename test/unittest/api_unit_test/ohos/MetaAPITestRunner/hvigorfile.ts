import { appTasks, OhosPluginId, OhosAppContext } from '@ohos/hvigor-ohos-plugin';
import { hvigor, getNode, HvigorNode, FileUtil } from '@ohos/hvigor';
import * as dotenv from 'dotenv';
import * as dotenvExpand from 'dotenv-expand';
import * as fs from 'fs';
import * as path from 'path';

enum LogLevel {
    ERROR,
    WARN,
    INFO,
    DEBUG
};

// Function to print a custom log message
function logMessage(logLevel: LogLevel, message: string): void {

    //const debugMode = hvigor.getParameter().getExtParam('debuggable');

    const preText = '> hvigor ';
    let level = '';
    let start = '\x1b[0m';
    const reset = '\x1b[0m';

    // Color codes
    if (logLevel == LogLevel.INFO) {
        start = '\x1b[36m';
        level = 'INFO: ';
    } else if (logLevel == LogLevel.ERROR) {
        start = '\x1b[31m';
        level = 'ERROR: ';
    } else if (logLevel == LogLevel.WARN) {
        start = '\x1b[33m';
        level = 'WARN: ';
    }

    // Print the message
    let output = `${preText}${start}${level}${message}${reset}`;
    console.log(output);
}

// Function to get the path of any .env file in the current directory
function getEnvFilePath(): string {
    // Get the current working directory (root of the project)
    const cwd = process.cwd();

    // Read the directory contents (files and subdirectories)
    const files = fs.readdirSync(cwd);

    // Find the first file that ends with .env
    const envFile = files.find(file => file.toLowerCase().endsWith('.env'));

    // Return .env file full
    if (envFile) {
        const envFilePath = path.resolve(cwd, envFile);
        logMessage(LogLevel.INFO, `Found .env file in ${envFilePath}`);
        return envFilePath;
    } else {
        // If no .env file is found
        //logMessage(LogLevel.WARN, `Couldn't find any .env file in ${path.resolve(cwd)}.`);
        return '';
    }
}

// Function to check if all required environment variables are set
function checkRequiredEnvVariables(): void {
    const missingVariables = envVariables.filter((varName) => !process.env[varName]);

    if (missingVariables.length > 0) {
        // Throw an error with a detailed message for missing variables
        throw new Error(
            `The following environment variables are missing: ${missingVariables.join(', ')}`
        );
    } else {
        logMessage(LogLevel.INFO, 'All required environment variables are set.');
    }
}

// List of required environment variables
const envVariables = [
    'CERT_PATH',
    'STORE_PASSWORD',
    'KEY_ALIAS',
    'KEY_PASSWORD',
    'PROFILE',
    'SIGN_ALG',
    'STORE_FILE'
];

function verifySignature() {

    // Load environment variables from .env file
    const envFilePath = getEnvFilePath();

    if (envFilePath) {

        logMessage(LogLevel.INFO, 'Using .env info for the .hap signature.');

        // Load .env variables (with expansion) in case .env file has system variables
        dotenvExpand.expand(dotenv.config({ path: envFilePath, debug: true, encoding: 'UTF-8' }));

        try {
            checkRequiredEnvVariables();

            // Get root/project node context
            const appContext = hvigor.getRootNode().getContext(OhosPluginId.OHOS_APP_PLUGIN) as OhosAppContext;

            // Obtains the object from the build-profile.json5 file based on the context/module
            const buildProfileOpt = appContext.getBuildProfileOpt();

            // Obtains the value of 'product' that is configured by -p in the command
            let productConfig = hvigor.getParameter().getExtParam('product');
            if(productConfig == undefined) {
                productConfig = 'default'; // Use this by default if -p doesn't specify it
            }

            // Find the signature config and runtimeOS for the target product
            const products = buildProfileOpt['app']['products'];
            const targetProduct = products.find(item => item.name === productConfig);
            const sgnConfig = targetProduct['signingConfig'];
            const osConfig = targetProduct['runtimeOS'];

            // Get and then set the signature field
            let sgn = buildProfileOpt['app']['signingConfigs'];

            if (sgn !== undefined && sgn.length > 0) {
                logMessage(LogLevel.ERROR,
                    'The file build-profile.json5 contains signing configurations. Please remove the content in signingConfigs field to prevent accidental credential leakage.')
                process.exit(-1);
            }

            const sgnData = [{
                  name: sgnConfig,
                  type: osConfig,
                  material: {
                      certpath: path.normalize(process.env.CERT_PATH),
                      storePassword: process.env.STORE_PASSWORD,
                      keyAlias: process.env.KEY_ALIAS,
                      keyPassword: process.env.KEY_PASSWORD,
                      profile: path.normalize(process.env.PROFILE),
                      signAlg: process.env.SIGN_ALG,
                      storeFile: path.normalize(process.env.STORE_FILE)
                  }
              }];

            // Update signature
            buildProfileOpt['app']['signingConfigs'] = sgnData;

            appContext.setBuildProfileOpt(buildProfileOpt);

        } catch (error) {
            logMessage(LogLevel.ERROR, `${error.message}`);
            process.exit(-1);
        }
    } else {
        logMessage(LogLevel.INFO, 'Using hardcoded signature from build-profile.json5 file.');
    }
}

/*
 * PLUGIN: VerifySignature
 *
 * */
export function VerifySignature(): HvigorPlugin {
    return {
        pluginId: 'VerifySignature',
        //  Function called during the configuration phase of the hvigor lifecycle
        async apply(node: HvigorNode): Promise<void> {
            verifySignTask(node);
        }
    }
}

function verifySignTask(currentNode: HvigorNode) {

    // Callback in the configuration phase to perform deferred tasks, such as obtaining node plug-in context or
    // registering a task
    hvigor.nodesEvaluated(async () => {
        currentNode.subNodes((node: HvigorNode) => {
            // Obtain the context information of the HAP module.
            const hapContext = node.getContext(OhosPluginId.OHOS_HAP_PLUGIN) as OhosHapContext;
            const moduleName = hapContext.getModuleName();

            hapContext.targets((target: Target) => {
                const targetName = target.getTargetName();
                const outputPath = target.getBuildTargetOutputPath();

                node.registerTask({
                    // Task name.
                    name: `${targetName}@VerifySignHap`,
                    // Function that defines the task logic.
                    run() {
                        verifySignature();
                    },
                    // Configure the dependencies for this task.
                    dependencies: [],
                    // Configure the dependencies for the next task (which is the entry task hvigor.getCommandEntryTask())
                    postDependencies: ['assembleHap']
                });
            });
        });
    });
}

export default {
    system: appTasks,  /* Built-in plugin of Hvigor. It cannot be modified. */
    plugins:[          /* Custom plugin to extend the functionality of Hvigor. */
      VerifySignature()
    ]
}
