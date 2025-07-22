# ZMK PAW3222 Driver

This driver enables use of the PIXART PAW3222 optical sensor with the ZMK framework.

---

## Features

- SPI communication with the PAW3222 sensor
- Supports cursor movement, vertical/horizontal scrolling, and snipe (precision) mode
- Layer-based input mode switching (move, scroll, snipe)
- Runtime CPI (resolution) adjustment
- Power management and low-power modes
- Optional power GPIO support
- **Customizable acceleration curve (multi-threshold, multi-factor) for pointer movement**
- **Runtime toggle of acceleration curve via function keys**
- **Support for multiple sensors with individual control**

---

## Overview

The PAW3222 is a low-power optical mouse sensor suitable for tracking applications such as mice and trackballs. This driver communicates with the PAW3222 sensor via SPI interface. It supports flexible configuration via devicetree and Kconfig, and enables advanced usage such as layer-based input mode switching, runtime configuration, and acceleration curve customization.

---

## Installation

1. Add as a ZMK module in your `west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: nuovotaka
      url-base: https://github.com/nuovotaka
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.2.1
      import: app/west.yml
    - name: zmk-driver-paw3222-alpha
      remote: nuovotaka
      revision: main
```

---

## Device Tree Configuration

Configure the sensor in your shield or board config file (`.overlay` or `.dtsi`):

```dts
&pinctrl {
    spi0_default: spi0_default {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
        };
    };

    spi0_sleep: spi0_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
            low-power-enable;
        };
    };
};

&spi0 {
    status = "okay";
    compatible = "nordic,nrf-spim";
    pinctrl-0 = <&spi0_default>;
    pinctrl-1 = <&spi0_sleep>;
    pinctrl-names = "default", "sleep";
    cs-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;

    trackball: trackball@0 {
        status = "okay";
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;

        /* Optional features */
        // accel-move-enable;                   // Enable acceleration for cursor movement
        // accel-scroll-enable;                 // Enable acceleration for scroll movement

        // accel-thresholds = <2 5 10>;
        // accel-factors = <1000 1500 2000 2500>; // 1.0x, 1.5x, 2.0x, 2.5x

        // rotation = <0>;    // default: 0 (0, 90, 180, 270)
        // scroll-tick = <10>;  // default: 10
        // snipe-layers = <5>;
        // scroll-layers = <6 7 8 9>;
        // scroll-horizontal-layers = <7 9>;
    };
};
```

---

## Properties

| Property Name            | Type          | Required | Description                                                                                                               |
| ------------------------ | ------------- | -------- | ------------------------------------------------------------------------------------------------------------------------- |
| irq-gpios                | phandle-array | Yes      | GPIO connected to the motion pin, active low.                                                                             |
| power-gpios              | phandle-array | No       | GPIO connected to the power control pin.                                                                                  |
| res-cpi                  | int           | No       | CPI resolution for the sensor. Can also be changed at runtime using the `paw32xx_set_resolution()` API.                   |
| force-awake              | boolean       | No       | Initialize the sensor in "force awake" mode. Can also be enabled/disabled at runtime via the `paw32xx_force_awake()` API. |
| rotation                 | int           | No       | Physical rotation of the sensor in degrees. (0, 90, 180, 270)                                                             |
| scroll-tick              | int           | No       | Threshold for scroll movement (delta value above which scroll is triggered).                                              |
| snipe-layers             | array         | No       | List of layer numbers to switch between using the snipe-layers feature.                                                   |
| scroll-layers            | array         | No       | List of layer numbers to switch between using the scroll-layers feature.                                                  |
| scroll-horizontal-layers | array         | No       | List of layer numbers to switch between using the horizontal scroll feature.                                              |
| **accel-move-enable**    | boolean       | No       | Enable acceleration curve for cursor movement.                                                                            |
| **accel-scroll-enable**  | boolean       | No       | Enable acceleration curve for scroll movement.                                                                            |
| **accel-thresholds**     | array         | No       | List of speed thresholds (counts/ms) for multi-level acceleration scaling.                                                |
| **accel-factors**        | array         | No       | List of acceleration scaling factors (fixed-point, e.g. 1000=1.0x, 1500=1.5x, etc).                                       |

---

## Kconfig

Enable the module in your keyboard's `Kconfig.defconfig`:

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

Also, make sure to add the following lines to your `.conf` file:

