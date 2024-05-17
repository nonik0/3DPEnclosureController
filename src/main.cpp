#include "enclosure_controller/enclosure_controller.h"

static EnclosureController encCtl;

void view_create(EnclosureController *encCtl);
void view_update();

void controlTask(void *pvParameters)
{
    encCtl.init();
    view_create(&encCtl);
    for (;;) view_update();
}

void setup()
{
    TaskHandle_t handle = NULL;
    xTaskCreatePinnedToCore(controlTask, "Control Task", 20000, NULL, 1, &handle, 1);
    if (handle == NULL) {
        log_e("Failed to create SSH Task");
    }
}

void loop()
{
    delay(100);
}

