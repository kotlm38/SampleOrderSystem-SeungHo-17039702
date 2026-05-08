# Phase 1 — 환경 셋업

## 목표
빌드 가능한 빈 Semi 프로젝트 뼈대를 만든다.  
소스 파일이 하나도 없는 현재 상태에서 컴파일 환경을 갖춘다.

---

## 작업 목록

### 1-1. nlohmann/json 헤더 추가

**파일:** `Semi/Lib/json.hpp`

- nlohmann/json v3.11.x single-header 파일을 `Semi/Lib/json.hpp`로 저장
- 라이브러리 설치나 빌드 불필요, 헤더 include만으로 사용
- 사용 방법:
  ```cpp
  #include "Lib/json.hpp"
  using json = nlohmann::json;
  ```

### 1-2. Semi.vcxproj — 소스 파일 등록

`Semi.vcxproj`의 `<ItemGroup>` 안에 아래 파일들을 추가한다.

**헤더 파일 (`<ClInclude>`):**
```xml
<ClInclude Include="Lib\json.hpp" />
<ClInclude Include="Model\Sample.h" />
<ClInclude Include="Model\Order.h" />
<ClInclude Include="Model\ProductionJob.h" />
<ClInclude Include="Model\DataStore.h" />
<ClInclude Include="Controller\SampleController.h" />
<ClInclude Include="Controller\OrderController.h" />
<ClInclude Include="Controller\ProductionController.h" />
<ClInclude Include="Controller\MonitoringController.h" />
<ClInclude Include="Controller\ShippingController.h" />
<ClInclude Include="View\MainMenuView.h" />
<ClInclude Include="View\SampleView.h" />
<ClInclude Include="View\OrderView.h" />
<ClInclude Include="View\MonitoringView.h" />
<ClInclude Include="View\ShippingView.h" />
<ClInclude Include="View\ProductionLineView.h" />
```

**소스 파일 (`<ClCompile>`):**
```xml
<ClCompile Include="main.cpp" />
<ClCompile Include="Model\Order.cpp" />
<ClCompile Include="Model\DataStore.cpp" />
<ClCompile Include="Controller\SampleController.cpp" />
<ClCompile Include="Controller\OrderController.cpp" />
<ClCompile Include="Controller\ProductionController.cpp" />
<ClCompile Include="Controller\MonitoringController.cpp" />
<ClCompile Include="Controller\ShippingController.cpp" />
<ClCompile Include="View\MainMenuView.cpp" />
<ClCompile Include="View\SampleView.cpp" />
<ClCompile Include="View\OrderView.cpp" />
<ClCompile Include="View\MonitoringView.cpp" />
<ClCompile Include="View\ShippingView.cpp" />
<ClCompile Include="View\ProductionLineView.cpp" />
```

### 1-3. Semi.vcxproj — AdditionalIncludeDirectories 설정

각 `<ClCompile>` ItemDefinitionGroup에 추가 include 경로를 설정한다.  
`Semi/Lib`를 include 경로에 추가하여 `#include "Lib/json.hpp"` 대신  
`#include "json.hpp"`로도 사용 가능하게 한다. (DataStore.cpp에서만 사용하므로 직접 경로 include로도 충분)

### 1-4. DB 폴더 및 빈 data.json 준비

- 솔루션 루트에 `DB/` 폴더가 이미 존재하는지 확인
- DataStore::save() 호출 시 `DB/` 폴더가 없으면 자동 생성하도록 구현할 것 (Phase 2에서 처리)
- 초기 `DB/data.json`은 파일이 없으면 DataStore::load()가 빈 상태로 초기화

### 1-5. 폴더 구조 생성

아래 폴더를 `Semi/` 아래에 생성:
```
Semi/
├── Lib/
├── Model/
├── Controller/
└── View/
```

---

## 완료 기준

- Visual Studio에서 Semi 프로젝트를 열었을 때 모든 소스 파일이 솔루션 탐색기에 표시됨
- `Lib/json.hpp`가 존재하고 `#include "Lib/json.hpp"` 컴파일 성공
- 빈 `main.cpp`만으로 빌드 오류 없이 컴파일 가능 (링크 경고는 허용)

---

## 주의사항

- `Semi.vcxproj`는 현재 `<ItemGroup></ItemGroup>` (빈 태그)이므로 편집 시 이 태그를 교체
- C++20 표준(`<LanguageStandard>stdcpp20</LanguageStandard>`)은 이미 설정되어 있으므로 변경 불필요
- nlohmann/json은 C++11 이상 지원, C++20에서 완전 호환
