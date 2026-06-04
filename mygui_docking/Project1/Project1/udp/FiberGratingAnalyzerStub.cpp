#include "FiberGratingAnalyzer.h"

#include <iostream>

class FiberGratingAnalyzer::Impl {
public:
    bool setPort(int) {
        std::cerr << "Fiber grating receiver is disabled in virtual demo build." << std::endl;
        return false;
    }

    bool startReceiving(const DataCallback&) {
        std::cerr << "Fiber grating receiver is disabled in virtual demo build." << std::endl;
        return false;
    }

    void stop() {}

    bool isReceiving() const {
        return false;
    }
};

FiberGratingAnalyzer::FiberGratingAnalyzer() : pimpl(new Impl()) {}

FiberGratingAnalyzer::~FiberGratingAnalyzer() {
    delete pimpl;
}

bool FiberGratingAnalyzer::setPort(int port) {
    return pimpl->setPort(port);
}

bool FiberGratingAnalyzer::startReceiving(const DataCallback& callback) {
    return pimpl->startReceiving(callback);
}

void FiberGratingAnalyzer::stopReceiving() {
    pimpl->stop();
}

bool FiberGratingAnalyzer::isReceiving() const {
    return pimpl->isReceiving();
}
