# Restime for Windows - Implementation Plan

## 1. Muc tieu

Xay dung phien ban Restime cho Windows, chay nen trong notification area
(system tray) va hien thi tinh trang gioi han su dung Codex.

Uu tien cao nhat:

1. Ton it RAM va CPU nhat co the.
2. Khong mo cua so chinh khi khoi dong.
3. Khong khoi chay Codex CLI trong qua trinh refresh.
4. Hoat dong on dinh khi mang mat, token het han, Codex chua dang nhap,
   session log lon hoac Windows Explorer khoi dong lai.
5. Phan phoi duoi dang mot file `.exe` nho gon cho Windows 10/11 x64.

Ten workspace:

```text
D:\restimewd
```

Ten san pham du kien:

```text
Restime
```

## 2. Gioi han cua taskbar Windows

Windows khong cho ung dung hien thi mot chuoi dai truc tiep tren taskbar theo
cach macOS menu bar hien thi:

```text
5h 80% 14:30 | W 65% 20/6
```

Phien ban Windows se dung system tray:

- Tray icon luon hien thi trang thai tong quat.
- Tooltip hien thi chuoi usage day du khi re chuot vao icon.
- Click trai mo mot popup/menu nho chua thong tin chi tiet.
- Click phai mo context menu co `Refresh now`, `Start with Windows`,
  `About` va `Quit`.
- Tray icon co the render dong con so phan tram 5h de nguoi dung nhin nhanh.

Khong tao nut taskbar hoac cua so chinh thuong truc, vi cach do ton RAM hon va
lam taskbar bi chiem cho khong can thiet.

## 3. Lua chon cong nghe

### Lua chon chinh: C++20 + Win32 API

Thanh phan:

- UI/tray: Win32 API va `Shell_NotifyIconW`.
- HTTP: WinHTTP.
- JSON: RapidJSON header-only, chi build cac phan can dung.
- JWT payload decode: Windows Cryptography API.
- Timer/background work: thread pool timer cua Windows.
- Build: CMake + MSVC Release.
- Test: mot executable test nho, khong can framework nang o ban dau.

Ly do:

- Khong can .NET runtime rieng trong process.
- Khong dung WPF, WinUI 3, WebView2 hoac Electron.
- Khong co rendering engine hoac cua so UI nang chay ngam.
- Win32 tray app co working set va startup time rat nho.
- Co the dong goi thanh mot executable duy nhat.

Khong chon:

- Electron/Tauri WebView: them web runtime va ton RAM khong can thiet.
- WPF/WinUI: de phat trien hon nhung working set cao hon muc tieu.
- Rust: van phu hop, nhung C++/Win32 truc tiep co duong di gon nhat cho mot
  tray utility rat nho va bo cong cu Windows pho bien hon.

## 4. Ngan sach tai nguyen va tieu chi nghiem thu

Muc tieu Release x64 tren Windows 11:

| Chi so | Muc tieu | Gioi han nghiem thu |
|---|---:|---:|
| Private working set sau 10 phut idle | <= 10 MB | <= 20 MB |
| Private working set sau 100 lan refresh | <= 12 MB | <= 25 MB |
| CPU khi idle | gan 0% | trung binh < 0.1% |
| CPU moi lan refresh | burst ngan | khong giu CPU lien tuc |
| So thread khi idle | <= 3 | <= 5 |
| Kich thuoc EXE Release | <= 2 MB | <= 5 MB |
| Startup den khi co tray icon | < 300 ms | < 1 giay |
| Disk write khi usage khong doi | 0 | 0 |

Khong dung `EmptyWorkingSet` hoac thu thuat ep Windows trim RAM de lam dep
chi so. Can giam allocation va dependency thuc su.

Chu ky refresh mac dinh:

```text
60 giay
```

Khong giu chu ky 15 giay cua ban macOS vi usage khong can cap nhat nhanh den
vay, va 60 giay giam network wake-up, CPU va pin. Click vao tray/menu se yeu
cau refresh ngay neu lan refresh gan nhat da qua 10 giay.

## 5. Pham vi tinh nang ban dau

### Bat buoc cho v1

