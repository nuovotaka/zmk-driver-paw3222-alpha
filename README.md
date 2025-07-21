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

---

## Overview

The PAW3222 is a low-power optical mouse sensor suitable for tracking applications such as mice and trackballs. This driver communicates with the PAW3222 sensor via SPI interface. It supports flexible configuration via devicetree and Kconfig, and enables advanced usage such as layer-based input mode switching and runtime configuration.

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
        // rotation = <0>;  　   // default:0　(0, 90, 180, 270)
        // scroll-tick = <10>;  // default:10
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

Also, make sure to add the following line to your `.conf` file to enable input support:

```
CONFIG_INPUT=y
```

---

## Usage

- The driver automatically switches input mode (move, scroll, snipe) based on the active ZMK layer and your devicetree configuration.
- You can adjust CPI (resolution) at runtime using the API (see below).
- Use `rotation` to match the sensor’s physical orientation.
- Configure `scroll-tick` to tune scroll sensitivity.

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

---

## 概要

PAW3222 は、マウスやトラックボールなどのトラッキング用途に適した低消費電力の光学センサーです。このドライバは SPI インターフェースを介して PAW3222 センサーと通信します。デバイスツリーや Kconfig で柔軟に設定でき、レイヤーごとの入力モード切り替えや実行時設定変更など高度な使い方も可能です。

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

| プロパティ名             | 型            | 必須 | 説明                                                 |
| ------------------------ | ------------- | ---- | ---------------------------------------------------- |
| irq-gpios                | phandle-array | Yes  | モーションピンに接続された GPIO（アクティブ Low）    |
| power-gpios              | phandle-array | No   | 電源制御ピンに接続された GPIO                        |
| res-cpi                  | int           | No   | センサーの CPI 解像度（API で実行時変更可）          |
| force-awake              | boolean       | No   | "force awake"モードで初期化（API で実行時変更可）    |
| rotation                 | int           | No   | センサーの角度を設定 (0, 90, 180, 270)               |
| scroll-tick              | int           | No   | スクロール感度の閾値を設定                           |
| snipe-layers             | array         | No   | スナイプモードで切り替えるレイヤー番号のリスト       |
| scroll-layers            | array         | No   | スクロールモードで切り替えるレイヤー番号のリスト     |
| scroll-horizontal-layers | array         | No   | 水平スクロールモードで切り替えるレイヤー番号のリスト |

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

さらに、`.conf` ファイルに以下の 1 行を追加して input サポートを有効にしてください：

```
CONFIG_INPUT=y
```

---

## 使い方

- アクティブな ZMK レイヤーとデバイスツリー設定に応じて、入力モード（移動・スクロール・スナイプ）が自動で切り替わります。
- API を使って実行時に CPI（解像度）を変更できます（下記参照）。
- `rotation` でセンサーの物理的な向きを調整できます。
- `scroll-tick` でスクロール感度を調整できます。

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
