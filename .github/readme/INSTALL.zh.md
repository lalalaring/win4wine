# 安装

[日本語](../../INSTALL.md) · [English](INSTALL.en.md) · **简体中文**

> [!WARNING]
> - Windows XP 的 ISO 请自己获取，使用请自担风险（见 [README 的「注意」](README.zh.md#注意)）。
> - 这是一个实验项目。只在**全新安装的 Debian 13** 上用 `scripts/setup.sh` 装好后确认过；在已经使用中的系统上安装
>   没有试过。脚本会安装软件包，也可能改动 LightDM 的设置（见「2. 脚本做了什么」）。建议用虚拟机或一台试验用的机器。

## 需要的东西

- 一台 **Debian 13（trixie）amd64** 机器（实体机或虚拟机都行）。不需要桌面环境。
- 一张 **Windows XP Professional SP3（x86）的 ISO**（或者一份 XP 的 Windows 目录），**日文版、简体中文版或英文版**；
  是哪一种会从 ISO 判断出来。
- 网络（用于安装软件包和下载 Wine 源码），以及**约 40 GB 空闲空间**（Wine 的源码和编译产物）。
- 一个能使用 sudo 的用户。

## 1. 一次装好

用将要使用 XP 外壳的那个用户来执行（不要用 root）。脚本会通过 sudo 安装软件包、写系统文件。

```sh
git clone https://github.com/mikuta0407/win4wine.git ~/win4wine
cd ~/win4wine
scripts/setup.sh --media /path/to/WindowsXPSP3.iso --greeter
```

| 选项 | 含义 |
|---|---|
| `--media PATH` | XP 的 ISO 或 Windows 目录（必需） |
| `--greeter` | 把登录界面变成 XP 的欢迎屏幕（LightDM 会成为显示管理器） |
| `--no-greeter` | 保留当前的登录界面（默认）；只安装 XP 的鼠标指针 |
| `--theme luna\|classic` | `luna`（默认：XP 的「Windows XP」主题、XP 样式的开始菜单、Bliss 壁纸）或 `classic`（XP 的「Windows 经典」主题、经典开始菜单、无壁纸） |
| `--dpi N` | 屏幕的 DPI（XP 的「DPI 设置」）：96（正常）、120（大尺寸）或者 19–480 之间的任意值；对 XP 外壳、其会话里的 Linux 应用以及欢迎屏幕生效 |
| `--wayland` | 同时安装 Wayland 会话「XP Shell (Wine, Wayland)」（会编译打过补丁的 labwc 与 wlroots）：支持每台显示器单独缩放、125% 或 150% 这类比例、Wayland 应用也有 XP 的边框；配合 `--greeter` 时它成为欢迎屏幕的默认会话。**比 X11 会话更实验**（确认的范围更小，有已知限制；[docs/wayland.md](../../docs/wayland.md)） |
| `--wine-install DIR` | 使用在另一台 Debian 13 上编译好的 Wine（把它的 `.local/wine-install` 复制过来），不在本机编译 |
| `--jobs N` | 编译 Wine 时的并行任务数（默认是 CPU 核数） |
| `--no-shadowfb` | 在虚拟机上保留 X 现有的绘制方式（「2. 脚本做了什么」的第 8 步） |
| `--shell reactos` | 安装 ReactOS 的外壳而不是 XP 的（不需要 `--media`；见下面的「ReactOS 的外壳」） |
| `--lang ja\|zh\|en` | ReactOS 外壳的语言（默认：当前区域是日文用 `ja`，是中文用 `zh`，否则用 `en`） |

失败了就再跑一遍：已经做完的步骤会跳过。编译出来的东西都放在这个 checkout 的 `.local/` 里。

## 2. 脚本做了什么

1. 安装软件包（Wine 的编译依赖、Xorg、python3-xlib、fcitx5 + Mozc + 拼音等），如果系统里没有显示管理器就装上 LightDM。
2. 取回 Wine 11.0，打上补丁并编译（`.local/wine-install`；4 核大约一个半小时；日志在 `.local/wine-build.log`）。
   编译过程中只显示当前步骤、已编译文件的数量与比例、剩余时间估计和最后几行输出。
3. 编译 bridge（窗口管理器与代理窗口）。
4. 从 ISO 里只取出需要的文件（`.local/assets`），并判断这个 XP 的语言（日文、简体中文或英文）。
5. 生成对应语言的区域（`ja_JP.UTF-8`、`zh_CN.UTF-8` 或 `en_US.UTF-8`）。
6. 创建 XP 的 prefix（`.local/prefix`），带上那个 XP 的字体和设置（见下面的「英文版 XP」「简体中文版 XP」）。
7. 写 `~/.config/win4wine/session.json`，并注册会话「XP Shell (Wine, X11)」。
8. 在虚拟显卡读回较慢的虚拟机上（QEMU 的标准 VGA、Hyper-V、VirtualBox 的 VBoxVGA 和 VBoxSVGA），先让 X 在内存里绘制
   （`/etc/X11/xorg.conf.d/20-win4wine-shadowfb.conf`，modesetting 的 ShadowFB）。移动窗口时 X 服务器的开销大约降到一半以下。
   它对该机器的所有 X 会话生效。`--no-shadowfb` 跳过；virtio-gpu 和 VMware 的虚拟显卡保持原样。
9. 带 `--greeter` 时，安装 XP 的欢迎屏幕。
10. 带 `--wayland` 时，用各自的补丁编译 labwc 和 wlroots（`.local/labwc-install`；日志在 `.local/labwc-build.log`），
    并额外注册会话「XP Shell (Wine, Wayland)」。

## 英文版 XP

把英文版 XP SP3 的 ISO 交给 `--media`，安装过程就会按英文版 XP 走；选项完全一样。

- 区域是 `en_US.UTF-8`。不会配置日文输入（fcitx5）（`session.json` 里没有 `input_method`）。
- 字体是英文版 XP 的：Luna 下标题栏用 Trebuchet MS、其余用 Tahoma 8 pt，开始按钮和开始菜单里的用户名用
  Franklin Gothic Medium；经典主题下是 Tahoma 8 pt。Linux 应用拿到的是 Tahoma。
- 命令提示符使用点阵字体 Terminal（8×12），和英文版 XP 一样。
- 欢迎屏幕用英文版 XP 的文字与字体（Arial、Tahoma、Franklin Gothic Medium）。
- 字体文件来自 ISO（英文版 ISO 里的 Franklin Gothic、Arial 粗体与斜体、Microsoft Sans Serif 和 Terminal）。
- 不打算在同一台机器上混装日文、简体中文和英文版 XP。一台机器只有一个欢迎屏幕：最后用 `--greeter` 装的那个。
- 日文、简体中文、英文以外的 XP 会按英文版 XP 的方式安装（区域用 `en_US.UTF-8`）。完全没有试过，所以 `setup.sh` 会给出警告（然后继续）。

从日文版 XP 换到英文版 XP（或换回来），把取出的文件和 prefix 移开，再用另一个 ISO 跑一遍 `setup.sh`：
只要它们还在，`setup.sh` 就不会重新生成。Wine 的编译会被复用，`setup.sh` 会更新 `session.json` 里的区域和输入法。
注销后，在文本控制台（Ctrl+Alt+F2）或者通过 SSH：

```sh
cd ~/win4wine
mv .local/assets .local/assets-ja
mv .local/prefix .local/prefix-ja
scripts/setup.sh --media /path/to/English-XP.iso --greeter
```

能正常工作以后，可以把 `-ja` 的目录删掉。

## 简体中文版 XP

把简体中文版 XP SP3 的 ISO 交给 `--media`，安装过程就会按简体中文版 XP 走；选项完全一样。

- 区域是 `zh_CN.UTF-8`（Wine 由此取到 ANSI 代码页 936）。中文输入用 fcitx5 + 拼音，`session.json` 里会写入
  `"input_method": "fcitx5"`。
- 字体是简体中文版 XP 的宋体（SimSun）9 pt，Luna 和经典主题下都是。Luna 的字体来自你自己 ISO 里的中文
  `luna.msstyles`（补丁 0066）；经典主题下的字体由 `prefix/registry.py` 写入。Linux 应用拿到的也是宋体。
- 命令提示符使用新宋体（NSimSun，在 `simsun.ttc` 里），8×16 个单元格。XP 本身用的是点阵版新宋体，所以也会从 ISO
  取出 `vga936.fon`。
- 双字节字符的折行与日文版 XP 相同（补丁 0044，现在也覆盖 936），控制台里这样一个字符占两个单元格
  （补丁 0049，与 932 是同一条代码路径）。
- 会话会在启动 fcitx5 之前把输入法写进它的配置（`~/.config/fcitx5/profile`）：已有的条目保留，缺的追加。
  用 Ctrl+Space 切换。这需要 `fcitx5-chinese-addons`，`setup.sh` 会装上。
- 带 `--greeter` 时，如果 logonui 是中文，欢迎屏幕的文字用宋体绘制。
- **还没有在中文版 ISO 上核对过。** 语言判断、字体、区域、控制台的单元格（新宋体 8×16）、经典主题的字体（宋体 9 pt）
  和欢迎屏幕的布局，取自 XP 的已知默认值和在日文版 XP 上量到的值，而不是在中文参考机上量出来的。手上有中文版 ISO 的话，
  可以把 `python3 tests/xp_language.py zh=.local/assets` 的结果和屏幕上的画面与 XP 本身对照。
- 繁体中文版 XP（0x0804 以外的中文，例如 0x0404）会按英文版 XP 安装。它的字体（MingLiU）和控制台字体都不同，
  照原样不能像日文版、简体中文版那样使用。`setup.sh` 会给出警告。
- 在日文版、简体中文版和英文版 XP 之间切换，把 `.local/assets` 和 `.local/prefix` 移开，再用另一个 ISO 跑
  `setup.sh`，和上面「英文版 XP」一节一样。

## ReactOS 的外壳

没有 XP 的 ISO 时，可以装 ReactOS 的外壳（经典外观）：

```sh
scripts/setup.sh --shell reactos --lang zh
```

- 会从 GitHub 下载 ReactOS 0.4.16 的发布包（约 200 MB），校验 SHA-256，只把外壳需要的文件取到 `.local/assets-reactos`；
  压缩包留在 `.local/dl`。
- prefix 是 `.local/prefix-reactos`，设置写在 `~/.config/win4wine/reactos-session.json`，会话名是
  「**ReactOS Shell (Wine, X11)**」。XP 外壳的设置和会话不受影响，两者可以并存（Wine 和 bridge 共用）。
- 带 `--greeter` 时，登录界面变成 ReactOS 的登录画面。
- `--lang` 可以是 `ja`、`zh`（简体中文）或 `en`：决定外壳的语言、它的 CJK 回退字体链接、开始菜单里文件夹的名字，
  以及会话的区域和输入法。
- `--theme` 可以是 `classic`（默认，类似 Windows 2000 的经典外观）或 ReactOS 自带的 `lautus`、`lunar`、`mizu`；
  以后想换就用 `--theme` 再跑一次 setup.sh。
- 带 `--wayland` 时也会安装 Wayland 会话「ReactOS Shell (Wine, Wayland)」（比 X11 更实验）。
- 细节：[docs/reactos.md](../../docs/reactos.md)（日文）；中文摘要见 [docs/README.zh.md](../../docs/README.zh.md)。

## 3. 登录

重启显示管理器（或者重启机器）。

- 带 `--greeter`：在 XP 的欢迎屏幕上选择用户并输入密码。
- 否则：在登录界面选择会话「**XP Shell (Wine, X11)**」（带 `--wayland` 时还有「**XP Shell (Wine, Wayland)**」）。

## 用法

| 想做的事 | 做法 |
|---|---|
| 切换日文输入 | Ctrl+Space 或全角/半角，或者点通知区域里的图标 |
| 切换中文输入（简体中文版 XP） | Ctrl+Space，或者点通知区域里的图标 |
| 启动 Linux 应用 | 开始 → 所有程序（名字以「(Linux)」结尾） |
| 安装 Windows 应用 | 在终端里 `wine setup.exe`；会话的 `PATH` 以这个 Wine 开头 |
| 打开开始菜单 | Windows 键、Ctrl+Esc |
| 运行 / 资源管理器 | Win+R / Win+E |
| 注销或关机 | 开始 → 注销 / 关闭计算机 |

## 设置（`~/.config/win4wine/session.json`）

| 键 | 默认值 | 含义 |
|---|---|---|
| `wine`, `prefix` | （由 setup.sh 写入） | 要使用的 Wine 与 prefix 的绝对路径 |
| `locale` | `ja_JP.UTF-8`（简体中文版 XP 为 `zh_CN.UTF-8`，英文版为 `en_US.UTF-8`） | 会话的区域 |
| `input_method` | `fcitx5`（英文版 XP 没有这一项） | 输入法；留空则不启动。日文的 Mozc 和简体中文版 XP 的拼音由会话在缺失时加进 fcitx5 的配置 |
| `linux_font` | `MS UI Gothic`（简体中文版 XP 为 `SimSun`，英文版为 `Tahoma`） | Linux 应用的字体；`null` 表示不动 |
| `linux_menu` | `true` | 在开始菜单里列出 Linux 应用 |
| `welcome_screen` | `true` | 登录后和注销过程中显示 XP 的欢迎屏幕 |
| `cursor_theme` | `win4wine-xp` | 鼠标指针主题；`null` 表示不改 |
| `shutdown_command`, `reboot_command` | `systemctl poweroff` / `reboot` | 「关闭计算机」执行的命令 |
| `outputs` | （无） | Wayland 会话按输出设置的参数，例如 `{"HDMI-A-1": {"scale": 1.5}, "DP-1": {"scale": 1, "position": "2560,0"}}`（也可以写 `mode`；输出名用 `wlr-randr` 查看） |
| `shell` | `xp` | 用 ReactOS 的外壳时为 `reactos`（setup.sh 会把它写进 `reactos-session.json`） |
| `primary` | 最左边的输出 | Wayland 会话把 Win32 的主显示器放在哪个输出上；XP 的桌面和任务栏就在那里 |
| `composite` | `true` | 让 X 服务器保留 Wine 窗口的内容（Composite 的自动重定向）：移动或关闭窗口时，下面的 Wine 窗口不会先变黑再重绘。`false` 关闭它 |
| `window_snap` | `false` | `true` 时窗口可以吸附到半个屏幕（像 Windows 7 的 Aero Snap；XP 本身没有这个功能）：Win+左/右 吸到左半/右半，Win+上 最大化，Win+下 还原或最小化；把标题栏拖到屏幕左/右边缘会吸到那一半，拖到上边缘会最大化；拖动已最大化窗口的标题栏会还原。下次登录生效 |

## 改变外观和文字大小

「显示 属性」（桌面右键 → 属性）会打开这个仓库安装的部件（主题、开始菜单、DPI、字体平滑；见下面的
「补上外壳缺的小部件」）。`setup.sh` 也能改同样的设置。会话的 Wine 正在运行时不能在部件里改，
所以先注销，然后：

1. 开始 → 注销，回到登录界面。
2. 按 Ctrl+Alt+F2 切到文本控制台，用同一个用户登录（或者从另一台机器 SSH 登录）。
3. 执行下面之一：

   ```sh
   cd ~/win4wine
   scripts/setup.sh --media /path/to/WindowsXPSP3.iso --theme classic   # XP 的「Windows 经典」；--theme luna 换回来
   scripts/setup.sh --media /path/to/WindowsXPSP3.iso --dpi 120          # 文字大小；96 是正常（XP 的「正常尺寸」）
   scripts/setup.sh --media /path/to/WindowsXPSP3.iso --font-smoothing cleartype   # ClearType；--font-smoothing standard 换回来
   scripts/setup.sh --shell reactos --theme lautus                      # ReactOS 的外壳：classic、lautus、lunar 或 mizu
   ```

4. 在控制台里 `exit` 注销，按 Ctrl+Alt+F7 回到登录界面，再登录一次。

- 选主题会带上它的壁纸，和 XP 上选主题一样（经典没有，Luna 是 Bliss）。
- 已经完成的步骤会跳过，所以之后的执行很快（只有在补丁变化时才重新编译 Wine）。
- DPI 调大会让文字和度量变大，图标仍然是 32×32，和 XP 一样。Linux 应用的文字也用同样的 DPI
  （GTK 在 192 以上还会把控件放大一倍）。
- `--font-smoothing cleartype` 相当于在 XP 的「使用下列方法平滑屏幕字体边缘」里选择 ClearType（默认是 XP 的标准）。
  和 XP 一样，MS UI Gothic 的小字号文字（菜单等，22 像素以下）保持点阵，而 Tahoma、Arial 等文字和较大的文字
  会变成 ClearType。Linux 应用也按屏幕的 RGB 子像素顺序绘制文字（在 X11 会话里）。

在 Wayland 会话里，Linux 应用按每个输出的缩放绘制（`outputs` 里的 `scale`），XP 外壳按它的 DPI 绘制，
在每台显示器上都不模糊；把 DPI 设成你主要使用的那个输出的缩放（1.5 就是 144）。见 [docs/wayland.md](../../docs/wayland.md)（日文）。

## 出问题时

- 会话日志：`~/.local/state/win4wine/`（上一次的日志是 `*.log.1`）。
- Debian 13 以外的发行版（例如 openKylin）也能用，但只确认过 Debian 13：软件包名和依赖不同，所以 `setup.sh`
  **会警告哪些包这个发行版没有，并跳过它们**（openKylin 3.0 没有 `fcitx5-frontend-gtk4`，它的 `xterm` 需要一个
  它没有的 `xbitmaps`；即便如此，GTK3、Qt5 和 Qt6 的应用仍然能输入）。整批安装失败时，会逐个安装软件包，
  只把装不上的那些跳过。
- XP 的外壳反复崩溃时，会话会退回到 Wine 自带的 explorer 和 xterm。
- 2026-10-13 之前，`setup.sh` 在设置新 prefix 的外观时可能失败（`Setting the theme failed`）。
  更新之后用 `scripts/setup.sh --media ISO --theme luna`（或 `classic`）再跑一次。
- 欢迎屏幕没有出现时：看 `/var/log/lightdm/seat0-greeter.log`。它需要的东西缺失时会退回到 LightDM 原本的登录界面。

## 卸载

```sh
greeter/install.sh --remove                     # 不再使用欢迎屏幕（回到 LightDM 原来的登录界面）
sudo rm /usr/share/xsessions/xp-shell.desktop /usr/local/bin/xp-shell-session   # 注销会话
sudo rm /usr/share/wayland-sessions/xp-shell-wayland.desktop /usr/local/bin/xp-shell-wayland-session   # 以及 Wayland 的那个
sudo rm /usr/share/xsessions/reactos-shell.desktop /usr/local/bin/reactos-shell-session   # 以及 ReactOS 外壳的
sudo rm /usr/share/wayland-sessions/reactos-shell-wayland.desktop /usr/local/bin/reactos-shell-wayland-session
python3 session/linux_apps.py .local/prefix --wine .local/wine-install/bin/wine --remove   # 删掉它建的开始菜单条目
sudo rm -f /etc/X11/xorg.conf.d/20-win4wine-shadowfb.conf   # 删掉虚拟机的 X 设置（下次启动 X 起生效）
```

编译产物、取出的文件和 prefix 都会随这个 checkout 的 `.local/` 一起消失。
脚本安装的软件包（包括 LightDM）和显示管理器的切换不会还原。

## 补上外壳缺的小部件

XP 的外壳有、而 Wine 没有的部件，由这个仓库自己写的程序补上。

- **桌面右键 → 属性**：外壳执行 `rundll32 desk.cpl`，而 Wine 自带的 `desk.cpl` 是空壳。`bridge-wm/desk-cpl.c`
  （部件）和 `bridge-wm/display-properties.c`（窗口）会被编译出来，由 `prefix/reactos.py` 放进 prefix，并给
  `rundll32.exe` 设置 `desk.cpl=native`。里面是主题、开始菜单、DPI 和字体平滑，写的是和 `prefix/display.py`
  一样的注册表值。
- **Windows 的工具**：Wine 的内置程序不是 prefix 里的文件，所以快捷方式没法指向它们。`bridge-wm/win-run.c`
  是唯一的指向目标，开始菜单的「附件」里放好了任务管理器、记事本、命令提示符、注册表编辑器、控制面板和 Wine 的配置。
- **中文/日文变成方框**：Wine 对中文默认的界面字体是宋体（SimSun）、对日文是 MS UI Gothic，而 ReactOS 的 prefix
  两个都没有（它带的是 ReactOS 自己的字体），于是 Wine 自己的窗口就把汉字画成方框。`prefix/reactos.py` 通过
  `FontSubstitutes` 和 `HKCU\Software\Wine\Fonts\Replacements` 把缺的字体族指向 prefix 里的 DroidSansFallback。

## 把 Proton 的兼容层装进这个 Wine

Proton 是另一套 Wine 构建，但游戏实际从它那里拿到的是 Direct3D 的翻译层（DXVK 和 VKD3D-Proton），
它们是普通的 Windows DLL，任何带 Vulkan 支持编译出来的 Wine 都能加载。

```sh
# 准备 Proton（GE-Proton 从 GitHub Releases 下载）
tar xzf GE-Proton*-x86_64.tar.gz -C ~/.local/proton
# 把层装进某个 prefix（默认是 ~/.local/prefix-games）
scripts/proton-layers.sh install ~/.local/prefix-games
scripts/proton-layers.sh check   ~/.local/prefix-games    # 看 DXVK 的日志和覆盖设置
```

这个 Wine 是否支持 Vulkan，可以用 `WINEDEBUG=+vulkan wine --version` 的警告来判断，或者看有没有
`winevulkan.dll`。没有的话，装上 `libvulkan-dev` 再用 `wine/build.sh` 重新编译（本仓库的构建默认会检测 Vulkan）。

### 双击 .exe

- **在 Windows 外壳里**（ReactOS 的桌面或资源管理器）：直接就能跑。外壳通过
  `HKCR\exefile\shell\open\command`（`"%1" %*`）启动程序，用的就是会话的 Wine——也就是本仓库用 Vulkan 编译出来的
  那个——加上它 prefix 里装的 Proton 层。核对时的日志：

```text
Loaded L"C:\users\openkylin\Desktop\probe-d3d11.exe" : native
info:  DXVK: v3.1.1-75-g71ec7b5be6c517d0
info:  D3D11InternalCreateDevice: Maximum supported feature level: D3D_FEATURE_LEVEL_11_1
```

- **在 Linux 侧**（openKylin 自己的桌面或文件管理器）：那边完全不认识 .exe，所以 `scripts/exe-handler.sh`
  会装上 MIME 处理器（`win4wine-exe.desktop`）、启动器（`win4wine-run-exe`）和同样的 `wine-games` 包装脚本，
  它们用的是同一个 Wine 和同一套层。可以用 `gio open program.exe` 验证，那和双击走的是同一条路。
  启动了什么会写进 `~/.local/state/win4wine/run.log`。

`wine-games` 会挑支持 Vulkan 的那个 Wine（lib 里有 `winevulkan.dll` 的那个），并且先把 prefix 初始化好再装层，
所以第一次双击也能用上 DXVK。
