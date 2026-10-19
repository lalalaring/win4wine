# 文档（中文摘要）

[日本語](README.md) · **简体中文**

`docs/` 里的设计文档原文是日文（上游新加了 Windows 7 外壳，见下面的 [windows7.md](windows7.md) 一节）。这一页是它们的中文摘要：每篇文档的要点、关键文件与结论，方便快速定位；
细节（测量值、表格、补丁逐条说明）仍然以原文为准，链接在每节标题上。

只想把系统装起来用的话，看仓库的 [README（中文）](../.github/readme/README.zh.md) 和
[安装指南（中文）](../.github/readme/INSTALL.zh.md)。

需要说明的一点：这些文档写于「桌面属性部件、开始菜单里的 Windows 工具、Proton 的 Direct3D 层、双击 .exe」之前，
这几项的系统安装与用法写在
[安装指南「补上外壳缺的小部件」](../.github/readme/INSTALL.zh.md#补上外壳缺的小部件) 和
[「把 Proton 的兼容层装进这个 Wine」](../.github/readme/INSTALL.zh.md#把-proton-的兼容层装进这个-wine) 两节里。

## [architecture.md](architecture.md) — 工作机制

**整体流程**：显示管理器（LightDM）→ `session/supervisor.py`（启动并看守会话）→ 同时跑 `bridge-wm/wm.py`（X11 窗口管理器）、
`bridge-wm/tray.py`（托盘图标中介）、`fcitx5`（中日文输入）、`proxy.exe`（Wine 里的 Linux 窗口代理与 XP 边框）、
`xp-explorer.exe`（XP 的外壳）。它们之间靠 IPC 目录里的文件通信。

**准备阶段**（`scripts/setup.sh` 会调这些）：

- `extract/extract.py`：从 ISO（或 XP 的 Windows 目录）只取出需要的文件到 `.local/assets`，不联网；语言由 `extract/media.py`
  从 shell32.dll 资源的语言判断（日文版取日文字体、简体中文版取宋体 `simsun.ttc` 与控制台点阵 `vga936.fon`、英文版取英文字体）。
- `wine/build.sh`：取回 Wine 11.0、打上 `wine/patches/`、编译。
- `prefix/create.py`、`prefix/stage.py`：建 XP 用的 prefix，注册 XP 的 COM、写入自己验证过的注册表值（按语言选择字体与代码页：
  日文版 MS UI Gothic/控制台 MS ゴシック，简体中文版宋体/新宋体 8×16，英文版 Tahoma/Microsoft Sans Serif/Terminal）。
- `bridge-wm/build.sh`：编译 `proxy.exe` 等 Win32 辅助程序到 `.local/bridge`。

**XP 外壳怎么跑起来**：`explorer.exe` 以 `C:\xp\xp-explorer.exe` 存在，与 Wine 自己的 explorer 是不同进程；
XP 的 shell32/browseui/shdocvw/shlwapi/comctl32 通过 AppDefaults **只给这个进程**用（native），其他程序仍用 Wine 的 DLL；
补丁大多只在 `WIN4WINE_EXTERNAL_SHELL` 存在时生效；prefix 是 WoW64，XP 的 DLL 放 `syswow64`，COM 注册只写 32 位一侧。

**WM（`bridge-wm/wm.py`）**：不自己画边框、不 reparent。用补丁 0019 加的 `_WIN4WINE_CLASS`/`_WIN4WINE_HWND` 区分
XP 桌面（Progman）、任务栏、开始菜单、代理边框、普通 Wine 窗口与 Linux 窗口；桌面放最底、任务栏与开始菜单在上、
Win32 最前窗口在其上；由任务栏位置算工作区 `_NET_WORKAREA`；转发 Alt+Tab / Windows 键 / Ctrl+Esc；用 XSETTINGS 藏起
GTK 的标题栏按钮；对 Wine 窗口做 Composite 自动重定向（`session.json` 的 `composite`，默认开；Linux 窗口不重定向，
因为它的边框频繁改变尺寸）。WM 不轮询：由 X 事件、IPC 目录的 inotify、信号唤醒，只写内容变化的文件。

**proxy（`bridge-wm/proxy.c`）**：Wine 侧的 Win32 进程，为每个 Linux 窗口建一个 `Win4WineProxy` 代理窗口。
XP 的任务栏把它当普通 Win32 窗口（标题、最小化状态、图标、shell hook 通知）；代理带 `WS_OVERLAPPEDWINDOW`，
由 Wine 用 XP（Luna）主题绘制边框，WM 再把边框叠在 Linux 窗口正下方——于是就"看到"Linux 窗口带 XP 边框。
边框按钮与任务栏操作通过命令文件转给 WM；shell hook 靠补丁 0020 记录的注册目标转发；同样事件驱动。

**IPC 文件**（`$XDG_RUNTIME_DIR` 下 0700 的目录，先写临时文件再 rename，只在内容变化时写）：
`windows.tsv`（WM→proxy 的窗口列表）、`geometry.tsv`（窗口内容位置）、`frames.tsv`（边框余量）、
`PID-*.cmd`（proxy→WM 的 activate/minimize/close/maximize/restore/geometry/switch）、`switch.tsv`（Alt+Tab 状态）、
`shellkeys.tsv`（Linux 窗口上的 Win/Ctrl+Esc）、`icon-*.argb`（窗口图标，格式 `W4I1` + straight ARGB）、
`tray.tsv`/`tray-*.argb`/`menu-*.tsv`（托盘图标与菜单）、`*.tray`（托盘点击与选择）、`proxy.lock`（proxy 存活，LockFile/F_GETLK）。

**Linux 窗口的 XP 边框**：WM 写 `windows.tsv` → proxy 建代理窗口并把边框余量写 `frames.tsv` → WM 按 `_WIN4WINE_HWND`
把两者配对并叠加（余量也写进 `_NET_FRAME_EXTENTS`）→ 拖动时 WM 在同一次处理里同时移动边框与 Linux 窗口（避免空出黑块），
proxy 在拖动过程中不发 `geometry` 命令。最大化用 `ShowWindow(SW_MAXIMIZE)`（外框按 XP 一样出屏），最小化一起隐藏；
边框只在 Linux 窗口持有 X 键盘时显示为活动外观（用 `WM_NCACTIVATE`，因为 Wine 会拒绝未获得输入的进程的
`SetForegroundWindow`）；对话框（transient）不加边框、不上任务栏。

**图标**：从 `_NET_WM_ICON`（没有就用 `WM_HINTS` 的 pixmap）读，写成 `icon-XID.argb`；跨进程读图标需要补丁 0022
（opt-in 时把图标像素注册到 wineserver 供读取）。

**键盘**：Alt+Tab 由 WM 抓键盘并写 `switch.tsv`，proxy 画 XP 的切换窗口，确定后送 `switch XID`；
Windows 键与 Ctrl+Esc 在 Wine 窗口上有焦点时直接交给 Wine，在 Linux 窗口上时由 WM 收下写 `shellkeys.tsv`，
proxy 用 `SendInput` 送进 Wine。窗口吸附（`window_snap`）默认关闭，见 `original-features.md`。

**托盘（`bridge-wm/tray.py`）**：StatusNotifierItem 走会话总线（`org.kde.StatusNotifierWatcher`，菜单用 `com.canonical.dbusmenu`）；
XEmbed 走 `_NET_SYSTEM_TRAY_S0`（离屏窗口 + Composite 取图，点击用 XTest 送真实按压）。proxy 用 `Shell_NotifyIconW`
把图标放进 XP 通知区域（补丁 0028 负责让 XP 的 Explorer 读懂格式）。

**会话（`session/supervisor.py`）**：启动顺序为 XP 登录画面 → WM → tray.py → fcitx5 → prefix 更新 → Linux 应用开始菜单 →
proxy → XP 外壳。启动 fcitx5 之前先把输入法写进它的配置（缺了就补，已有不动）。任务栏出现即认为启动成功；
外壳或 proxy 崩溃会重启 Wine，120 秒内失败 3 次则退回到不带 XP DLL 的 prefix + Wine 自带 explorer + xterm（恢复用）。
注销/关机走补丁 0018 的请求文件；关机命令取 `session.json` 的 `shutdown_command`/`reboot_command`。
日志在 `~/.local/state/win4wine/`，每次启动把上次的重命名为 `*.log.1`。

**Linux 应用开始菜单（`session/linux_apps.py`）**：读 XDG 的 desktop entry，为每个可见应用在 prefix 的公共 Programs 里
建快捷方式（名字加 ` (Linux)`，按 Categories 归到 XP 的「附件」「 Internet」等文件夹），指向 `linux-launch.exe`；
记录在 `C:\xp\linux-apps\state.json`，用 inotify 跟随软件的安装与卸载。

**登录界面（`greeter/`）**：`xp-greeter.py` 是 LightDM 的 greeter，只把 XP 的 logonui.exe/msgina.dll/winlogon.exe
当作 PE 资源读取（不执行其中的代码），`greeter/welcome.py` 用 cairo 与 Pango 绘制；缺文件时退回 lightdm-gtk-greeter。
登录中/注销中的 XP 画面由 `greeter/xp-splash.py` 在会话内显示；XP 光标由 `greeter/cursors.py` 从 user32.dll 生成
X 光标主题 `win4wine-xp`。

**DPI**：与 XP 一样由 prefix 的 `HKCU\Control Panel\Desktop\LogPixels` 决定（`prefix/display.py --dpi`），补丁 0051 让外壳
成为 system DPI aware；supervisor 把同一个值交给 Linux 应用（`session/dpi.py`：`Xft.dpi`、fontconfig dpi、
`XCURSOR_SIZE`，192 以上再加 GTK 的 `GDK_SCALE`）；欢迎屏幕的 DPI 在 `/etc/win4wine/greeter.json`。

**字体平滑（ClearType）**：`HKCU\Control Panel\Desktop` 的 `FontSmoothing` 与 `FontSmoothingType`（1/2）决定，
默认与 XP 一样是「标准」；ClearType 时补丁 0052 按 XP 的方式绘制文字（带嵌入点阵的字号保持黑白，如 MS UI Gothic ≤22 像素），
Linux 应用也走 RGB 子像素（`Xft.rgba`）。

**Wayland 会话（`wayland/`）**：`wayland/session.sh` 建 IPC 目录并启动打过补丁的 labwc（用 `-S` 在其中运行 supervisor），
labwc 承担 WM 的角色，IPC 文件与 proxy.exe 与 X11 会话完全相同；Wine 与 XP 外壳仍是 Xwayland 的 X11 客户端。

## [design.md](design.md) — 设计要点

**四条底线**：不改动来自 Windows 的二进制；不反汇编微软的二进制（行为靠公开 API 调用记录、公开文档、跑在真机 XP 上的
自制探针来确认）；仓库里不放微软的文件（连像素与字符串也不放，只放量出来的规则和数值，运行时从用户自己的 ISO 读）；
不影响普通 Wine 应用的既有行为（给外壳的改动一律 opt-in）。

**哪些部件用 XP 的**：`explorer.exe` 用 XP（任务栏、开始菜单、桌面的本体）；shell32/browseui/shdocvw/shlwapi 用 XP
（Wine 的 browseui 缺 XP 外壳要用的功能，必须同代搭配）；browselc/shdoclc/linkinfo/msgina/mydocs 用 XP 的资源或部分功能
（日文菜单文字、快捷方式、注销与关机对话框、图标）；comctl32 用 XP 的 v6（**只在外壳进程里**，否则 XP 的 toolbar 不工作；
按钮等仍用 Wine 带主题的实现）；uxtheme、kernel32/ntdll/user32/gdi32 等用 Wine；mshtml、mscoree 关闭（不做 IE 与 .NET）。
只在 `xp-explorer.exe` 进程里用 XP 的 DLL，理由是这样就不会让 Wine 自身的管理进程与恢复路径也依赖 XP 的 DLL。

**prefix**：为外壳建专用 prefix，只写自己验证过的 COM 注册与注册表值（`prefix/registry.py` 的 registry profile），
不整份迁移真实 XP 的注册表；Wine 换版本会重写注册，所以 `prefix/stage.py` 看更新标记重装 XP 的 DLL 与注册；prefix 为 WoW64。

**与 Linux 应用集成（bridge）**：WM 不画边框——XP 边框由 proxy 建的 Win32 窗口交给 Wine 与用户的主题来画（而不是自己模仿）；
XP 任务栏把 Linux 窗口当 Win32 窗口，所以 proxy 为每个窗口准备代理；WM 与 proxy 只在事件上醒来（此前轮询导致边框跟随滞后，
改成 X 事件 + inotify + WinEvent 后延迟约 11 ms）；WM 保持 Python 实现（换语言不解决延迟来源）；边框与 Linux 窗口由 WM 在同一次处理里
一起移动；Wine 窗口内容交给 X 服务器保存（Composite，不做带阴影透明的合成，省虚拟机 CPU）；虚拟机上让 X 先在内存里绘制（ShadowFB）。

**中日文输入**：用 fcitx5（日文 Mozc、简体中文版拼音），与 Wine 走 XIM、与 GTK/Qt 走各自的输入模块；启动与环境变量由
supervisor 管理，不用 im-config（避免影响其他会话）；输入法本身由 supervisor 写进 fcitx5 配置（fcitx5 自己不会加输入法）；
输入模式显示在 XP 通知区域；候选文字按 XP IME 的方式显示在输入框内（XIM 的 on-the-spot，`UseOnTheSpot=True`，
补丁 补丁 0029 让候选窗口标在插入符下方）。

**登录界面**：自己用 Python + GTK3 + cairo + Pango 画；不运行 XP 的 logonui.exe/msgina.dll，只读资源——因为在处理密码的地方
不跑微软的代码；文字按 GDI 的排版方式绘制，布局对齐真机 XP 上量到的数值。

**DPI**：与 XP 一样只有一个 `LogPixels`；XP 没有 DPI 虚拟化，各程序自己按 DPI 绘制，所以关掉 Wine 的图像放大（会模糊），
并让外壳进程 system DPI aware；图标与光标大小等 XP 自己的规则按真机 96〜216 DPI 测量；Linux 应用通过 `Xft.dpi` 等拿到同一个 DPI。

**Wayland**：X11 无法做到每显示器缩放，所以做了第二个会话，用改过的 labwc 作 compositor（仍在试验）；不自己写 compositor
而是改 labwc，是因为 Xwayland、输出、输入、协议都能交给 wlroots 与 labwc，只补 bridge-wm 的角色（XP 边框、层叠顺序、IPC）；
XP 外壳与 Wine 保持为 Xwayland 客户端（XP 的 Explorer 自己决定窗口坐标，Wayland 客户端模型复现不了）；XP 外壳按输出的
1:1 像素显示、不让 compositor 放大（否则模糊）；X11 会话不变。

**和真机 XP 对照**：参考机是日文版 SP3 与英文版 SP3 的虚拟机，比较公开 API 的数值与屏幕像素；简体中文版还没有参考机，
所以与它有关的数值（经典主题宋体 9 点、控制台新宋体 8×16、欢迎屏幕布局）来自 XP 的默认值与日文版的实测，没有真机测量；
只把尺寸、颜色、规则等数值放进仓库，不放 XP 的像素；测试在 `tests/`，同一个探针在真机 XP 和 Wine 上都跑一遍再比较。

## [patches.md](patches.md) — Wine 的 126 个补丁

`wine/patches/` 里是 126 个补丁，按顺序打在未改动的 Wine 11.0 上（版本见 `wine/upstream.env`），由 `wine/build.sh` 调用
`wine/patch-state.py` 应用（也能给只打了一部分的源码补上缺的部分）。

**opt-in**：给 XP 外壳的改动只在设置了 `WIN4WINE_EXTERNAL_SHELL` 的进程里生效（会话与 prefix 辅助工具会设置它），
所以同一个 Wine 跑普通 Windows 程序时行为不变。判断集中在补丁 0013 加入的 `include/wine/win4wine.h` 的 `win4wine_shell()`。
只有文档化行为的修正等少数补丁不需要 opt-in（逐条的列表见原文的 opt-in 栏）。另外还有两个更细的开关：
`WIN4WINE_SESSION_REQUEST`（注销/关机请求）与 `WIN4WINE_FOLDER_SHELL`（只有 ReactOS 会话设置）；
外壳的种类由 `WIN4WINE_SHELL` 区分（`reactos`、`win7`，不设就是 XP），补丁 0013 的 `win4wine_shell()` 统一判断。

**分组**：0001〜0008 Wine 通用修正；0009〜0012 XP 部件调用的 API；0013〜0018 外部外壳的基础；0019〜0035 与 bridge
（WM/proxy）协作、X11 显示与中文/日文输入；0036〜0042 窗口布局、UI 状态、菜单、WM_PRINT、壁纸；0043〜0053 文字与
GDI 像素；0054〜0057 控件；0058〜0073 uxtheme；0074 Wayland 会话；0075〜0078 ReactOS 外壳；0079〜0097 Windows 7
外壳调用的 API 与 Wine 修正；0098〜0104 Vista/7 主题（版本 4 样式）；0105〜0112 Windows 7 外壳的会话；0113〜0120
Windows 7 的外观；0121〜0126 XP 与 ReactOS 外壳的细节。
文档还写了排列顺序的思路、补丁的书写格式、以及新增补丁的步骤；旧编号对照见 `history/patch-history.md`。

与本仓库中文/日文支持相关的几个（新编号）：**0044**（CJK 折行与文本测量，代码页从 932 推广到 932 与 936）、**0049**
（控制台双字节字符占两个单元格）、**0065/0066**（uxtheme 的日文字体 profile 与本地化主题值，从用户自己的 msstyles 取字体）、
**0052**（ClearType）。

## [development.md](development.md) — 开发

**仓库结构**：`wine/`（Wine 版本、补丁、构建、补丁应用）、`bridge-wm/`（WM、Win32 代理、托盘、调查工具）、
`wayland/`（labwc/wlroots 版本、补丁、构建、配置、启动）、`session/`（会话看守、输入法、Linux 应用菜单、会话注册）、
`prefix/`（prefix 创建与更新、外观与 DPI、注册表 profile）、`extract/`（从 ISO 取文件、判断语言）、`greeter/`（欢迎屏幕、
登录/注销画面、XP 光标）、`scripts/`（一键安装、chroot、提交前检查）、`tests/`（集成测试与 C 探针）、
`localres/` 与 `.local/`（ISO、取出的文件、prefix、构建、日志、截图，**不受 Git 管理**）。

**Git**：`git config core.hooksPath .githooks`；提交前检查 `scripts/check-staged.py` 会拒绝 `localres/`、`.local/` 的文件、
PE、字体、注册表 hive、含 NUL 的二进制（连 `git add -f` 也拒绝），唯一例外是 `docs/images/` 下的 PNG（README 截图）。

**chroot 开发环境**：宿主需要 git、sudo、debootstrap 与 ≥15 GB 空间；`scripts/debian.sh init`、`scripts/debian.sh run /bin/bash`，
chroot 里的 `/work` 就是本仓库，接着按文档里的顺序构建 Wine、bridge、探针，取文件、起 Xvfb、建 prefix。
Wine 用新的 WoW64 方式（`--enable-archs=i386,x86_64`），因此**不需要 i386 的宿主库**；
改了补丁必须**从 configure 重新构建**（复用构建目录会留下旧对象，看不出遗漏）；`extract.py` 与 `create.py` 在输出已存在时会停下。

**补丁检查**：`python3 wine/patch-state.py SOURCE wine/patches --apply`；labwc 与 wlroots 同理；
不要把 `.local/wine-upstream` 的直接修改当成果，必须整理成补丁。

**测试**：`wm_integration`（WM 单体）、`wine_restore`、`bridge_integration`（WM + proxy + XP 外壳）、
`native_frames`（Linux 窗口的 XP 边框）、`xterm_redraw`/`wm_latency`（重绘与延迟）、`wayland_compositor`、
`tray_host`、`linux_apps`、`recovery_integration`、`reactos_session`、`reactos_greeter`、`greeter_layout`/`cursor_theme`、
`patch_state`/`font_*`/`workarea`/`dpi_settings`、`xp_language`（三种语言的注册表差异与文件判断）、
`input_method`（把输入法写进 fcitx5 配置）；`tests/*.c` 是公开 API 探针，同一份在 Wine 与真机 XP 上跑并比较数值。

**手工安装与更新**：不用 `setup.sh` 时按同样步骤准备 Wine、bridge、取出的文件与 prefix，再写
`~/.config/win4wine/session.json`，并用 `session/install.sh`、`greeter/install.sh` 注册；
更新已有环境时先停会话，换 Wine，跑 `prefix/stage.py --wine "$WINE" "$WINEPREFIX"`，再重建 bridge。
外观与 DPI 用 `prefix/display.py --theme luna|classic --menu xp|classic --dpi N` 改（`--theme` 会连壁纸一起换：
classic 无壁纸，luna 是 Bliss）。还写了从真实 XP 的 hive 只读导入有限值（CLSID 与 NLS）的方法。

## [reactos.md](reactos.md) — ReactOS 外壳

没有 XP 的 ISO 也能试：用 ReactOS 的 `explorer.exe` 与周边 DLL 代替 XP 的外壳，外观是 ReactOS 的「经典」
（接近 Windows 2000 的「Windows 标准」配色），共用同一套 Wine、bridge 与 supervisor。

```sh
scripts/setup.sh --shell reactos --lang zh    # 日文用 --lang ja，英文用 --lang en
```

登录界面选择会话「**ReactOS Shell (Wine, X11)**」；如果也装了 XP 外壳，两个会话会并列出现。

**取到的文件**：`extract/reactos.py` 从官方 release 下载 `ReactOS-0.4.16-i386.zip`（校验 SHA-256，zip 留在 `.local/dl`，
也可以 `--zip` 用本地压缩包），只取出 `explorer.exe`、shell32/browseui/shdocvw/shlwapi/comctl32(5 与 6)/zipfldr、msgina、
字体（tahoma/tahomabd/Marlett/DroidSansFallback）、user32（只读光标）、Nebula.bmp 与可选主题的 msstyles；
用 32 位的 i386 版（与 XP 外壳同一条 WoW64 的 32 位路径）。许可方面 ReactOS 整体是 GPL-2.0-or-later，
多数 DLL 是 LGPL-2.1-or-later；仓库不分发这些文件，只从官方 release 取用。

**prefix（`prefix/reactos.py`）**：同为 WoW64；ReactOS 的 DLL 放 `syswow64`，整个 prefix 保持 Wine 的 DLL（`builtin`），
只让 `ros-explorer.exe` 通过 AppDefaults 用 `native`；Common Controls 6 与 Wine 同版本号，所以替换 `winsxs` 里的文件；
`HKLM\...\Winlogon` 的 `Shell` 写 `ros-explorer.exe`；COM 注册交给 ReactOS 自己的 `DllRegisterServer`；
文件夹的「打开」「资源管理器」也交给它；中/日文用 FontLink 把 Tahoma 等接到 DroidSansFallback（Wine 会重建 FontLink，
所以每次会话前的 `stage` 都要重写），另外装 `fonts-noto-cjk` 供 Linux 应用使用（外壳字体设为 Noto Sans CJK JP/SC）；
开始菜单的「启动」「管理工具」用对应语言的名字（中文是「启动」「管理工具」）；外观上关掉主题用 ReactOS 的经典配色与尺寸、
Nebula 壁纸、隐藏 Wine 命名空间里的 Internet 与「/」、不显示「帮助和支持」。

**会话**：`reactos-session.json` 里 `"shell": "reactos"`，supervisor 启动 `C:\ros\ros-explorer.exe`（不显示 XP 欢迎屏幕）、
设置 `WIN4WINE_FOLDER_SHELL`（补丁 0075 让 Wine 的 explorer 把控制面板/搜索/Win+E 之类的启动转给 ReactOS 的 shell）
与 `WIN4WINE_SWITCHER=reactos`（Alt+Tab 用 ReactOS 的样式绘制）。bridge 与面向 XP 的补丁在这里同样有效。

**可选**：ReactOS 的 `lautus`/`lunar`/`mizu` 主题（`--theme`），以及 ReactOS 的登录界面
（`greeter/reactos-greeter.py` 与 `greeter/rosdialog.py`，按 msgina.dll 对话框的布局与文案绘制，认证仍由 LightDM/PAM 完成）。

**与 XP 外壳的差别与限制**：外观接近但不做逐像素对齐（例如粗体宽度：ReactOS 合成粗体，Wine 用 `tahomabd.ttf`）；
没有「帮助和支持」、网络与打印机设置；文件夹窗口的列表里会出现 Wine 的 Unix 根（`/`，桌面上不显示）；
Linux 应用没有 `Path` 时从 prefix 的 `C:\windows` 启动；Wayland 会话同样是试验性的。测试是 `tests/reactos_session.py`。

## [wayland.md](wayland.md) — Wayland 会话（试验中）

第二个会话「XP Shell (Wine, Wayland)」：用打过补丁的 labwc（wlroots 的 compositor）承担 `bridge-wm` 的窗管理角色，
X11 会话完全保留、行为不变。**比 X11 会话更实验**，确认范围更小，下面有已知限制。

**能得到什么**：每台显示器单独缩放（100% 与 150% 可同时用，X11 只有整屏一个 DPI）；Linux 应用支持分数缩放
（GTK/Qt/Chromium/Firefox 的 125%、150%）；XP 外壳与 Wine 仍是 Xwayland 客户端，按 DPI（`LogPixels`）绘制后
按输出的 1:1 像素显示、不被放大，所以不模糊。XP 本身没有每显示器 DPI，外壳的 DPI 只有一个，设成你常用输出的缩放
（缩放 1.5 就用 144）。

**用法**：`scripts/setup.sh --media ISO --wayland` 会构建 labwc 与 wlroots 并注册会话；输出缩放与位置写在 `session.json`
的 `outputs`，Win32 主显示器用 `primary`（输出名）；`mode` 也可以写，输出名用 `wlr-randr` 查看。

**部件**：`wayland/build.sh` 把 wlroots 0.18.2 与 libsfdo 作为 subproject 静态编进 labwc 0.8.3；
wlroots 的 0001（把 scene 的 surface 按 1/缩放放置、必要时裁剪，从而在该输出上逐像素绘制）、0002（XWM 保留
`_NET_WM_STATE_ABOVE`）；labwc 的 0001（`xwayland-scale.c` 与承担 bridge-wm 角色的 `win4wine.c`）；
`wayland/labwc/` 的配置；`wayland/session.sh` 与 `session/supervisor.py` 的分工；Wine 的 0027（Xwayland 下读不到根窗口，
XP 的阴影改画成带透明度的黑窗口）与 0062（注销的灰屏由各窗口内容拼出）。

**`xwayland-scale.c`**：给 Xwayland 看自制的 xdg-output，把各输出按像素并排（"X 空间"），主显示器放在最前（X 的 0,0）
并设为 Xwayland RandR 的 primary；X 窗口按 1/缩放放置（X 的一个像素对应输出的一个像素），跨不同缩放输出的窗口按输出裁剪拼接；
窗口位置/尺寸请求、size hints、工作区（`_NET_WORKAREA` 与每输出的 `_GTK_WORKAREAS_D0`）、按住拖出窗口的指针位置都在两套坐标间换算。

**`win4wine.c`（bridge-wm 的角色，只在有 `WIN4WINE_IPC` 时运行）**：与 proxy.exe 的 IPC 与 X11 会话一致；
给 Linux 窗口（Wayland 窗口与 Wine 以外的 X11 窗口）加 XP 边框并绑定移动/缩放/最大化/最小化/关闭/拖动；
Wine 窗口的类别用 labwc 自己的 X 连接读取，桌面放最下、任务栏/开始菜单/最前窗口（及其所属窗口）在上，X 层叠顺序也同步
（Xwayland 用它决定指针下是哪个窗口）；Alt+Tab、Windows 键、Ctrl+Esc 照旧转发；激活策略贴合 Wine（Wine 选下一个窗口、
按 `_NET_WM_USER_TIME` 为 0 与全屏最前窗口不激活、Wine 窗口持键盘时直接给 X 焦点、被 wlroots 掉焦点后重给等）；
不给 Xwayland 看 relative pointer（否则第一次点击会落错地方）；窗口放大时给 X 窗口发 frame callback。

**已验证**：嵌套 labwc（chroot、两个输出、缩放 1 与 1.5、144 DPI）与测试虚拟机的真实输出（DRM、1280×800、
缩放 1/96 DPI 与 1.5/144 DPI），覆盖 XP 外壳（桌面、任务栏、开始菜单、菜单阴影、运行对话框、我的电脑、注销对话框与灰屏、
最大化/还原/拖动、1.5 下不模糊）、Linux 窗口（foot/mousepad/xterm 的 XP 边框、任务栏按钮、图标、拖动、最大化、
最小化与恢复、关闭、活动配色）、键盘、日文输入（text-input-v3 与 XIM）、双输出、会话结束、X11 会话回归；
自动测试是 `tests/wayland_compositor.py`（不用 Wine）。

**与 X11 的差别与遗留**：注销的灰屏不含 Wayland 窗口（那些不是 X 窗口）；非主显示器只画背景色；
主显示器在最前，所以往它左侧输出拖动 Wine 窗口的途中，超出部分会按原输出的缩放（偏小）绘制；
非整数缩放的输出上，X 窗口之间的位置可能差一个输出像素（布局坐标是整数）；带父窗口的 Linux 对话框不加 XP 边框，
而用 labwc 的边框（配色接近 XP Luna）；GTK4/libadwaita 应用自绘客户端装饰，估计会与 XP 边框重叠（尚未验证）；
labwc 的补丁是一整块大的，不像 Wine 的补丁那样按目的拆分。

## [original-features.md](original-features.md) — 原创功能（XP 没有的）

原则是照 XP 复现外观与行为，但也加了几个 XP 没有的功能：全部**默认关闭**，只在 `session.json`
（ReactOS 外壳是 `reactos-session.json`）里显式打开时才生效；关闭时行为与 XP 相同。

**窗口吸附（`window_snap`）**：像 Windows 7 的 Aero Snap。`"window_snap": true` 后下次登录生效，
XP 外壳与 ReactOS 外壳、X11 与 Wayland 会话都能用，对 Wine 窗口与带 XP 边框的 Linux 窗口同样有效。

| 操作 | 行为 |
|---|---|
| Win+← / Win+→ | 把前台窗口吸附到工作区的左/右半；按反方向回到吸附前的状态；同方向再按一次移到相邻显示器的近侧半屏 |
| Win+↑ | 最大化 |
| Win+↓ | 最大化时还原；已吸附时回到吸附前；否则最小化 |
| 把标题栏拖到屏幕左/右边缘 | 吸附到那一半；停在边缘时用与 XP 拖动框相同的网纹提示去向 |
| 把标题栏拖到屏幕上缘 | 最大化 |
| 拖动已吸附窗口的标题栏 | 回到吸附前的大小并继续拖动 |
| 拖动已最大化窗口的标题栏 | 稍一移动就还原并继续拖动（XP 里最大化的窗口不能拖动） |

**机制**：proxy 用 `RegisterHotKey` 接 Win+方向键；X11 下标题栏拖动由 Wine 的移动循环完成（补丁 0032 让它发
`EVENT_SYSTEM_MOVESIZESTART/END`），proxy 据此画去向框并在松手时吸附，剥离吸附与还原最大化也要在 Wine 的循环里做；
Wayland 下交给 labwc 自己的 snapping，proxy 只把 Win+方向键转给它，并通过 `tiled.tsv`/`snapping.tsv` 了解状态。

**限制**：属试作；"移到相邻显示器"只在一台显示器的环境验证过；改变已吸附窗口的大小会取消吸附（与 Windows 7 相同）。

## [windows7.md](windows7.md) — Windows 7 外壳（试验中）

上游新加的第三个外壳：用 **Windows 7 SP1 x64** 的 `explorer.exe` 与周边 DLL 代替 XP 的，外观是 Windows 7 的
Basic 主题（还没有 Aero 玻璃效果）。开始菜单、任务栏、通知区域、文件夹窗口都能用，Linux 应用在任务栏上按程序分组；
只验证过日文版的 Windows 7。与 XP / ReactOS 的外壳并列，各有自己的会话（「Windows 7 Shell (Wine, X11)」）。

要点（细节见原文）：

- **从 Windows 7 的 ISO 取文件**：外壳的 `explorer.exe`、它调用的 DLL（`shell32`、`comctl32` 等）、字体、
  光标与登录画面所需的资源；取的清单在 `extract/manifests.py`，判断语言与版本在 `extract/win7.py`。
- **注册表**：不从真机整份迁移，而是从 WinSxS 里的组件清单（manifest）生成需要的注册项。
- **prefix（`prefix/win7.py`）**：与 XP / ReactOS 一样是 WoW64 的 prefix，`syswow64` 放 Windows 7 的 DLL，
  只给该用它们的程序用（哪些程序用哪些 DLL 在原文「どのプログラムが Windows 7 の DLL を使うか」一节）。
- **光标与登录画面**：从 Windows 7 的 `user32.dll` 生成 X 光标主题；`greeter/w7logon.py` 绘制 Windows 7 的登录画面。
- **会话**：`--shell win7 --media <Windows 7 的 ISO>`，会话名「Windows 7 Shell (Wine, X11)」；
  语言取媒体本身的（`--lang` 只对 ReactOS 外壳有效）。
- **补丁**：Windows 7 相关的补丁占 0079〜0120（外壳调用的 API、Vista/7 主题的版本 4 样式、会话与外观），
  这些只在 `WIN4WINE_EXTERNAL_SHELL` 且 `WIN4WINE_SHELL=win7` 时生效。
- **限制**：仍是试验阶段，只在日文版上验证过；没有 Aero 玻璃；原文「制限」一节列了其余部分。

## 记录（history/）

开发过程中留下的记录，保持写作当时的文体，最新说明以上面的文档为准。

| 文档 | 内容 |
|---|---|
| [history/investigations.md](history/investigations.md) | 调查与判断的记录（按进行顺序）：缺陷原因、真机 XP 上量到的数值、没有采用的做法 |
| [history/patch-history.md](history/patch-history.md) | 补丁的新增与编号重排经过，以及与旧编号的对照表 |
| [history/wayland-study.md](history/wayland-study.md) | 各 compositor 的比较与最初试作的结果 |

记录里的「阶段 N」指 2026-10 的开发阶段 1〜11。