```
CONFIG_INPUT=y
CONFIG_PAW32XX_MAX_DEVICES=4  # Maximum number of sensors (default: 4)
CONFIG_PAW32XX_TOGGLE_ACCEL_KEYCODE=0xF0  # F23 key for toggling acceleration
CONFIG_PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT=y  # Enable sensor selection mode
CONFIG_PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT_KEYCODE=0xF1  # F24 key for sensor selection mode
```

---

## Usage

- The driver automatically switches input mode (move, scroll, snipe) based on the active ZMK layer and your devicetree configuration.
- You can adjust CPI (resolution) at runtime using the API (see below).
- Use `rotation` to match the sensor’s physical orientation.
- Configure `scroll-tick` to tune scroll sensitivity.
- **Configure `accel-thresholds` and `accel-factors` in your device tree for a customizable acceleration curve.**
- **Use `accel-move-enable` or `accel-scroll-enable` to enable acceleration for cursor or scroll movement. These properties must be explicitly defined in your device tree for the runtime acceleration toggle feature to work.**

---

## Acceleration Curve (Multi-Threshold, Multi-Factor)

The driver supports a customizable acceleration curve for pointer and scroll movement.

### How It Works

- Define multiple speed thresholds and corresponding acceleration factors in your device tree.
- The driver applies the appropriate scaling factor based on the current movement speed.
- This enables both fine control for slow movements and rapid cursor travel for fast movements.
- **Toggle acceleration on/off at runtime using function keys**
- **Control multiple sensors individually or all at once**

#### Example Device Tree Configuration

```dts
accel-thresholds = <2 5 10>;
accel-factors = <1000 1500 2000 2500>; // 1.0x, 1.5x, 2.0x, 2.5x
```

- For speed < 2: 1.0x
- For speed ≥ 2 and < 5: 1.5x
- For speed ≥ 5 and < 10: 2.0x
- For speed ≥ 10: 2.5x

If not specified, sensible defaults are used. If no acceleration curve is configured, the driver falls back to linear scaling (no acceleration).

### Runtime Control of Acceleration

**Important**: To use the acceleration curve feature, you must explicitly define the `accel-move-enable` and/or `accel-scroll-enable` properties in your device tree. Without these properties, the runtime acceleration toggle feature will not work.

```dts
trackball: trackball@0 {
    // Other settings...
    accel-move-enable;    // Required for cursor movement acceleration
    accel-scroll-enable;  // Required for scroll acceleration
}
```

You can toggle the acceleration curve on/off at runtime using function keys:

- **F23 key**: Toggle acceleration for the currently selected sensor(s)
- **F24 key**: Enter sensor selection mode
  - In sensor selection mode:
    - Press **0** to select all sensors
    - Press **1-4** to select a specific sensor
    - Press **F24** again to exit selection mode

This allows you to quickly adjust acceleration settings based on your current task without changing your device tree configuration.

### Multiple Sensor Support

The driver supports multiple PAW3222 sensors connected to the same MCU:

- Each sensor can be controlled individually
- Works with split keyboards (each side manages its own sensors)
- Up to 4 sensors per MCU are supported by default

For split keyboards, each side's MCU manages its own sensors independently. For example, if you have two sensors on each side of a split keyboard, you can control them separately using the sensor selection mode.

---

## API Reference

### Change CPI (Resolution)

```c
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);
```

- Changes sensor resolution at runtime.

### Force Awake Mode

```c
int paw32xx_force_awake(const struct device *dev, bool enable);
```

- Enables/disables "force awake" mode at runtime.

### Toggle Acceleration Curve

```c
int paw32xx_toggle_acceleration(const struct device *dev, bool enable_move, bool enable_scroll);
```

- Changes acceleration curve settings for a specific sensor.

---

## Customization Tips

- **If you prefer finer, more precise control:**  
  Lower the thresholds and/or reduce the acceleration factors.  
  This will make the pointer respond more gently, making small, slow movements easier to control.

- **If you prefer more dynamic, faster movement:**  
  Raise the thresholds and/or increase the acceleration factors, or add more steps.  
  This will make the pointer move farther when you move the mouse quickly, allowing for rapid cursor movement across the screen.

Adjust these values in your device tree configuration to match your personal preference or application needs.

## Example Tuning for Different Use Cases

### 1. Gaming (FPS, fast aiming)

- **Recommended settings:**
  ```dts
  accel-thresholds = <3 7 15>;
  accel-factors = <1000 1300 1700 2200>; // 1.0x, 1.3x, 1.7x, 2.2x
  ```