- Single-instance: chi mot Restime duoc chay.
- Tray icon va tooltip usage.
- Doc tai khoan tu `%USERPROFILE%\.codex\auth.json`.
- Lay usage tu `https://chatgpt.com/backend-api/wham/usage`.
- Neu API loi, fallback sang `%USERPROFILE%\.codex\sessions\**\*.jsonl`.
- Hien thi:
  - Email tai khoan hien tai.
  - Phan tram con lai cua cua so 5h.
  - Gio reset cua cua so 5h.
  - Phan tram con lai hang tuan.
  - Ngay reset hang tuan.
  - Thoi diem cap nhat cuoi.
  - Nguon du lieu: API hoac local log.
  - Loi refresh gan nhat neu co.
- Menu `Refresh now`.
- Menu `Start with Windows`.
- Menu `Quit`.
- Giu usage cuoi cung khi refresh moi bi loi.
- Khoi phuc tray icon khi `explorer.exe` restart.

### Nen co sau khi v1 on dinh

- Lich su tai khoan tren may.
- Xoa tung tai khoan khoi lich su.
- Mau/icon canh bao khi 5h hoac weekly con thap.
- Tuy chon chu ky refresh.
- Installer va code signing.

### Khong lam trong v1

- Cua so dashboard day du.
- Notification lien tuc.
- Auto-update.
- Multi-platform chung mot codebase.
- Telemetry.

## 6. Kien truc du kien

```text
Restime.exe
|
+-- App
|   +-- khoi tao process, single-instance mutex, message loop
|
+-- TrayController
|   +-- tao/cap nhat tray icon
|   +-- tooltip, click, context menu
|   +-- xu ly TaskbarCreated khi Explorer restart
|
+-- RefreshCoordinator
|   +-- timer, chong refresh trung lap, backoff khi loi
|   +-- chay I/O ngoai UI/message thread
|
+-- CodexAuthReader
|   +-- doc auth.json
|   +-- decode JWT va lay email/access token
|
+-- CodexUsageApiClient
|   +-- WinHTTP request
|   +-- parse API response
|
+-- SessionUsageReader
|   +-- tim session JSONL moi nhat
|   +-- doc nguoc/tail file va parse rate_limits
|
+-- AccountHistoryStore
|   +-- luu lich su tai khoan nho gon
|
+-- UsageFormatter
    +-- tao tooltip va noi dung menu
```

Nguyen tac:

- Message thread chi xu ly UI/tray; khong block boi network hoac disk scan.
- Tai moi thoi diem chi cho phep mot refresh.
- Tai su dung WinHTTP session/connection thay vi tao lai moi lan.
- Chi cap nhat tray icon/tooltip khi noi dung thay doi.
- Khong giu session log hoac JSON document lon trong RAM sau khi parse.
- Khong ghi log ra disk mac dinh trong Release.

## 7. Luong du lieu

### Khi khoi dong

1. Tao named mutex de bao dam single-instance.
2. Tao hidden message-only window.
3. Dang ky message `TaskbarCreated`.
4. Tao tray icon voi trang thai chua co du lieu.
5. Doc setting nho tu Registry.
6. Bat dau refresh tren background thread.
7. Lap timer cho lan refresh tiep theo.

### Khi refresh

1. Doc timestamp/size cua `auth.json`.
2. Neu file khong doi, tai su dung email va access token dang co trong RAM.
3. Goi usage API voi timeout ket noi va response ngan.
4. Neu API thanh cong, parse usage va cap nhat state.
5. Neu API loi, tim local session JSONL moi nhat va doc phan tail.
6. Post ket qua ve message thread.
7. Chi render/cap nhat tray khi state thay doi.
8. Neu loi, giu usage cu va hien thi loi trong menu.

### Khi Codex account thay doi

1. Phat hien `auth.json` thay doi tai lan refresh.
2. Decode email/token moi.
3. Luu snapshot tai khoan cu neu can.
4. Reset usage hien tai.
5. Refresh usage cho tai khoan moi.

## 8. Doc auth va bao mat

Duong dan:

```text
%USERPROFILE%\.codex\auth.json
```

Can doc:

- `tokens.access_token`
- Email tu payload cua `tokens.id_token`
- Fallback email tu profile claim trong access token

Quy tac bao mat:

