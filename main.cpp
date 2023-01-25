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

class MyShellCallback : public BnShellCallback
{
public:
    TextOutput& mErrorLog;
    MyShellCallback(TextOutput& errorLog) : mErrorLog(errorLog) {}
    virtual int openFile(const String16& path, const String16& seLinuxContext,
            const String16& mode) {
	return 0;
    }
};


class MyResultReceiver : public BnResultReceiver
{
public:
    virtual void send(int32_t resultCode) {
    }
    int32_t waitForResult() {
	return 0;
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
    
    sp<IServiceManager> sm = nullptr;
    sm = defaultServiceManager();
    assert(sm != nullptr);

    const auto cmd = argv[0];

    String16 serviceName = String16(cmd.data(), cmd.size());
    Vector<String16> args;
    args.add(String16(argv[1].data(), argv[1].size()));
    args.add(String16(argv[2].data(), argv[2].size()));
    args.add(String16(argv[3].data(), argv[3].size()));
    args.add(String16(argv[4].data(), argv[4].size()));
    args.add(String16(argv[5].data(), argv[5].size()));

    sp<IBinder> service = nullptr;
    service = sm->checkService(serviceName);
    assert(service != nullptr);

    sp<MyShellCallback> cb = new MyShellCallback(aerr);
    sp<MyResultReceiver> result = new MyResultReceiver();

    Parcel send;
    Parcel reply;
    send.writeFileDescriptor(STDIN_FILENO);
    send.writeFileDescriptor(STDOUT_FILENO);
    send.writeFileDescriptor(STDERR_FILENO);
    const size_t numArgs = args.size();
    send.writeInt32(numArgs);
    for (size_t i = 0; i < numArgs; i++) {
	send.writeString16(args[i]);
    }

    send.writeStrongBinder(cb != nullptr ? IInterface::asBinder(cb) : nullptr);
    send.writeStrongBinder(result != nullptr ? IInterface::asBinder(result) : nullptr);
    status_t error = service->transact(MyResultReceiver::SHELL_COMMAND_TRANSACTION, send, &reply);

    return 0;
}
