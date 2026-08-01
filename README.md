# Mission001 — PID와 가상 주소를 IOCTL로 전달

이 솔루션은 유저모드 프로그램에서 `PID`와 `VirtualAddress`를
`METHOD_BUFFERED` IOCTL로 KMDF 드라이버에 전달하고 WinDbg에 출력합니다.

## 구성

- `Mission001Driver`: IOCTL을 수신하고 WinDbg 로그를 출력하는 KMDF 드라이버
- `Mission001User`: PID와 주소를 보내는 x64 콘솔 프로그램
- `Shared/Ioctl.h`: 양쪽이 공유하는 IOCTL 번호와 16바이트 데이터 구조

## 1. 빌드

`Mission001.sln`을 Visual Studio 2022로 열고 상단을 `Debug | x64`로 설정한 뒤
`빌드 > 솔루션 다시 빌드`를 선택합니다.

생성되는 핵심 파일은 다음과 같습니다.

- `Mission001Driver/x64/Debug/Mission001Driver.sys`
- `Mission001User/x64/Debug/Mission001User.exe`

출력 폴더는 Visual Studio/WDK 버전에 따라 `x64/Debug` 또는 프로젝트의
하위 폴더로 조금 달라질 수 있습니다.

## 2. 테스트 VM 준비

아래 작업은 반드시 테스트용 VM의 **관리자 권한 터미널**에서 수행합니다.
개발 PC가 아니라 드라이버를 실행할 VM에 `.sys`와 `.exe`를 복사합니다.

테스트 서명 모드를 켜야 한다면 다음 명령 실행 후 VM을 재부팅합니다.

```powershell
bcdedit /set testsigning on
```

Secure Boot가 켜져 있으면 위 설정이 거부될 수 있습니다. 테스트 VM 설정에서
Secure Boot를 끈 뒤 다시 시도합니다.

## 3. 드라이버 등록 및 시작

예를 들어 두 파일을 VM의 `C:\Mission001`에 복사했다면:

```powershell
sc.exe create Mission001Driver type= kernel start= demand binPath= "C:\Mission001\Mission001Driver.sys"
sc.exe start Mission001Driver
sc.exe query Mission001Driver
```

`STATE : 4 RUNNING`이면 정상입니다.

## 4. WinDbg 로그 설정

커널 디버깅이 연결된 WinDbg 명령창에서:

```text
ed nt!Kd_IHVDRIVER_Mask 0xFFFFFFFF
g
```

첫 명령은 과제 드라이버가 사용하는 IHVDRIVER 로그를 보이게 합니다.
`g`는 중단된 대상 VM을 계속 실행합니다.

드라이버가 이미 실행 중이었다면 로그를 확실히 다시 보기 위해 VM에서:

```powershell
sc.exe stop Mission001Driver
sc.exe start Mission001Driver
```

WinDbg에 다음 로그가 나타납니다.

```text
[Mission001] DriverEntry: driver loaded.
[Mission001] Device ready: \\.\Mission001
```

## 5. PID와 가상 주소 전송

VM의 관리자 권한 터미널에서 인자 없이 실행하는 방법이 가장 간단합니다.
프로그램 자신의 PID와 `sampleValue` 변수의 실제 가상 주소가 사용됩니다.

```powershell
C:\Mission001\Mission001User.exe
```

콘솔 예시:

```text
[SEND] PID=4321, VirtualAddress=0x000000A1B2C3F8A0
[OK] The driver accepted the IOCTL. Check WinDbg.
```

WinDbg에는 동일한 값이 출력됩니다.

```text
[Mission001] IOCTL received: PID=4321, VirtualAddress=0x000000A1B2C3F8A0
```

원하는 값을 직접 지정할 수도 있습니다.

```powershell
C:\Mission001\Mission001User.exe 4321 0x00007FF612341000
```

## 6. 캡처

WinDbg 창을 넓혀 아래 한 줄이 완전히 보이도록 하고 캡처합니다.

```text
[Mission001] IOCTL received: PID=..., VirtualAddress=0x...
```

파일명은 `Mission01.png`로 저장합니다.

## 정리 명령

실습 후 VM의 관리자 권한 터미널에서:

```powershell
sc.exe stop Mission001Driver
sc.exe delete Mission001Driver
```

## 코드에서 중요한 이유

- `DeviceIoControl`: 유저모드가 커널 드라이버로 IOCTL 요청을 보냅니다.
- `METHOD_BUFFERED`: I/O 관리자가 유저 입력을 커널 버퍼로 복사해 줍니다.
- `WdfRequestRetrieveInputBuffer`: 드라이버가 해당 커널 버퍼를 안전하게 얻습니다.
- 입력 길이 검사: 작은/잘못된 버퍼를 구조체로 읽는 것을 막습니다.
- `DbgPrintEx`: 수신한 PID와 주소를 WinDbg로 출력합니다.

