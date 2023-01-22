#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IResultReceiver.h>
#include <binder/IServiceManager.h>
#include <binder/IShellCallback.h>
#include <binder/TextOutput.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <memory>
#include <vector>
#include <selinux/selinux.h>
#include <selinux/android.h>
#include <string_view>

using namespace android;

static int sort_func(const String16* lhs, const String16* rhs)
{
    return lhs->compare(*rhs);
}

struct SecurityContext_Delete {
    void operator()(char* p) const {
        freecon(p);
    }
};

typedef std::unique_ptr<char[], SecurityContext_Delete> Unique_SecurityContext;

class MyShellCallback : public BnShellCallback
{
public:
    TextOutput& mErrorLog;
    bool mActive = true;
    MyShellCallback(TextOutput& errorLog) : mErrorLog(errorLog) {}
    virtual int openFile(const String16& path, const String16& seLinuxContext,
            const String16& mode) {
        String8 path8(path);
        char cwd[256];
        getcwd(cwd, 256);
        String8 fullPath(cwd);
        fullPath.appendPath(path8);
        if (!mActive) {
            mErrorLog << "Open attempt after active for: " << fullPath << endl;
            return -EPERM;
        }
#if DEBUG
        ALOGD("openFile: %s, full=%s", path8.string(), fullPath.string());
#endif
        int flags = 0;
        bool checkRead = false;
        bool checkWrite = false;
        if (mode == u"w") {
            flags = O_WRONLY|O_CREAT|O_TRUNC;
            checkWrite = true;
        } else if (mode == u"w+") {
            flags = O_RDWR|O_CREAT|O_TRUNC;
            checkRead = checkWrite = true;
        } else if (mode == u"r") {
            flags = O_RDONLY;
            checkRead = true;
        } else if (mode == u"r+") {
            flags = O_RDWR;
            checkRead = checkWrite = true;
        } else {
            mErrorLog << "Invalid mode requested: " << mode.string() << endl;
            return -EINVAL;
        }
        int fd = open(fullPath.string(), flags, S_IRWXU|S_IRWXG);
#if DEBUG
        ALOGD("openFile: fd=%d", fd);
#endif
        if (fd < 0) {
            return fd;
        }
        if (is_selinux_enabled() && seLinuxContext.size() > 0) {
            String8 seLinuxContext8(seLinuxContext);
            char* tmp = nullptr;
            getfilecon(fullPath.string(), &tmp);
            Unique_SecurityContext context(tmp);
            if (checkWrite) {
                int accessGranted = selinux_check_access(seLinuxContext8.string(), context.get(),
                        "file", "write", nullptr);
                if (accessGranted != 0) {
#if DEBUG
                    ALOGD("openFile: failed selinux write check!");
#endif
                    close(fd);
                    mErrorLog << "System server has no access to write file context " << context.get() << " (from path " << fullPath.string() << ", context " << seLinuxContext8.string() << ")" << endl;
                    return -EPERM;
                }
            }
            if (checkRead) {
                int accessGranted = selinux_check_access(seLinuxContext8.string(), context.get(),
                        "file", "read", nullptr);
                if (accessGranted != 0) {
#if DEBUG
                    ALOGD("openFile: failed selinux read check!");
#endif
                    close(fd);
                    mErrorLog << "System server has no access to read file context " << context.get() << " (from path " << fullPath.string() << ", context " << seLinuxContext8.string() << ")" << endl;
                    return -EPERM;
                }
            }
        }
        return fd;
    }
};


class MyResultReceiver : public BnResultReceiver
{
public:
    Mutex mMutex;
    Condition mCondition;
    bool mHaveResult = false;
    int32_t mResult = 0;
    virtual void send(int32_t resultCode) {
        AutoMutex _l(mMutex);
        mResult = resultCode;
        mHaveResult = true;
        mCondition.signal();
    }
    int32_t waitForResult() {
        AutoMutex _l(mMutex);
        while (!mHaveResult) {
            mCondition.wait(mMutex);
        }
        return mResult;
    }
};


int main() {

    std::vector<std::string_view> argv;
    argv.emplace_back("settings");
    argv.emplace_back("put");
    argv.emplace_back("secure");
    argv.emplace_back("location_mode");
    argv.emplace_back("3");

    int in = STDIN_FILENO;
    int out = STDOUT_FILENO;
    int err = STDERR_FILENO;

    sp<IServiceManager> sm = defaultServiceManager();
    if(sm == nullptr) {
	printf("sm is nullptr\n");
	return 0;
    }
    else
	printf("sm is not nullptr\n");

    const auto cmd = argv[0];

    String16 serviceName = String16(cmd.data(), cmd.size());
    Vector<String16> args;
    args.add(String16(argv[1].data(), argv[1].size()));
    args.add(String16(argv[2].data(), argv[2].size()));
    args.add(String16(argv[3].data(), argv[3].size()));
    args.add(String16(argv[4].data(), argv[4].size()));
    args.add(String16(argv[5].data(), argv[5].size()));

    sp<IBinder> service;
    service = sm->checkService(serviceName);
    if(service == nullptr) {
	printf("can't find service\n");
	return 0;
    }
    else
	printf("service is not nullptr\n");

    sp<MyShellCallback> cb = new MyShellCallback(aerr);
    sp<MyResultReceiver> result = new MyResultReceiver();

    status_t error = IBinder::shellCommand(service, in, out, err, args, cb, result);
    if(error < 0)
	printf("error: %d\n", error);
    else
	printf("no error!\n");

    return 0;
}
