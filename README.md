# ESP32-S3 WiFi 调试工具

## 功能

| 功能 | 说明 |
|------|------|
| **Web 面板** | `http://<IP>/` 查看状态、LED控制、OTA升级 |
| **Telnet** | `telnet <IP> 23` 远程串口调试 |
| **OTA 升级** | 网页上传 bin 文件无线烧录 |
| **WiFi 重连** | 断开自动重连，失败自动开AP |

## 使用方法

### 1. 修改 WiFi 配置
编辑 `esp32_wifi_debug.ino`，填入你的 WiFi 信息：
```cpp
#define WIFI_SSID      "你的WiFi名"
#define WIFI_PASSWORD  "你的WiFi密码"
```

### 2. 编译上传（USB）
```bash
cd esp32_wifi_debug
pio run -t upload
```

### 3. 之后无线更新（OTA）
```bash
# 方法1: PlatformIO OTA
pio run -t upload --upload-port <ESP32_IP>

# 方法2: 网页上传
# 浏览器打开 http://<ESP32_IP>/ota
# 选择 .pio/build/esp32-s3-devkitc-1/firmware.bin 上传
```

## 目录结构

```
esp32_wifi_debug/
├── esp32_wifi_debug.ino   # 主程序
├── platformio.ini        # PlatformIO 配置
└── README.md             # 本文件
```

## T-Display-S3 引脚注意

- **RGB LED**: GPIO 48
- **BOOT 按钮**: GPIO 0 (按BOOT+RESET进入下载模式)
- **USB-C**: 既是串口也支持原生USB

## 串口调试命令

连接 Telnet 后，直接输入命令会透传到串口。
可用串口监视器看到完整输出。

## WiFi 备份模式

如果 WiFi 连接失败，会自动开启 AP 模式：
- SSID: `ESP32-Debug`
- 密码: `12345678`
- AP IP: `192.168.4.1`
