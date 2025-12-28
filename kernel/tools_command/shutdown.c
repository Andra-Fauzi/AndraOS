#include "shutdown.h"

#include <uacpi/sleep.h>

// Shutdown sistem
void acpi_shutdown(void) {
    uacpi_status status = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
    if (status == UACPI_STATUS_OK) {
        uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
    }
}

void c_shutdown(char *buffer, int length) {
    acpi_shutdown();
}