#include "BtReadySignal.h"

#include <stdio.h>
#include "pico/multicore.h"
#include "pico/platform.h"

int BtReadySignal::doorbellNum = -1;

void BtReadySignal::claim() {
    // core_mask 0b11: both cores may claim/reference this doorbell number.
    doorbellNum = multicore_doorbell_claim_unused(0b11, /*required=*/true);
    printf("BtReadySignal claimed doorbell %d on %d\n", doorbellNum, get_core_num());
}

void BtReadySignal::signalReady() { multicore_doorbell_set_other_core(doorbellNum); }

void BtReadySignal::waitForReady() {
    printf("BtReadySignal waiting for ready on doorbell %d on %d\n", doorbellNum, get_core_num());
    while (!multicore_doorbell_is_set_current_core(doorbellNum)) {
        tight_loop_contents();
    }
    multicore_doorbell_clear_current_core(doorbellNum);
    printf("BtReadySignal received ready on doorbell %d on %d\n", doorbellNum, get_core_num());
}