- Khong ghi access token vao log, Registry, dump debug hoac history.
- Khong gui token den endpoint nao ngoai Codex usage endpoint.
- Xoa/ghi de buffer token khi process ket thuc neu thuan tien.
- Gioi han kich thuoc `auth.json` duoc phep doc, vi du 1 MB.
- Chi chap nhan HTTPS endpoint duoc hard-code.
- Timeout request ngan va huy request khi thoat ung dung.

## 9. Doc usage API

Endpoint:

```text
https://chatgpt.com/backend-api/wham/usage
```

Header:

```text
Authorization: Bearer <access_token>
Accept: application/json
```

Du lieu can parse:

```text
rate_limit.primary_window.used_percent
rate_limit.primary_window.reset_at
rate_limit.secondary_window.used_percent
rate_limit.secondary_window.reset_at
```

Chuyen `used_percent` thanh remaining percent:

```text
remaining = clamp(round(100 - used), 0, 100)
```

Timeout de xuat:

- Resolve/connect: 2 giay.
- Send/receive: 5 giay.
- Tong refresh khong qua 8 giay.

Backoff khi loi lien tiep:

```text
1 phut -> 2 phut -> 5 phut -> toi da 15 phut
```

Click `Refresh now` bo qua backoff, nhung khong tao refresh trung lap.

## 10. Fallback local session log

Duong dan:

```text
%USERPROFILE%\.codex\sessions\**\*.jsonl
```

Chien luoc tiet kiem RAM/CPU:

1. Khong doc toan bo cay session moi 60 giay neu API van hoat dong.
2. Chi fallback khi API loi.
3. Tim toi da 12 file JSONL gan nhat.
4. Voi file lon, chi doc tail toi da 4 MB.
5. Doc tung block tu cuoi file va dung khi tim duoc event `rate_limits` hop le.
6. Khong load toan bo file vao mot `std::string`.
7. Gioi han do dai mot dong JSON, vi du 1 MB.

Du lieu local can parse:

```text
timestamp
payload.rate_limits.primary.used_percent
payload.rate_limits.primary.resets_at
payload.rate_limits.secondary.used_percent
payload.rate_limits.secondary.resets_at
```

## 11. Thiet ke tray UI

### Tray icon

- Dung icon tinh mac dinh khi chua co usage.
- Render bitmap nho co so remaining 5h khi co usage.
- Mau de xuat:
  - Xanh: tren 50%.
  - Vang: 20-50%.
  - Do: duoi 20%.
  - Xam: khong co du lieu/loi.
- Cache icon theo bucket gia tri de tranh tao/huy GDI object lien tuc.
- Huy moi `HICON`, menu va GDI object dung cach.

### Tooltip

Vi tooltip tray bi gioi han do dai, dung format ngan:

```text
Restime | 5h 80% 14:30 | W 65% 20/6
```

### Popup/context menu

```text
Codex Usage
Account: user@example.com
-------------------------
5h remaining: 80%
5h reset: 14:30
Weekly remaining: 65%
Weekly reset: 20/6
-------------------------
Last updated: 13:42
Source: Codex API
-------------------------
Refresh now
Start with Windows       [x]
About
Quit
```

Khong tao popup framework rieng trong v1. Dung native context menu de RAM thap,
phu hop Windows va co accessibility/keyboard behavior san.

## 12. Luu setting va account history

Setting nho nen luu trong Registry:

```text
HKCU\Software\Restime
```

Gia tri:

- `StartWithWindows`
- `RefreshIntervalSeconds`
- `AccountHistory` neu v1 can history

