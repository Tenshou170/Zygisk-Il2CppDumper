#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cinttypes>
#include "hack.h"
#include "zygisk.hpp"
#include "game.h"
#include "log.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class MyModule : public zygisk::ModuleBase {
public:
    MyModule() : api(nullptr), env(nullptr), enable_hack(false), game_data_dir(nullptr), data(nullptr), length(0) {}

    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->nice_name || !args->app_data_dir) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        if (package_name && app_data_dir) {
            preSpecialize(package_name, app_data_dir);
        } else {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
        if (package_name) env->ReleaseStringUTFChars(args->nice_name, package_name);
        if (app_data_dir) env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            std::thread hack_thread(hack_prepare, game_data_dir, data, length);
            hack_thread.detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack;
    char *game_data_dir;
    void *data;
    size_t length;

    static bool isTargetPackage(const char *package_name) {
        if (!package_name || strlen(package_name) == 0) return false;

        // 1. Check compile-time configured package name if set
        if (strcmp(GamePackageName, "com.game.packagename") != 0 && strcmp(package_name, GamePackageName) == 0) {
            return true;
        }

        // 2. Check dynamic runtime configuration files
        const char *config_paths[] = {
            "/data/local/tmp/il2cppdumper/package.txt",
            "/data/adb/modules/zygisk_il2cppdumper/package.txt"
        };
        for (const auto &cfg_path : config_paths) {
            FILE *fp = fopen(cfg_path, "r");
            if (fp) {
                char target[256] = {0};
                if (fgets(target, sizeof(target), fp)) {
                    size_t len = strlen(target);
                    while (len > 0 && (target[len - 1] == '\r' || target[len - 1] == '\n' || target[len - 1] == ' ' || target[len - 1] == '\t')) {
                        target[--len] = '\0';
                    }
                    if (strcmp(package_name, target) == 0) {
                        fclose(fp);
                        return true;
                    }
                }
                fclose(fp);
            }
        }

        return strcmp(package_name, GamePackageName) == 0;
    }

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        if (isTargetPackage(package_name)) {
            LOGI("detect game: %s", package_name);
            enable_hack = true;
            game_data_dir = new char[strlen(app_data_dir) + 1];
            strcpy(game_data_dir, app_data_dir);

#if defined(__i386__)
            auto path = "zygisk/armeabi-v7a.so";
#endif
#if defined(__x86_64__)
            auto path = "zygisk/arm64-v8a.so";
#endif
#if defined(__i386__) || defined(__x86_64__)
            int dirfd = api->getModuleDir();
            int fd = openat(dirfd, path, O_RDONLY);
            if (fd != -1) {
                struct stat sb{};
                fstat(fd, &sb);
                length = sb.st_size;
                data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
            } else {
                LOGW("Unable to open arm file");
            }
#endif
        } else {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }
};

REGISTER_ZYGISK_MODULE(MyModule)