#include <iostream>
#include <memory>

#include "args.hxx"

#include "Dispatcher.h"
#include "Signaling.h"

#include "rtc_base/logging.h"
#include "rtc_base/ssladapter.h"

using namespace std;

int main(int argc, char **argv) {
    rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
    rtc::LogMessage::LogTimestamps();
    rtc::LogMessage::LogThreads();
    rtc::InitializeSSL();
    //
    args::ArgumentParser parser("This is a test program.");
    args::Group group(parser, "Working mode:", args::Group::Validators::Xor);
    args::Flag sending(group, "sending", "Sending mode", {"sending"});
    args::Flag receiving(group, "receiving", "Receiving mode", {"receiving"});
    //
    try {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        std::cout << parser;
        return EXIT_SUCCESS;
    }
    catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return EXIT_FAILURE;
    }
    catch (args::ValidationError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return EXIT_FAILURE;
    }
    //
    //
    //
    shared_ptr<Signaling> signaling = make_shared<Signaling>();
    shared_ptr<PeerContext> peer = make_shared<PeerContext>(signaling);
    shared_ptr<Dispatcher> dispatcher = make_shared<Dispatcher>(signaling,peer);
     //
    dispatcher->run();
    //
    return EXIT_SUCCESS;
}