Startup:

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
```

Chi ghi Registry khi nguoi dung thay doi setting hoac account snapshot thay
doi. Khong ghi moi lan refresh neu status khong doi.

## 13. Cau truc repository du kien

```text
D:\restimewd
|-- CMakeLists.txt
|-- CMakePresets.json
|-- README.md
|-- plan.md
|-- LICENSE
|-- assets
|   |-- restime.ico
|   `-- restime.rc
|-- src
|   |-- main.cpp
|   |-- app.cpp
|   |-- app.h
|   |-- tray_controller.cpp
|   |-- tray_controller.h
|   |-- refresh_coordinator.cpp
|   |-- refresh_coordinator.h
|   |-- codex_auth_reader.cpp
|   |-- codex_auth_reader.h
|   |-- codex_usage_reader.cpp
|   |-- codex_usage_reader.h
|   |-- session_usage_reader.cpp
|   |-- session_usage_reader.h
|   |-- account_history_store.cpp
|   |-- account_history_store.h
|   |-- usage.cpp
|   |-- usage.h
|   |-- usage_formatter.cpp
|   `-- usage_formatter.h
|-- tests
|   |-- test_main.cpp
|   |-- auth_reader_tests.cpp
|   |-- usage_parser_tests.cpp
|   |-- session_reader_tests.cpp
|   `-- history_store_tests.cpp
|-- third_party
|   `-- rapidjson
|-- tools
|   |-- build-release.ps1
|   |-- test.ps1
|   `-- measure-resources.ps1
`-- dist
```

`dist` va build output phai nam trong `.gitignore`.

## 14. Build va compiler flags

Can cai:

- Visual Studio 2022 Build Tools.
- Desktop development with C++ workload.
- Windows 10/11 SDK.
- CMake va Ninja.

May hien tai:

- Co .NET 8 runtime.
- Khong co .NET SDK.
- Chua phat hien `git`, `cl` hoac `msbuild` trong PATH.

Vi chon native C++, .NET SDK khong can thiet.

Release flags de xuat:

```text
/O2 /GL /Gy /Gw /DNDEBUG
/link /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS
```

Can benchmark ca `/MD` va `/MT`:

- `/MD`: EXE nho hon, phu thuoc Visual C++ Runtime.
- `/MT`: EXE doc lap hon, kich thuoc lon hon mot chut.

Mac dinh cho ban portable v1: `/MT`, mot executable duy nhat.

## 15. Ke hoach test

### Unit test

- Parse API response hop le.
- Parse JSONL `rate_limits` hop le.
- Clamp remaining percent trong khoang 0-100.
- Parse timestamp/reset epoch.
- Decode email tu ID token.
- Fallback email tu access token profile.
- Auth JSON thieu/hong/qua lon.
- API JSON thieu field hoac sai kieu.
- Session file rong, hong, dong rat dai va file lon.
- Account history case-insensitive.
- Usage formatter va tooltip length.

### Integration test

- Mock local HTTP endpoint cho success, timeout, 401, 500 va malformed JSON.
- API loi thi fallback local log.
- Khong co auth.json.
- Codex account thay doi khi app dang chay.
- Explorer restart va tray icon xuat hien lai.
- Chay executable lan hai khong tao tray icon thu hai.
- `Start with Windows` bat/tat dung Registry.

### Resource/leak test

- Chay idle 10 phut va ghi private working set/CPU/thread count.
- Refresh 100 lan voi API mock.
- Mo/dong menu 500 lan.
- Doi tray icon 1.000 lan.
- Kiem tra GDI object count va USER object count khong tang dan.
- Kiem tra handle count khong tang dan.
- Kiem tra RAM sau stress tro ve gan baseline.

Tool do:

- PowerShell `Get-Process`.
- Windows Performance Recorder/Analyzer.
- Visual Studio Diagnostic Tools.
- Application Verifier neu can.

## 16. Cac giai doan thuc hien

### Phase 0 - Chuan bi moi truong

- Cai Visual Studio Build Tools + C++ workload + Windows SDK.
- Cai Git, CMake va Ninja.
- Khoi tao Git repository trong `D:\restimewd`.
- Tao `.gitignore`, CMake project va script build/test.
- Build mot Windows subsystem executable rong.

Hoan thanh khi:

- Build Release x64 thanh cong.
- Executable chay khong mo console.

### Phase 1 - Tray shell toi thieu

- Tao hidden message-only window.
- Tao single-instance mutex.
- Tao tray icon.
- Them native context menu `Refresh now`, `About`, `Quit`.
- Xu ly Explorer restart.
- Don sach icon/menu/handle khi thoat.

Hoan thanh khi:

- Tray icon hoat dong on dinh.
- Idle private working set nam trong gioi han muc tieu ban dau.

### Phase 2 - Domain model, auth va API

- Them `CodexUsage` va formatter.
- Doc/decode `auth.json`.
- Goi usage API bang WinHTTP tren background.
- Parse response va cap nhat tooltip/menu.
- Them timeout, cancellation va error state.
- Them unit/integration test.

Hoan thanh khi:

- Hien thi usage API dung ma khong block message thread.
- Token khong bi ghi ra disk/log.

### Phase 3 - Local session fallback

- Quet session file gan nhat.
- Doc tail theo block.
- Parse event `rate_limits`.
- API loi thi fallback local.
- Them test file lon va malformed data.

Hoan thanh khi:

- Fallback dung, khong load file lon vao RAM.

### Phase 4 - Settings va account history

- Them `Start with Windows`.
- Them account history neu van nam trong ngan sach RAM/code complexity.
- Giam disk/Registry write.
- Hoan thien menu va error display.

Hoan thanh khi:

- Setting ben vung qua restart.
- Account switch khong lam lo token va khong mat usage cu can thiet.

### Phase 5 - Toi uu va hardening

- Chay leak/resource test.
- Profile startup, refresh va idle.
- Loai bo allocation/dependency khong can thiet.
- Kiem tra timeout, offline, sleep/resume va Explorer restart.
- Kiem tra Windows 10 va Windows 11 x64.

Hoan thanh khi:

- Dat cac gioi han RAM, CPU, handle va startup da dat ra.

### Phase 6 - Dong goi

- Tao portable Release `.exe`.
- Them version metadata va icon.
- Tao SHA-256 checksum.
- Viet README cai dat/chay/go bo.
- Tuy chon tao installer sau khi portable build on dinh.
- Code sign neu co certificate.

Hoan thanh khi:

- Mot may Windows sach co the chay Restime va go bo de dang.

## 17. Rui ro va cach xu ly

### Endpoint/API thay doi

Rui ro: endpoint `wham/usage` hoac JSON schema thay doi.

Xu ly:

- Tach API parser khoi tray/UI.
- Loi parse khong crash app.
- Local session fallback.
- Test fixture cho schema hien tai.

### Windows an tray icon

Rui ro: Windows co the dua icon vao overflow menu.

Xu ly:

- Giai thich trong README cach pin icon.
- Khong co API hop le de ep icon luon hien tren taskbar.

### Auth/token thay doi trong khi doc

Rui ro: Codex dang ghi `auth.json` cung luc Restime doc.

Xu ly:

- Retry ngan mot lan.
- Khong xoa state cu neu lan doc moi tam thoi hong.

### Session tree rat lon

Rui ro: recursive scan ton CPU/disk.

Xu ly:

- Chi fallback khi API loi.
- Cache file gan nhat.
- Gioi han so file va byte doc.
- Co the them USN/file watcher sau nay neu benchmark cho thay can.

### Memory leak tu GDI/tray/menu

Rui ro: icon dong va context menu de ro ri handle.

Xu ly:

- RAII wrapper cho `HANDLE`, `HICON`, `HMENU`, `HINTERNET`.
- Stress test va theo doi GDI/USER/handle count.

## 18. Definition of Done cho v1

v1 duoc xem la hoan thanh khi:

- Restime chay nhu mot Windows tray app khong co console/cua so chinh.
- Chi mot instance duoc chay.
- Doc dung account Codex tren Windows.
- Hien thi usage tu API va fallback local log khi API loi.
- Refresh khong block UI va khong spawn Codex CLI.
- Hoat dong dung sau sleep/resume va Explorer restart.
- Co `Refresh now`, `Start with Windows` va `Quit`.
- Khong luu hoac log access token.
- Tat ca test bat buoc pass.
- Khong co xu huong tang RAM, handle, GDI object sau stress test.
- Private working set va CPU dat gioi han nghiem thu.
- Tao duoc mot portable Release `.exe` kem checksum va README.

## 19. Thu tu cong viec tiep theo

Cong viec tiep theo nen thuc hien theo thu tu:

1. Cai bo cong cu C++/CMake/Git can thiet.
2. Scaffold repository va build Release x64 rong.
3. Do baseline RAM cua executable rong.
4. Xay tray shell va do lai RAM.
5. Them auth/API theo tung module, do RAM sau moi phase.
6. Them local fallback, test, settings va dong goi.

Moi phase chi duoc xem la xong sau khi da build, test va ghi lai chi so tai
nguyen. Cach nay giup phat hien som module nao lam RAM tang bat thuong.
