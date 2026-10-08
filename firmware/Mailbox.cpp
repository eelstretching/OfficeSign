#include "Mailbox.h"

#include "pico/multicore.h"
#include "pico/time.h"

Mailbox::Mailbox() {
    mutex_init(&mutex);
    // core_mask 0b11: both cores may claim/reference this doorbell number.
    doorbellNum = multicore_doorbell_claim_unused(0b11, /*required=*/true);
}

void Mailbox::publish(const SignMessage& msg) {
    mutex_enter_blocking(&mutex);
    latest = msg;
    hasUnread = true;
    mutex_exit(&mutex);

    multicore_doorbell_set_other_core(doorbellNum);
}

bool Mailbox::take(SignMessage& out) {
    if (!multicore_doorbell_is_set_current_core(doorbellNum)) {
        return false;
    }
    multicore_doorbell_clear_current_core(doorbellNum);

    bool got = false;
    mutex_enter_blocking(&mutex);
    if (hasUnread) {
        out = latest;
        hasUnread = false;
        got = true;
    }
    mutex_exit(&mutex);
    return got;
}

void Mailbox::setConnected(bool isConnected) {
    connected = isConnected;
    if (!isConnected) {
        disconnectedAtUs = time_us_64();
    }
}

uint64_t Mailbox::microsSinceDisconnected() const { return time_us_64() - disconnectedAtUs; }
