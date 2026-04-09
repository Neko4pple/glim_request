# glim_request

MFC Dialog 기반 예제 프로젝트입니다.

## 구현 요약
- 도형 그리기 로직은 **CImage 버퍼에 대한 직접 픽셀 접근(GetBits / GetPitch)** 으로만 처리했습니다.
- 점 원은 **채워진 원**, 외접원은 **테두리만 있는 원**으로 직접 렌더링합니다.
- `Ellipse`, `FillPolygon`, `DrawPolygon`, GDI+ 기반 도형 API는 사용하지 않았습니다.
- 화면 출력은 렌더링이 끝난 래스터 이미지를 다이얼로그에 표시하는 방식입니다.
- 랜덤 이동은 `std::thread` 로 분리했고, UI 갱신은 `PostMessage` + `Invalidate(FALSE)` 로 처리했습니다.

## 포함 파일
- `glim_request.cpp` / `glim_request.h` : 앱 엔트리
- `glim_requestDlg.cpp` / `glim_requestDlg.h` : 핵심 로직
- `glim_request.rc` / `resource.h` : 다이얼로그 리소스
- `pch.*`, `framework.h`, `targetver.h` : MFC 기본 전처리 헤더
- `glim_request.vcxproj`, `glim_request.sln` : Visual Studio 프로젝트 파일

## 주요 기능
1. 최초 3회 마우스 클릭으로 점 생성
2. 3개 점 생성 직후 외접원 계산 및 표시
3. 점 드래그 시 외접원 실시간 재계산
4. 랜덤 이동 버튼 클릭 시 0.5초 간격, 총 10회 자동 이동
5. 초기화 버튼으로 전체 상태 복원

## 외접원 계산식
세 점을 `P1(x1,y1)`, `P2(x2,y2)`, `P3(x3,y3)` 라고 할 때,

- 분모:
  `D = 2 * (x1(y2-y3) + x2(y3-y1) + x3(y1-y2))`
- 중심 좌표:
  `Ux = ((x1^2+y1^2)(y2-y3) + (x2^2+y2^2)(y3-y1) + (x3^2+y3^2)(y1-y2)) / D`
  `Uy = ((x1^2+y1^2)(x3-x2) + (x2^2+y2^2)(x1-x3) + (x3^2+y3^2)(x2-x1)) / D`
- 반지름:
  `R = sqrt((Ux-x1)^2 + (Uy-y1)^2)`

`D` 가 0에 매우 가까우면 세 점이 일직선에 가깝다고 보고 외접원을 그리지 않습니다.

## 주의
- MFC 설치가 된 Visual Studio 환경에서 열어 빌드해야 합니다.
- 아이콘 리소스는 기본 플레이스홀더만 사용했습니다.