- **Rationale:**  
  Lower thresholds and moderate factors allow for precise slow aiming, but enable fast turning when you move the mouse quickly.

---

### 2. CAD / Design Work

- **Recommended settings:**
  ```dts
  accel-thresholds = <2 4 8>;
  accel-factors = <1000 1150 1300 1500>; // 1.0x, 1.15x, 1.3x, 1.5x
  ```
- **Rationale:**  
  Very gentle acceleration preserves fine control for detailed drawing or editing, minimizing overshoot.

---

### 3. Ultra-High-Resolution Monitors (4K/5K+)

- **Recommended settings:**
  ```dts
  accel-thresholds = <2 6 16>;
  accel-factors = <1000 1600 2100 2700>; // 1.0x, 1.6x, 2.1x, 2.7x
  ```
- **Rationale:**  
  Aggressive acceleration ensures you can move the pointer across a large screen area quickly, but still retain control for slow movements.

---

## Troubleshooting

- If the sensor does not work, check SPI and GPIO wiring.
- Confirm `irq-gpios` and (if used) `power-gpios` are correct.
- Use Zephyr logging to check for errors at boot.
- Ensure the ZMK version matches the required version.

---

## License

```
SPDX-License-Identifier: Apache-2.0

Copyright 2024 Google LLC
Modifications Copyright 2025 sekigon-gonnoc
Modifications Copyright 2025 nuovotaka
```

---

# ZMK PAW3222 ドライバ

このドライバは、PIXART PAW3222 光学センサーを ZMK フレームワークで使用できるようにします。

---

## 特徴

- PAW3222 センサーとの SPI 通信
- カーソル移動、垂直/水平スクロール、高精度スナイプモード対応
- レイヤーごとの入力モード自動切り替え（移動・スクロール・スナイプ）
- 実行時 CPI（解像度）変更対応
- 電源管理・低消費電力モード
- オプションで電源 GPIO 制御
- **カスタマイズ可能な加速度カーブ機能（複数しきい値・倍率）**
- **ファンクションキーによる加速度カーブのオン/オフ切り替え**
- **複数センサーの個別制御対応**

---

## 概要

PAW3222 は、マウスやトラックボールなどのトラッキング用途に適した低消費電力の光学センサーです。このドライバは SPI インターフェースを介して PAW3222 センサーと通信します。デバイスツリーや Kconfig で柔軟に設定でき、レイヤーごとの入力モード切り替えや実行時設定変更など高度な使い方も可能です。

## 加速度カーブ機能

### 加速度カーブ（多段階しきい値・倍率）

PAW3222 ドライバは、ユーザーがカスタマイズ可能な「加速度カーブ」機能をサポートしています。

#### 加速度カーブとは？

- マウスを速く動かすとカーソルがより大きく動き、ゆっくり動かすと細かく動きます。
- 精密な操作と大きな移動を両立できる、直感的な操作感を実現します。

#### 実行時の加速度カーブ制御

ファンクションキーを使って、実行時に加速度カーブのオン/オフを切り替えることができます：

- **F23 キー**: 現在選択されているセンサーの加速度カーブをオン/オフ切り替え
- **F24 キー**: センサー選択モードに入る
  - センサー選択モードでは：
    - **0**キーを押すとすべてのセンサーを選択
    - **1-4**キーを押すと特定のセンサーを選択
    - **F24**キーを再度押すと選択モードを終了

これにより、デバイスツリー設定を変更せずに、現在のタスクに応じて加速度設定を素早く調整できます。

#### 複数センサー対応

このドライバは、同じ MCU に接続された複数の PAW3222 センサーをサポートしています：

- 各センサーを個別に制御可能
- 分割キーボードでも動作（各サイドが自身のセンサーを管理）
- デフォルトで MCU あたり最大 4 つのセンサーをサポート

分割キーボードの場合、各サイドの MCU が独自のセンサーを独立して管理します。例えば、分割キーボードの両側に 2 つずつセンサーがある場合、センサー選択モードを使用して個別に制御できます。

---

## インストール

1. `west.yml` に ZMK モジュールとして追加：

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: nuovotaka
      url-base: https://github.com/nuovotaka
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.2.1
      import: app/west.yml
    - name: zmk-driver-paw3222-alpha
      remote: nuovotaka
      revision: main
