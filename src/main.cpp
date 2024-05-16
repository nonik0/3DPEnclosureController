#include "enclosure_controller/enclosure_controller.h"

static EnclosureController encCtl;

void view_create(EnclosureController* encCtl);
void view_update();

void setup()
{
    encCtl.init();
    view_create(&encCtl);
}

void loop() { view_update(); }
