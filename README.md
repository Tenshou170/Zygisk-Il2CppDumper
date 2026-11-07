# Zygisk-Il2CppDumper
Il2CppDumper with Zygisk, dump il2cpp data at runtime, can bypass protection, encryption and obfuscation.

中文说明请戳[这里](README.zh-CN.md)

## How to use
1. Install [Magisk](https://github.com/topjohnwu/Magisk) (v24+) or [KernelSU](https://github.com/tiann/KernelSU) / APatch with [ZygiskNext](https://github.com/Dr-TSNG/ZygiskNext)
2. Build module
   - GitHub Actions
      1. Go to the **Actions** tab in this repo
      2. In the left sidebar, click the **Build** workflow
      3. Click **Run workflow**, optionally input the game package name
      4. Download the flashable module zip from the workflow artifact
   - Android Studio
      1. Open the project
      2. (Optional) Edit `module/src/main/cpp/game.h` to set default package name
      3. Run `:module:assembleRelease`, flashable zip is generated in `out/`
3. Install module in Magisk / KernelSU Manager and reboot
4. Target package selection:
   - If compiled with a specific package name, it hooks automatically upon app launch.
   - Or dynamically specify any target without rebuilding:
     ```bash
     adb shell "su -c 'mkdir -p /data/local/tmp/il2cppdumper && echo com.example.game > /data/local/tmp/il2cppdumper/package.txt'"
     ```
5. Start the game, `dump.cs` will be generated in `/data/data/<package_name>/files/dump.cs`