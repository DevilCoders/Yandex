#include "wait_guard.h"

TWaitGuard::TWaitGuard()
    : Counter(new NRTYArchive::TLinksCounter)
{}

TWaitGuard::~TWaitGuard() {
    Counter->WaitAllTasks();
}

void TWaitGuard::RegisterObject() {
    Counter->RegisterLink();
}

void TWaitGuard::UnRegisterObject() {
    Counter->UnRegisterLink();
}


void TWaitGuard::WaitAllTasks(ui64 maxTasksCount) const {
    Counter->WaitAllTasks(maxTasksCount);
}

bool TWaitGuard::Check() const {
    return Counter->Check();
}
