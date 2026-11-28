<div align="center">

# 🔍 ChuChuSearch · 쮸쮸서치

**물체로 변신해 숨고, 마법으로 찾아내는 3인칭 멀티플레이 숨바꼭질**

![Unreal Engine 5.4](https://img.shields.io/badge/Unreal_Engine-5.4-0E1128?logo=unrealengine) ![C++20](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus) ![Blueprint](https://img.shields.io/badge/Blueprint-Visual_Scripting-137CBD?logo=unrealengine) ![Multiplayer](https://img.shields.io/badge/Multiplayer-Listen_%7C_Dedicated-8A2BE2) ![AWS EC2](https://img.shields.io/badge/AWS-EC2-FF9900?logo=amazonwebservices) ![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)

![Team](https://img.shields.io/badge/Team-3%EC%9D%B8_%28%ED%94%84%EB%A1%9C%EA%B7%B8%EB%9E%98%EB%B0%8D_1%EC%9D%B8%29-FF7F7F) ![Status](https://img.shields.io/badge/Status-%EA%B0%9C%EB%B0%9C_%EC%A4%91-success) [![YouTube](https://img.shields.io/badge/YouTube-%EC%8B%9C%EC%97%B0_%EC%98%81%EC%83%81-FF0000?logo=youtube)](https://youtu.be/NzZujXlLXhQ)

[![ChuChuSearch Gameplay](https://img.youtube.com/vi/NzZujXlLXhQ/maxresdefault.jpg)](https://youtu.be/NzZujXlLXhQ)

</div>

---

## 📑 목차

[![01 프로젝트 개요](https://img.shields.io/badge/01-%ED%94%84%EB%A1%9C%EC%A0%9D%ED%8A%B8_%EA%B0%9C%EC%9A%94-FF7F7F)](#-프로젝트-개요) [![02 게임 한눈에 보기](https://img.shields.io/badge/02-%EA%B2%8C%EC%9E%84_%ED%95%9C%EB%88%88%EC%97%90_%EB%B3%B4%EA%B8%B0-EC982A)](#-게임-한눈에-보기) [![03 주요 기능](https://img.shields.io/badge/03-%EC%A3%BC%EC%9A%94_%EA%B8%B0%EB%8A%A5-F2C94C)](#-주요-기능) [![04 네트워크 & 서버](https://img.shields.io/badge/04-%EB%84%A4%ED%8A%B8%EC%9B%8C%ED%81%AC_%26_%EC%84%9C%EB%B2%84-8A2BE2)](#-네트워크--서버) [![05 아키텍처](https://img.shields.io/badge/05-%EC%95%84%ED%82%A4%ED%85%8D%EC%B2%98-137CBD)](#-아키텍처) [![06 조작법](https://img.shields.io/badge/06-%EC%A1%B0%EC%9E%91%EB%B2%95-38A169)](#-조작법) [![07 시작하기](https://img.shields.io/badge/07-%EC%8B%9C%EC%9E%91%ED%95%98%EA%B8%B0-0078D6)](#-시작하기) [![08 Tech Stack](https://img.shields.io/badge/08-Tech_Stack-0E1128)](#-tech-stack) [![09 배운 점](https://img.shields.io/badge/09-%EB%B0%B0%EC%9A%B4_%EC%A0%90-5C2D91)](#-배운-점)

---

## 📌 프로젝트 개요

![Genre Hide & Seek](https://img.shields.io/badge/Genre-Hide_%26_Seek-FF7F7F) ![Players Multiplayer](https://img.shields.io/badge/Players-Multiplayer-8A2BE2) ![Period 2026.04 ~ 2026.12](https://img.shields.io/badge/Period-2026.04_~_2026.12-555555) ![Role Programming (Solo)](https://img.shields.io/badge/Role-Programming_%28Solo%29-00599C?logo=cplusplus)

| 항목 | 내용 |
| --- | --- |
| 🎮 장르 | 3인칭 멀티플레이 숨바꼭질 (술래 vs 러너) |
| 📅 개발 기간 | 2026.04 ~ 2026.12 |
| 👥 개발 인원 | 3인 (**프로그래밍 1인 전담**) |
| 🛠 엔진 / 언어 | Unreal Engine 5.4 · C++ · Blueprint |
| 🌐 네트워크 | 리슨 서버 / 데디케이티드 서버 (AWS EC2) |
| 🗂 형상 관리 | Git · GitHub |

> 3인 팀에서 **유일한 프로그래머**로 게임플레이, 네트워크, UI, 서버 배포까지 프로그래밍 전반을 담당했습니다.

---

## 🎲 게임 한눈에 보기

![술래 Seeker](https://img.shields.io/badge/%EC%88%A0%EB%9E%98-Seeker-D64046) ![러너 Runner](https://img.shields.io/badge/%EB%9F%AC%EB%84%88-Runner-389660) ![유령 Ghost](https://img.shields.io/badge/%EC%9C%A0%EB%A0%B9-Ghost-925CE8) ![Phase 5단계](https://img.shields.io/badge/Phase-5%EB%8B%A8%EA%B3%84-EC982A)

```mermaid
flowchart LR
    A[🏠 처음 화면<br/>닉네임 · 서버 주소] -->|접속하기 / 방 만들기| B[🌾 로비<br/>돌아다니며 연습 · 준비]
    B -->|방장이 출발 발판에서 E| C[🎯 게임<br/>숨기 → 찾기 → 탈출]
    C -->|결과 화면| B
    B -->|게임 나가기| A
```

| 역할 | 목표 | 할 수 있는 것 |
| --- | --- | --- |
| 🪄 **술래** | 러너를 모두 찾아 탈락시키기 | 마법 발사, 의심 가는 물체 꾹 눌러 부수기 |
| 🫥 **러너** | 단서 3개 모으고 비밀번호 문을 열어 탈출하기 | 주변 물체로 변신해 숨기, 제자리 고정·회전 |
| 👻 **유령** | 탈락해도 팀 돕기 | 날아다니며 가짜 물체 배치해 술래 속이기 |

**게임 진행 단계**

```text
숨는 시간 ──▶ 1페이즈 ──▶ 강제 교체 ──▶ 2페이즈 ──▶ 피버 타임 ──▶ 결과
 (30초)       (4분)        (30초)        (4분)        (1분)
              술래 추격     같은 물체에    다시 추격    숨은 위치에서
                           계속 숨은 러너               주기적으로 소리
                           는 물체 교체
```

> 시간이 다 지나면 러너 승리, 러너가 모두 탈락하면 술래 승리. 단계 시간과 최대 인원은 방장이 로비에서 바꿀 수 있습니다.

---

## ✨ 주요 기능

![Lobby 3D 로비](https://img.shields.io/badge/Lobby-3D_%EB%A1%9C%EB%B9%84-EC982A?logo=unrealengine) ![Possession 물체 변신](https://img.shields.io/badge/Possession-%EB%AC%BC%EC%B2%B4_%EB%B3%80%EC%8B%A0-389660) ![Projectile 술래 공격](https://img.shields.io/badge/Projectile-%EC%88%A0%EB%9E%98_%EA%B3%B5%EA%B2%A9-D64046) ![Ghost 물체 배치](https://img.shields.io/badge/Ghost-%EB%AC%BC%EC%B2%B4_%EB%B0%B0%EC%B9%98-925CE8) ![UI C++ UMG](https://img.shields.io/badge/UI-C%2B%2B_UMG-5C2D91?logo=cplusplus)

<details open>
<summary><b>🌾 걸어 다니는 3D 로비</b></summary>

<br>

메뉴 화면 대신 목장 맵을 캐릭터로 돌아다니며 게임을 준비하는 로비입니다.

| 장치 | 동작 |
| --- | --- |
| 🟠 **출발 발판** | 위에 서면 준비 완료. 방장이 발판에서 **E** → 3초 카운트다운 후 출발 |
| 🔴🟢 **역할 문** | **E**로 술래·러너 지망. 지망한 역할로 로비에서 바로 연습 |
| 🟣 **게임 설정 게시판** | 단계 시간·최대 인원 설정 (방장만 변경, 나머지는 보기) |
| ⚫ **유령 구멍** | 유령이 되어 물체 배치를 미리 체험하고 다시 사람으로 복귀 |

- **연습 규칙**: 맞아도 탈락 없이 피격 효과만, 부서진 물체는 5초 뒤 같은 자리에 다시 생성
- **머리 위 이름표**: 역할 지망에 따라 색이 바뀌고 방장·준비 상태 표시
- **처음 안내**: 장치에 처음 다가가거나 역할이 바뀔 때 한 번만 뜨는 안내
- **Tab** 참가자 목록, **Enter** 채팅

</details>

<details>
<summary><b>🫥 물체 변신(빙의) 시스템</b></summary>

<br>

- 바라보는 물체에 테두리 표시(커스텀 뎁스 스텐실 + 후처리 아웃라인)
- 변신하면 캐릭터 충돌을 물체 크기에 맞추고, 같은 팀이 숨은 물체는 부술 수 없음
- **고정(T)** 후 **Q/E**로 회전, **R**로 똑바로 세우기
- 1페이즈 내내 같은 물체에 숨어 있던 러너는 강제 교체 단계에서 다른 물체로 옮겨야 함

</details>

<details>
<summary><b>🪄 술래 공격 · 물체 파괴</b></summary>

<br>

```text
좌클릭 ──▶ 투사체 발사 ──▶ 충돌 판정 ─┬─▶ 러너 적중 ──▶ 하트 감소 / 탈락
                                     └─▶ 물체 적중 ──▶ 유령이 놓은 가짜면 유령 점수
좌클릭 꾹(3초) ──▶ 서버 거리·시야 검증 ──▶ 물체 파괴 ──▶ 숨은 러너면 적중 / 단서 등장 / 유령 배치 목록 등록
```

- 서버에서 사거리와 시야를 다시 검사해 벽 너머 파괴 차단
- 적중 시 카메라 흔들림, 피격 화면 효과, 3D 사운드

</details>

<details>
<summary><b>👻 유령 · 🔑 단서 · 🚪 탈출</b></summary>

<br>

- **유령**: 탈락한 러너는 유령이 되어 부서진 물체를 골라 다시 놓아 술래를 속임 (물체 썸네일 버튼)
- **단서**: 특정 물체를 부수면 단서가 등장, 러너가 모아 비밀번호를 알아냄
- **비밀번호 문**: 단서 3개를 모은 러너가 비밀번호를 풀고 특별 출구로 탈출하면 러너 승리

</details>

<details>
<summary><b>🏆 점수 · 관전 · 시간 끌기 방지</b></summary>

<br>

- **점수**: 적중·탈락·단서·비밀번호·코앞에서 숨기·아슬아슬하게 피하기·가짜 물체 적중·승리
- 게임 중에는 상대 팀 점수를 숨기고 결과 화면에서 공개
- **관전(V)**: 탈락 후 다른 플레이어 시점 관전
- **피버 타임**: 숨은 러너 위치에서 주기적으로 소리를 내 멀리서 버티기 방지

</details>

<details>
<summary><b>🎨 C++로 그린 UI</b></summary>

<br>

처음 화면, 로비 HUD, 채팅·참가자 목록, 설정 창, 점수판, 조작법 안내, 결과 화면 등
대부분의 UI를 **위젯 블루프린트 없이 C++(UMG)로 직접 구성**했습니다.

- 같은 톤의 크림색 패널·리본 제목·둥근 버튼을 코드로 통일
- 마우스 감도·상하 반전, 그래픽, 음량(마스터·배경음·효과음) 설정과 탭별 초기화
- 맵이 바뀌어도 끊기지 않는 배경음(`GameInstanceSubsystem`)

</details>

---

## 🌐 네트워크 & 서버

![Listen Server](https://img.shields.io/badge/Listen-Server-8A2BE2) ![Dedicated Server](https://img.shields.io/badge/Dedicated-Server-8A2BE2) ![AWS EC2](https://img.shields.io/badge/AWS-EC2-FF9900?logo=amazonwebservices) ![Travel Seamless](https://img.shields.io/badge/Travel-Seamless-137CBD) ![Reconnect 지원](https://img.shields.io/badge/Reconnect-%EC%A7%80%EC%9B%90-389660)

```mermaid
flowchart TB
    subgraph Dedicated["☁️ AWS EC2 · 데디케이티드 서버"]
        S[HideServer.exe<br/>WaitRoomMap 시작]
    end
    subgraph Listen["🏠 리슨 서버"]
        H[방장 PC<br/>방 만들기]
    end
    C1[클라이언트] -->|UDP 7777| S
    C2[클라이언트] -->|UDP 7777| S
    C3[친구] -->|방장 IP| H
```

- **두 가지 방식을 한 클라이언트로**: 전용 서버 주소로 접속하거나, 내 PC로 방을 열 수 있음
- **방장 자동 지정**: 먼저 들어온 사람이 방장, 방장이 나가면 다음 사람에게 넘어감
- **맵 이동**: 로비 → 게임은 Seamless Travel, 게임 → 로비는 재접속 방식으로 상태를 깨끗하게 초기화
- **재접속 복원**: 게임 중 끊긴 플레이어가 돌아오면 역할·단서·캐릭터 복원 (팀 전원 이탈 시 60초 유예)
- **서버 권한 검증**: 빙의·파괴·공격·닉네임·채팅을 서버에서 다시 검사 (도배·사칭 방지)
- **접속 안내**: 방이 가득 참, 게임 진행 중, 버전 불일치, 연결 끊김을 구분해 팝업 안내, 접속 취소 지원

---

## 🧱 아키텍처

![Game Framework UE5](https://img.shields.io/badge/Game_Framework-UE5-0E1128?logo=unrealengine) ![C++ Classes 40](https://img.shields.io/badge/C%2B%2B_Classes-40-00599C?logo=cplusplus) ![Replication RPC](https://img.shields.io/badge/Replication-RPC-8A2BE2)

```mermaid
flowchart LR
    subgraph Server["🖥 서버"]
        WGM["WaitRoomGameMode<br/>로비 규칙 · 방장 · 출발"]
        GM["MyGameMode<br/>게임 단계 · 승패 · 재접속"]
        GS["MyGameState<br/>타임라인 · 유령 목록 · 단서"]
    end
    subgraph Player["🙋 플레이어마다"]
        PS["MyPlayerState<br/>닉네임 · 역할 · 준비 · 점수"]
        PC["MyPlayerController<br/>입력 · UI · 로비 상호작용"]
        CH["MyCharacter<br/>빙의 · 공격 · 파괴"]
        GH["GhostCharacter<br/>비행 · 물체 배치"]
    end
    PROP["PropBase<br/>변신 대상 물체"]
    DEV["LobbyDevice<br/>발판 · 문 · 게시판 · 구멍"]

    WGM --> GS
    GM --> GS
    GM --> PS
    PC --> CH
    PC --> GH
    CH --> PROP
    PC -.E.-> DEV
```

<details>
<summary><b>📂 주요 C++ 클래스</b></summary>

<br>

| 분류 | 클래스 | 역할 |
| --- | --- | --- |
| **Game Flow** | `AWaitRoomGameMode` | 로비 규칙, 방장 지정, 출발 조건, 연습 규칙 |
| | `AMyGameMode` | 게임 단계 진행, 승패, 재접속, 점수 |
| | `AMyGameState` | 단계 타임라인, 유령 배치 목록, 단서 정보 |
| | `AMyPlayerState` | 닉네임, 역할 지망, 준비, 점수 |
| **Gameplay** | `AMyPlayerController` | 입력, UI, 로비 장치 상호작용, 배경음 |
| | `AMyCharacter` | 빙의, 술래 공격, 물체 파괴, 피격 |
| | `AGhostCharacter` | 유령 비행, 물체 배치 미리보기 |
| | `APropBase` | 변신 대상 물체 공통 기능 |
| | `ASeekerProjectile` | 술래 투사체와 충돌 처리 |
| | `APasswordDoor` / `ACluePickup` | 비밀번호 문, 단서 |
| **Lobby** | `ALobbyDevice` 외 4종 | 출발 발판, 역할 문, 설정 게시판, 유령 구멍, 구역 표지판 |
| **UI** | `UTitleWidget`, `ULobbyHudWidget`, `ULobbyPanelWidget` 등 | C++로 만든 UMG 위젯 |
| **System** | `UMusicSubsystem`, `UConnectionNoticeSubsystem` | 맵을 넘는 배경음, 접속 실패 안내 |
| **Tool** | `UPropIconCommandlet` | 물체 썸네일 자동 생성 (에디터 전용) |

</details>

---

## 🎮 조작법

![Input Enhanced Input](https://img.shields.io/badge/Input-Enhanced_Input-444444?logo=unrealengine) ![Device Keyboard & Mouse](https://img.shields.io/badge/Device-Keyboard_%26_Mouse-555555)

| 공통 | | 러너 | | 술래 | | 유령 | |
| :-: | --- | :-: | --- | :-: | --- | :-: | --- |
| `WASD` | 이동 | `F` / 우클릭 | 물체에 숨기·나오기 | 좌클릭 | 마법 발사 | `Space` / `Ctrl` | 위 / 아래 |
| `Space` | 점프 | `T` | 제자리 고정 | 좌클릭 꾹 | 물체 파괴 | 아이콘 클릭 | 물체 고르기 |
| `Tab` | 점수판 · 참가자 | `Q` / `E` | 고정 중 회전 | | | 좌클릭 | 물체 놓기 |
| `H` | 조작법 안내 | `R` | 똑바로 세우기 | | | `R` | 목록 새로고침 |
| `ESC` / `F10` | 설정 | 우클릭 | 비밀번호 입력 | | | `V` | 관전 |
| `E` | 로비 장치 | | | | | | |
| `Enter` | 로비 채팅 | | | | | | |

---

## 🚀 시작하기

![Platform Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows) ![Unreal Engine 5.4](https://img.shields.io/badge/Unreal_Engine-5.4-0E1128?logo=unrealengine) ![Visual Studio 2022](https://img.shields.io/badge/Visual_Studio-2022-5C2D91?logo=visualstudio)

### 요구 사항

- Windows 10/11
- Unreal Engine **5.4** (데디케이티드 서버 빌드는 **소스 빌드 엔진** 필요)
- Visual Studio 2022 (C++ 게임 개발 워크로드)

### 빌드 & 실행

```powershell
# 1) 저장소 받기
git clone https://github.com/ChuChuSearch/ue5-chuchusearch-hideandseek.git

# 2) Hide.uproject 우클릭 → Generate Visual Studio project files → 에디터 빌드 후 실행
```

| 방식 | 방법 |
| --- | --- |
| 🧪 에디터 테스트 | Play 옵션 **Number of Players 2+**, **Play As Listen Server** |
| 🏠 리슨 서버 | 클라이언트 실행 → 닉네임 입력 → **방 만들기** (UDP 7777 허용 필요) |
| ☁️ 데디케이티드 | `HideServer` 패키징 → `HideServer.exe -log` → 클라이언트에서 서버 주소로 **접속하기** |

> 처음 화면의 기본 서버 주소는 `Config/DefaultGame.ini`의 `[Hide.Title] DefaultServerAddress`에서 바꿀 수 있습니다.

---

## 🛠 Tech Stack

![Unreal Engine 5.4](https://img.shields.io/badge/Unreal_Engine-5.4-0E1128?logo=unrealengine) ![C++](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus) ![Blueprint](https://img.shields.io/badge/Blueprint-Visual_Scripting-137CBD?logo=unrealengine) ![UMG](https://img.shields.io/badge/UMG-C++-5C2D91) ![Enhanced Input](https://img.shields.io/badge/Enhanced_Input-UE5-444444?logo=unrealengine) ![Replication](https://img.shields.io/badge/Replication-RPC-8A2BE2) ![AWS EC2](https://img.shields.io/badge/AWS-EC2-FF9900?logo=amazonwebservices) ![Visual Studio](https://img.shields.io/badge/Visual_Studio-2022-5C2D91?logo=visualstudio) ![Git](https://img.shields.io/badge/Git-GitHub-F05032?logo=git)

---

## 💡 배운 점

![Server Authority 검증](https://img.shields.io/badge/Server_Authority-%EA%B2%80%EC%A6%9D-D64046) ![Game Framework 역할 분리](https://img.shields.io/badge/Game_Framework-%EC%97%AD%ED%95%A0_%EB%B6%84%EB%A6%AC-0E1128?logo=unrealengine) ![Automation Commandlet](https://img.shields.io/badge/Automation-Commandlet-555555) ![Cloud AWS EC2](https://img.shields.io/badge/Cloud-AWS_EC2-FF9900?logo=amazonwebservices)

- **서버 권한 기반 멀티플레이 설계**: 클라이언트 요청을 서버에서 다시 검증하고, 필요한 정보만 필요한 사람에게 복제(상대 팀 점수, 비밀번호 정답 등)
- **Game Framework 역할 분리**: GameMode · GameState · PlayerState · PlayerController · Character의 책임을 나눠 기능이 늘어도 구조를 유지
- **리슨/데디케이티드 겸용 구조**: 서버가 플레이어가 아닐 수 있다는 전제로 방장·사운드·UI 로직 작성
- **맵 이동과 상태 관리**: Seamless Travel과 재접속 이동의 차이, 맵을 넘는 데이터(`GameInstance`/`LocalPlayer` 서브시스템)
- **코드로 만드는 UI**: 위젯 블루프린트 없이 C++로 UMG를 구성해 디자인 톤 통일과 유지보수성 확보
- **에디터 자동화**: 에디터 스크립트·커맨드렛으로 맵 배치, 메시 분리, 충돌 생성, 썸네일 제작 같은 반복 작업 자동화
- **클라우드 배포**: AWS EC2에 데디케이티드 서버를 올리며 보안 그룹·방화벽·포트 구성 경험

---

## 📋 개발 현황

![Status 개발 중](https://img.shields.io/badge/Status-%EA%B0%9C%EB%B0%9C_%EC%A4%91-success)

현재 개발 진행 중이며, README는 **구현이 완료된 기능을 기준으로** 작성합니다.

<div align="center">

<sub>Made with ❤️ and Unreal Engine by the ChuChuSearch team</sub>

</div>
