#pragma once

class MainMenuView {
public:
    void run();
private:
    void printMenu() const;
    void dispatch(int choice);
};