```

---

## デバイスツリー設定

シールドまたはボード設定ファイル（`.overlay` または `.dtsi`）でセンサーを設定：

```dts
&pinctrl {
    spi0_default: spi0_default {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
        };
    };

    spi0_sleep: spi0_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_SCK, 0, 12)>,
                <NRF_PSEL(SPIM_MOSI, 1, 9)>,
                <NRF_PSEL(SPIM_MISO, 1, 9)>;
            low-power-enable;
        };
    };
};

&spi0 {
    status = "okay";
    compatible = "nordic,nrf-spim";
    pinctrl-0 = <&spi0_default>;
    pinctrl-1 = <&spi0_sleep>;
    pinctrl-names = "default", "sleep";
    cs-gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;

    trackball: trackball@0 {
        status = "okay";
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;

        /* オプション設定例 */
        // accel-move-enable;                   // カーソル移動時加速度を使用する
        // accel-scroll-enable;                 // スクロール時加速度を使用する

        // accel-thresholds = <2 5 10>;
        // accel-factors = <1000 1500 2000 2500>; // 1.0倍, 1.5倍, 2.0倍, 2.5倍

        // rotation = <0>;  　   // デフォルト:0　(0, 90, 180, 270)
        // scroll-tick = <10>;  // デフォルト:10
        // snipe-layers = <5>;
        // scroll-layers = <6 7 8 9>;
        // scroll-horizontal-layers = <7 9>;
    };
};
```

---

## プロパティ

| プロパティ名             | 型            | 必須 | 説明                                                   |
| ------------------------ | ------------- | ---- | ------------------------------------------------------ |
| irq-gpios                | phandle-array | Yes  | モーションピンに接続された GPIO（アクティブ Low）      |
| power-gpios              | phandle-array | No   | 電源制御ピンに接続された GPIO                          |
| res-cpi                  | int           | No   | センサーの CPI 解像度（API で実行時変更可）            |
| snipe-cpi                | int           | No   | スナイプモード時の CPI 解像度（API で実行時変更可）    |
| force-awake              | boolean       | No   | "force awake"モードで初期化（API で実行時変更可）      |
| rotation                 | int           | No   | センサーの角度を設定 (0, 90, 180, 270)                 |
| scroll-tick              | int           | No   | スクロール感度の閾値を設定                             |
| snipe-layers             | array         | No   | スナイプモードで切り替えるレイヤー番号のリスト         |
| scroll-layers            | array         | No   | スクロールモードで切り替えるレイヤー番号のリスト       |
| scroll-horizontal-layers | array         | No   | 水平スクロールモードで切り替えるレイヤー番号のリスト   |
| **accel-move-enable**    | boolean       | No   | カーソル移動時の加速度カーブ有効フラグ                 |
| **accel-scroll-enable**  | boolean       | No   | スクロール時の加速度カーブ有効フラグ                   |
| **accel-thresholds**     | array         | No   | 加速度カーブの速度しきい値（counts/ms）のリスト        |
| **accel-factors**        | array         | No   | 加速度カーブの倍率リスト（1000=1.0 倍, 1500=1.5 倍等） |

---

## Kconfig

キーボードの `Kconfig.defconfig` に以下を追加してください：

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

さらに、`.conf` ファイルに以下の行を追加してください：

```
CONFIG_INPUT=y
CONFIG_PAW32XX_MAX_DEVICES=4  # 最大センサー数（デフォルト: 4）
CONFIG_PAW32XX_TOGGLE_ACCEL_KEYCODE=0xF0  # 加速度切り替え用F23キー
CONFIG_PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT=y  # センサー選択モードを有効化
CONFIG_PAW32XX_TOGGLE_ACCEL_DEVICE_SELECT_KEYCODE=0xF1  # センサー選択用F24キー
```

---

## 使い方

- `accel-move-enable`を記述することで、カーソル移動時に加速度が適用されます。**この設定は実行時の加速度カーブ機能を使用するために必須です。**
- `accel-scroll-enable`を記述することで、スクロール時に加速度が適用されます。**この設定は実行時の加速度カーブ機能を使用するために必須です。**
- `accel-thresholds` で複数のしきい値を設定します。
- `accel-factors`で倍率を設定します。
- (`accel-thresholds`)と(`accel-factors`)で動作速度に応じて自動的に適切な倍率（加速度）が適用されます。
- アクティブな ZMK レイヤーとデバイスツリー設定に応じて、入力モード（移動・スクロール・スナイプ）が自動で切り替わります。
- API を使って実行時に CPI（解像度）を変更できます（下記参照）。
- `rotation` でセンサーの物理的な向きを調整できます。
- `scroll-tick` でスクロール感度を調整できます。

### 加速度カーブの実行時制御

**重要**: 加速度カーブ機能を使用するには、デバイスツリーで `accel-move-enable` または `accel-scroll-enable` プロパティを明示的に記述する必要があります。これらのプロパティが設定されていない場合、実行時の加速度カーブ機能は動作しません。

```dts
trackball: trackball@0 {
    // 他の設定...
    accel-move-enable;    // カーソル移動時の加速度カーブに必須
    accel-scroll-enable;  // スクロール時の加速度カーブに必須
}
```

ファンクションキーを使って、実行時に加速度カーブのオン/オフを切り替えることができます：

- **F23 キー**: 現在選択されているセンサーの加速度カーブをオン/オフ切り替え
- **F24 キー**: センサー選択モードに入る
  - センサー選択モードでは：
    - **0**キーを押すとすべてのセンサーを選択
    - **1-4**キーを押すと特定のセンサーを選択
    - **F24**キーを再度押すと選択モードを終了

### 複数センサー対応

このドライバは、同じ MCU に接続された複数の PAW3222 センサーをサポートしています：

- 各センサーを個別に制御可能
- 分割キーボードでも動作（各サイドが自身のセンサーを管理）
- デフォルトで MCU あたり最大 4 つのセンサーをサポート

分割キーボードの場合、各サイドの MCU が独自のセンサーを独立して管理します。例えば、分割キーボードの両側に 2 つずつセンサーがある場合、センサー選択モードを使用して個別に制御できます。

---

## API リファレンス

### CPI（解像度）を変更

```c
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);
```

- 実行時にセンサー解像度を変更します。

### Force Awake モード

```c
int paw32xx_force_awake(const struct device *dev, bool enable);
```

- 実行時に "force awake" モードを有効/無効にします。

### 加速度カーブを切り替え

```c
int paw32xx_toggle_acceleration(const struct device *dev, bool enable_move, bool enable_scroll);
```

- 特定のセンサーの加速度カーブ設定を変更します。

---

## カスタマイズのポイント

- **より細かい操作を重視したい場合**  
  しきい値や倍率（accel-factors）を下げてください。  
  ポインターの動きがより穏やかになり、ゆっくりした小さな動きがしやすくなります。

- **よりダイナミックな動きを重視したい場合**  
  しきい値や倍率を上げたり、段階数を増やしてください。  
  速くマウスを動かしたときにカーソルがより遠くまで移動し、画面全体を素早く移動できます。

これらの値は、用途やお好みに合わせてデバイスツリーで調整してください。

---

## 用途別のチューニング例

### 1. ゲーム用途（FPS など素早いエイム重視）

- **推奨設定例:**
  ```dts
  accel-thresholds = <3 7 15>;
  accel-factors = <1000 1300 1700 2200>; // 1.0倍, 1.3倍, 1.7倍, 2.2倍
  ```
- **解説:**  
  低めのしきい値と適度な倍率で、ゆっくり動かすときは精密なエイム、素早く動かすときは一気に大きく振り向けます。

---

### 2. CAD・設計作業

- **推奨設定例:**
  ```dts
  accel-thresholds = <2 4 8>;
  accel-factors = <1000 1150 1300 1500>; // 1.0倍, 1.15倍, 1.3倍, 1.5倍
  ```
- **解説:**  
  とても緩やかな加速度で、細かい操作や図面の微調整がしやすく、カーソルの飛びすぎを防ぎます。

---

### 3. 超高解像度モニタ（4K/5K 以上）

- **推奨設定例:**
  ```dts
  accel-thresholds = <2 6 16>;
  accel-factors = <1000 1600 2100 2700>; // 1.0倍, 1.6倍, 2.1倍, 2.7倍
  ```
- **解説:**  
  大画面でも素早くカーソルを移動できるよう、しきい値・倍率とも高め。ゆっくり動かせば細かい操作も維持できます。

---

## トラブルシューティング

- センサーが動作しない場合は、SPI や GPIO の配線を確認してください。
- `irq-gpios` および（使用する場合）`power-gpios` の指定が正しいか確認してください。
- Zephyr ログで起動時のエラーを確認してください。
- ZMK のバージョンが要件を満たしているか確認してください。

---

## ライセンス

```
SPDX-License-Identifier: Apache-2.0

Copyright 2024 Google LLC
Modifications Copyright 2025 sekigon-gonnoc
Modifications Copyright 2025 nuovotaka
```
