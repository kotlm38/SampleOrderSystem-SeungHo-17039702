#pragma once

class SampleView {
public:
    void show();  // 시료관리 서브메뉴 진입

private:
    void printMenu() const;
    void doRegister();
    void doList() const;
    void doSearch() const;
};
