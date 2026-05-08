#include "SampleView.h"
#include "../Controller/SampleController.h"
#include <iostream>
#include <iomanip>

void SampleView::show() {
    int choice = -1;
    while (choice != 0) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1: doRegister(); break;
            case 2: doList();     break;
            case 3: doSearch();   break;
            case 0:               break;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void SampleView::printMenu() const {
    std::cout << "\n--- 시료 관리 ---\n";
    std::cout << "1. 시료 등록\n";
    std::cout << "2. 시료 조회\n";
    std::cout << "3. 시료 검색\n";
    std::cout << "0. 뒤로\n";
    std::cout << "선택: ";
}

void SampleView::doRegister() {
    std::string id, name;
    int timeSec;
    float yield;

    std::cout << "시료 ID: ";    std::getline(std::cin, id);
    std::cout << "시료 이름: ";  std::getline(std::cin, name);
    std::cout << "평균 생산시간(초): "; std::cin >> timeSec;
    std::cout << "수율(0.0~1.0): ";    std::cin >> yield;
    std::cin.ignore();

    SampleController ctrl;
    if (ctrl.registerSample(id, name, timeSec, yield))
        std::cout << "등록 완료.\n";
    else
        std::cout << "이미 존재하는 ID입니다.\n";
}

void SampleView::doList() const {
    SampleController ctrl;
    auto samples = ctrl.getAllSamples();
    if (samples.empty()) { std::cout << "등록된 시료가 없습니다.\n"; return; }

    std::cout << "\n"
              << std::left << std::setw(12) << "ID"
              << std::setw(20) << "이름"
              << std::setw(12) << "생산시간(초)"
              << std::setw(8)  << "수율"
              << std::setw(8)  << "재고"
              << "\n";
    std::cout << std::string(60, '-') << "\n";
    for (const auto& s : samples) {
        std::cout << std::left << std::setw(12) << s.id
                  << std::setw(20) << s.name
                  << std::setw(12) << s.avgProductionTimeSec
                  << std::setw(8)  << s.yield
                  << std::setw(8)  << s.stock
                  << "\n";
    }
}

void SampleView::doSearch() const {
    std::cout << "검색 키워드: ";
    std::string keyword;
    std::getline(std::cin, keyword);

    SampleController ctrl;
    auto results = ctrl.searchByName(keyword);
    if (results.empty()) { std::cout << "결과 없음.\n"; return; }

    for (const auto& s : results) {
        std::cout << "[" << s.id << "] " << s.name
                  << "  생산시간: " << s.avgProductionTimeSec << "초"
                  << "  수율: " << s.yield
                  << "  재고: " << s.stock << "\n";
    }
}
