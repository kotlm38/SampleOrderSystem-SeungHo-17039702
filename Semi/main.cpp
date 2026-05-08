#include "Model/DataStore.h"
#include "Controller/ProductionController.h"
#include "View/MainMenuView.h"

int main() {
    DataStore::instance().load();
    ProductionController::instance().start();
    MainMenuView{}.run();
    ProductionController::instance().stop();
    return 0;
}
