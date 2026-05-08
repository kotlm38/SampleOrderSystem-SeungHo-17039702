#pragma once

class MonitoringView {
public:
    void show();
private:
    void printMenu() const;
    void doOrderCount() const;
    void doInventory() const;
};
