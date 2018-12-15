#include <iostream>
#include <memory>

#include "common/args.hxx"
#include "common/AWorker.h"
#include "receiving/ReceivingWorker.h"
#include "sending/SendingWorker.h"

int main(int argc, char **argv) {
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
    std::shared_ptr<AWorker> worker;
    if (sending) {
        // sending mode
        worker = std::make_shared<SendingWorker>();
    } else if (receiving) {
        // receiving mode
        worker = std::make_shared<ReceivingWorker>();
    }
    //
    worker->run();
    //
    return EXIT_SUCCESS;
}