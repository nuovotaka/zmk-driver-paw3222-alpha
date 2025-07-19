# ZMK PAW3222 Driver

This driver enables the use of the PIXART PAW3222 optical sensor with the ZMK framework.

## Overview

The PAW3222 is a low-power optical mouse sensor suitable for tracking applications such as mice and trackballs. This driver communicates with the PAW3222 sensor via SPI interface.

## Installation

1. Add as a ZMK module in your west.yml:

```
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: nuovotaka
      url-base: https://github.com/nuovotaka
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-driver-paw3222-alpha
      remote: nuovotaka
      revision: main
```

## Device Tree Configuration

Configure in your shield or board config file (.overlay or .dtsi):

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

        /*   optional features   */
        // snipe-layers = <5>;
        // scroll-layers = <6 7 8 9>;
        // scroll-horizontal-layers = <7 9>;
    };
};
```

## Properties

| Property Name            | Type          | Required | Description                                                                                                                  |
| ------------------------ | ------------- | -------- | ---------------------------------------------------------------------------------------------------------------------------- |
| irq-gpios                | phandle-array | Yes      | GPIO connected to the motion pin, active low.                                                                                |
| power-gpios              | phandle-array | No       | GPIO connected to the power control pin.                                                                                     |
| res-cpi                  | int           | No       | CPI resolution for the sensor. Can also be changed at runtime using the `paw32xx_set_resolution()` API.                      |
| force-awake              | boolean       | No       | Initialize the sensor in "force awake" mode. Can also be enabled or disabled at runtime via the `paw32xx_force_awake()` API. |
| snipe-layers             | array         | No       | List of layer numbers to switch between using the snipes-layers feature.                                                     |
| scroll-layers            | array         | No       | List of layer numbers to switch between using the scroll-layers feature.                                                     |
| scroll-horizontal-layers | array         | No       | List of layer numbers to switch between using the horizontal scroll feature.                                                 |

## Enable the module in your keyboard's Kconfig file

Add the following to your keyboard's `Kconfig.defconfig`:

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

## Add the following in `akdk_bt1_defconfig` on the keyboard

- boards > arm > akdk_bt1

```
CONFIG_PAW32XX_SCROLL_TICK 10
CONFIG_PAW32XX_SENSOR_ROTATION_90=y
```

## CONFIG setting

| config 名                          | 型      | 必須 | 説明                                                        |
| ---------------------------------- | ------- | ---- | ----------------------------------------------------------- |
| CONFIG_PAW32XX_SENSOR_ROTATION_0   | boolean | No   | sensor location：０ deg、default（ball-left、sensor-right） |
| CONFIG_PAW32XX_SENSOR_ROTATION_90  | boolean | No   | sensor location：90 deg                                     |
| CONFIG_PAW32XX_SENSOR_ROTATION_180 | boolean | No   | sensor location：180 deg                                    |
| CONFIG_PAW32XX_SENSOR_ROTATION_270 | boolean | No   | sensor location：270 deg                                    |

---

# ZMK PAW3222 ドライバ

このドライバは、PIXART PAW3222 光学センサーを ZMK フレームワークで使用できるようにします。

## 概要

PAW3222 は、マウスやトラックボールなどのトラッキングアプリケーションに適した低消費電力の光学マウスセンサーです。このドライバは SPI インターフェースを介して PAW3222 センサーと通信します。

## インストール

1. ZMK モジュールとして追加：

```
# west.yml に追加
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: nuovotaka
      url-base: https://github.com/nuovotaka
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-driver-paw3222-alpha
      remote: nuovotaka
      revision: main
```

## デバイスツリー設定

シールドまたはボード設定ファイル（.overlay または.dtsi）で設定：

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

        /*   optional features   */
        // snipe-layers = <5>;
        // scroll-layers = <6 7 8 9>;
        // scroll-horizontal-layers = <7 9>;
    };
};
```

## プロパティ

| プロパティ名             | 型            | 必須 | 説明                                                          |
| ------------------------ | ------------- | ---- | ------------------------------------------------------------- |
| irq-gpios                | phandle-array | Yes  | モーションピンに接続された GPIO（アクティブ Low）             |
| power-gpios              | phandle-array | No   | 電源制御ピンに接続された GPIO                                 |
| res-cpi                  | int           | No   | センサーの CPI 解像度（API で実行時変更可）                   |
| force-awake              | boolean       | No   | "force awake"モードで初期化（API で実行時変更可）             |
| snipe-layers             | array         | No   | snipes-layers 機能で切り替えるレイヤー番号のリスト            |
| scroll-layers            | array         | No   | scroll-layers 機能で切り替えるレイヤー番号のリスト            |
| scroll-horizontal-layers | array         | No   | scroll-horizontal-layers 機能で切り替えるレイヤー番号のリスト |

## キーボードの Kconfig ファイルでモジュールを有効化

キーボードの `Kconfig.defconfig` に以下を追加：

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

endif
```

## キーボードの `akdk_bt1_defconfig` にて以下を追加

- boards > arm > akdk_bt1

```
CONFIG_PAW32XX_SCROLL_TICK 10
CONFIG_PAW32XX_SENSOR_ROTATION_90=y
```

## CONFIG setting

| config 名                          | 型      | 必須 | 説明                                                     |
| ---------------------------------- | ------- | ---- | -------------------------------------------------------- |
| CONFIG_PAW32XX_SENSOR_ROTATION_0   | boolean | No   | マウスセンサー位置：０度、正位置（トラボ左、センサー右） |
| CONFIG_PAW32XX_SENSOR_ROTATION_90  | boolean | No   | マウスセンサー位置：90 度回転                            |
| CONFIG_PAW32XX_SENSOR_ROTATION_180 | boolean | No   | マウスセンサー位置：180 度回転                           |
| CONFIG_PAW32XX_SENSOR_ROTATION_270 | boolean | No   | マウスセンサー位置：270 度回転                           |
