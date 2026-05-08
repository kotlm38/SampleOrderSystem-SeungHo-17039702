#pragma once

class MainMenuView {
public:
    void run();  // 메인 루프 진입점

private:
    void printMenu() const;
    void dispatch(int choice);
};
