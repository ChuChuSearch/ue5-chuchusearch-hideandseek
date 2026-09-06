# 🎮 ChuChuSearch

> **Unreal Engine 기반 3인칭 멀티플레이 숨바꼭질 게임**

플레이어가 술래와 숨는 역할로 나뉘어 진행하는 멀티플레이 숨바꼭질 게임입니다.

3인 팀 프로젝트에서 **프로그래밍을 전담**하여 Unreal Engine의  
**C++ / Blueprint**를 기반으로 게임 시스템과 플레이어 상호작용 기능을 구현하고 있습니다.
---
## 🎬 Gameplay Video

### ChuChuSearch 시연영상

[![ChuChuSearch Gameplay](https://img.youtube.com/vi/NzZujXlLXhQ/maxresdefault.jpg)](https://youtu.be/NzZujXlLXhQ)

[![YouTube](https://img.shields.io/badge/▶%20Watch%20on%20YouTube-FF0000?style=for-the-badge&logo=youtube&logoColor=white)](https://youtu.be/NzZujXlLXhQ)
---

## 🛠 Tech Stack

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-0E1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Blueprint](https://img.shields.io/badge/Blueprint-137CBD?style=for-the-badge&logo=unrealengine&logoColor=white)
![Git](https://img.shields.io/badge/Git-F05032?style=for-the-badge&logo=git&logoColor=white)

---

## 📌 Project Overview

| 항목 | 내용 |
| --- | --- |
| 프로젝트명 | ChuChuSearch |
| 장르 | 3인칭 멀티플레이 숨바꼭질 |
| 개발 기간 | 2025.11 ~ 2026.10 (진행 중) |
| 개발 인원 | 3인 |
| 담당 역할 | 프로그래밍 전담 (개발자 1인) |
| 엔진 | Unreal Engine |
| 개발 언어 | C++, Blueprint |
| 형상 관리 | Git |

---

## 👨‍💻 담당 역할

3인 팀에서 **유일한 프로그래머로 게임 개발 전반을 담당**했습니다.

- C++ / Blueprint 기반 게임플레이 시스템 구현
- Unreal Game Framework 기반 클래스 구조 설계
- 플레이어 입력 및 상호작용 구현
- 술래 공격 시스템 구현
- Prop 상호작용 및 파괴 처리
- 게임 상태 및 플레이어 상태 관리
- 대기 공간과 실제 게임의 GameMode 분리
- UI와 게임 로직 연동

---

## 🎯 Main Features

### 1. Game Framework 기반 게임 구조

Unreal Engine의 Game Framework를 활용하여  
게임 규칙, 전체 게임 상태, 플레이어별 상태와 입력 처리의 역할을 분리했습니다.

```text
GameMode
 ├─ 게임 규칙 및 진행 관리
 │
GameState
 ├─ 게임 전체의 공유 상태 관리
 │
PlayerState
 ├─ 플레이어별 상태 관리
 │
PlayerController
 ├─ 입력 및 플레이어 상호작용
 │
Character
 └─ 캐릭터 행동
```

대기 공간과 실제 게임의 서로 다른 규칙을 관리하기 위해  
`WaitRoomGameMode`와 실제 플레이용 GameMode를 분리하여 구성했습니다.

---

### 2. PlayerController 기반 플레이어 기능

`PlayerController`를 중심으로 플레이어의 입력과 게임 내 상호작용을 처리했습니다.

- 플레이어 입력 처리
- 공격 처리
- 오브젝트 Possession
- 오브젝트 Lock 처리
- UI 상호작용

입력 처리와 Character의 실제 동작을 분리하여  
PlayerController가 플레이어의 명령을 게임플레이 로직으로 연결하도록 구성했습니다.

---

### 3. 술래 공격 시스템

술래가 Projectile을 발사하여 플레이어나 Prop과 상호작용할 수 있는 공격 시스템을 구현했습니다.

```text
Seeker Attack
      │
      ▼
Projectile Spawn
      │
      ▼
   Collision
    ┌──┴──┐
    ▼     ▼
 Player  Prop
    │     │
    ▼     ▼
   Hit  Destroy
```

- Projectile 생성 및 발사
- 충돌 대상 판별
- 플레이어 피격 처리
- Prop 충돌 및 파괴 처리
- 파괴된 Prop의 `RespawnList` 등록

Projectile의 충돌 결과가 개별 객체의 처리에서 끝나지 않고  
게임의 오브젝트 관리 로직과 연결되도록 구성했습니다.

---

### 4. Prop 상호작용

숨바꼭질 게임에서 사용되는 Prop을 별도의 `PropBase`로 구성하여  
플레이어 및 술래의 공격과 상호작용할 수 있도록 구현했습니다.

```text
PropBase
   │
   ├─ Player Interaction
   │
   └─ SeekerProjectile Collision
                │
                ▼
             Destroy
                │
                ▼
           RespawnList
```

이를 통해 개별 Prop마다 동일한 로직을 구현하지 않고  
공통 동작을 기반으로 다양한 Prop을 구성할 수 있도록 했습니다.

---

## 🧩 Main C++ Classes

| Class | 역할 |
| --- | --- |
| `MyPlayerController` | 입력, 공격, Possession, Lock, UI 상호작용 |
| `MyCharacter` | 플레이어 캐릭터 동작 |
| `MyGameMode` | 실제 게임의 규칙 및 진행 관리 |
| `WaitRoomGameMode` | 대기 공간의 게임 규칙 관리 |
| `MyGameState` | 게임 전체에서 공유되는 상태 관리 |
| `MyPlayerState` | 플레이어별 상태 관리 |
| `PropBase` | 게임 내 Prop의 공통 기능 |
| `SeekerProjectile` | 술래 Projectile 공격 및 충돌 처리 |

---

## 📂 Project Structure

```text
Source/Hide/
├── MyPlayerController
├── MyCharacter
├── MyGameMode
├── MyGameState
├── MyPlayerState
├── WaitRoomGameMode
├── PropBase
└── SeekerProjectile
```

> 실제 Source 폴더에서는 각 클래스가 `.h` / `.cpp` 파일로 구성되어 있습니다.

---

## 💡 What I Learned

3인 팀에서 프로그래밍을 전담하며 단순히 개별 기능을 구현하는 것뿐만 아니라  
여러 게임 시스템이 함께 동작할 수 있도록 전체 구조를 설계하는 경험을 했습니다.

특히 프로젝트를 통해 다음 내용을 실제 개발에 적용했습니다.

- Unreal Engine C++와 Blueprint를 함께 활용한 게임 개발
- GameMode, GameState, PlayerState, PlayerController의 역할 분리
- 객체지향 구조를 활용한 게임플레이 클래스 설계
- Projectile과 Collision을 활용한 상호작용 구현
- 플레이어와 Prop 간 상호작용 처리
- 게임 로직과 UI의 연동
- Git을 활용한 프로젝트 버전 관리

---

## 🔧 Development Environment

- Unreal Engine
- C++
- Blueprint
- Git

---

## 📋 Development Status

현재 개발 진행 중인 프로젝트입니다.

게임플레이 시스템과 콘텐츠를 지속적으로 구현 및 개선하고 있으며,  
README에는 **현재 구현이 완료된 기능을 기준으로 작성**하고 있습니다.
