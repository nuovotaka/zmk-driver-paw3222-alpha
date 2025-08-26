[ [English](README.md) | Japanese ]

---

<div align="center">
    <h1>ZMK PAW3222 ドライバ</h1>
        <a href="https://badgen.net/badge/ZMK/v0.3.0/blue" alt="ZMK">
            <img src="https://badgen.net/badge/ZMK/v0.3.0/blue" />
        </a>
        <a href="https://badgen.net/badge/License/Apache-2.0">
            <img src="https://badgen.net/badge/License/Apache-2.0" />
        </a>
        <a href="https://deepwiki.com/nuovotaka/zmk-driver-paw3222-alpha" alt="Ask DeepWiki">
        <img src="https://deepwiki.com/badge.svg" />
    </a>
    <hr>
    <p align="center">
        このドライバは、PIXART PAW3222 光学センサーを ZMK フレームワークで使用できるようにします。
    </p>
</div>

---

## 特徴

- PAW3222 センサーとの SPI 通信
- カーソル移動、垂直/水平スクロール、高精度スナイプモード対応
- レイヤーごとの入力モード自動切り替え（移動・スクロール・スナイプ）
- 実行時 CPI（解像度）変更対応
- 電源管理・低消費電力モード
- オプションで電源 GPIO 制御
- **トグルベースのモード切替:** 柔軟なモード制御のための 3 つの独立したトグル機能：
  - **Move/Scroll トグル:** カーソル移動とスクロールモードの切り替え
  - **Normal/Snipe トグル:** カーソルとスクロール両方の高精度（スナイプ）モードの有効/無効
  - **Vertical/Horizontal トグル:** スクロール方向を垂直と水平で切り替え
- **ビヘイビア API 統合:** Zephyr のビヘイビアドライバ API を実装し、キー割り当てに対応

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
      revision: v0.3.0
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
        // snipe-divisor = <2>; // デフォルト:2 (Kconfigで設定可能)
        // snipe-layers = <5>;
        // scroll-layers = <6>;
        // scroll-horizontal-layers = <7>;
        // scroll-snipe-layers = <8>
        // scroll-horizontal-snipe-layers = <9>;
    };
};
```

---

## プロパティ

| プロパティ名                   | 型            | 必須 | 説明                                                       |
| ------------------------------ | ------------- | ---- | ---------------------------------------------------------- |
| irq-gpios                      | phandle-array | Yes  | モーションピンに接続された GPIO（アクティブ Low）          |
| power-gpios                    | phandle-array | No   | 電源制御ピンに接続された GPIO                              |
| res-cpi                        | int           | No   | センサーの CPI 解像度（API で実行時変更可）                |
| force-awake                    | boolean       | No   | "force awake"モードで初期化（API で実行時変更可）          |
| rotation                       | int           | No   | センサーの角度を設定 (0, 90, 180, 270)                     |
| scroll-tick                    | int           | No   | スクロール感度の閾値を設定                                 |
| snipe-divisor                  | int           | No   | スナイプモードの感度除数（値が大きいほど低感度）           |
| snipe-layers                   | array         | No   | スナイプモードで切り替えるレイヤー番号のリスト             |
| scroll-layers                  | array         | No   | スクロールモードで切り替えるレイヤー番号のリスト           |
| scroll-horizontal-layers       | array         | No   | 水平スクロールモードで切り替えるレイヤー番号のリスト       |
| scroll-snipe-layers            | array         | No   | 高精度垂直スクロールモードで切り替えるレイヤー番号のリスト |
| scroll-horizontal-snipe-layers | array         | No   | 高精度水平スクロールモードで切り替えるレイヤー番号のリスト |
| scroll-snipe-divisor           | int           | No   | スクロールスナイプモードの感度除数（値が大きいほど低感度） |
| scroll-snipe-tick              | int           | No   | スナイプモードでのスクロール閾値（値が大きいほど鈍感）     |

---

## Kconfig

キーボードの `Kconfig.defconfig` に以下を追加してください：

```kconfig
if ZMK_KEYBOARD_YOUR_KEYBOARD

config ZMK_POINTING
    default y

config PAW3222
    default y

config PAW3222_BEHAVIOR
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
- `rotation` でスクロールが常に y 軸方向の動きで動作するよう設定します。カーソル移動の回転には ZMK の input-processors（`zip_xy_transform` など）を使用してください。
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

## Behavior-Based モード切り替え

Instead of using empty layers, you can use ZMK behaviors to switch input modes:

### Keymap の設定

```dts
/ {
    behaviors {
        paw_mode: paw_mode {
            compatible = "paw32xx,mode";
            label = "PAW_MODE";
            #binding-cells = <1>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                // Toggle between move and scroll modes
                &paw_mode 0

                // Toggle between normal and snipe modes
                &paw_mode 1

                // Toggle between Vertical and Horizontal modes
                &paw_mode 2
            >;
        };
    };
};
```

### デバイスツリーの設定

```dts
trackball: trackball@0 {
    compatible = "pixart,paw3222";
    // ... other properties ...

    // Use behavior-based switching
    switch-method = "toggle";

    // Layer-based properties are ignored when using behavior switching
};
```

### Complete Example

```dts
// In your .overlay file
&spi0 {
    trackball: trackball@0 {
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;

        // Use behavior-based switching
        switch-method = "toggle";

        // Sensitivity settings
        res-cpi = <1200>;
        scroll-tick = <10>;
        snipe-divisor = <2>;
        scroll-snipe-divisor = <3>;
        scroll-snipe-tick = <20>;
    };
};

// In your .keymap file
/ {
    behaviors {
        paw_mode: paw_mode {
            compatible = "paw32xx,mode";
            #binding-cells = <1>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp Q &kp W &kp E &kp R &kp T
                &kp A &kp S &kp D &kp F &kp G
                &kp Z &kp X &paw_mode 0 &paw_mode 1 &paw_mode 2
                //           ↑Move/Scroll   ↑Normal/Snipe  ↑Vertical/Horizontal
            >;
        };
    };
};
```

## モード切替機能

ドライバーは 3 つの独立したトグル機能を提供し、柔軟なモード制御を可能にします：

1. **Move/Scroll トグル (パラメータ 0):**

   - カーソル移動（MOVE/SNIPE）とスクロールモード（SCROLL/SCROLL_SNIPE）を切り替え
   - MOVE または SNIPE モード時：SCROLL モードに切り替え
   - 任意の SCROLL モード時：MOVE モードに戻る

2. **Normal/Snipe トグル (パラメータ 1):**

   - 現在の操作タイプで高精度（スナイプ）モードのオン/オフを切り替え
   - MOVE ↔ SNIPE（カーソル移動用）
   - SCROLL ↔ SCROLL_SNIPE（垂直スクロール用）
   - SCROLL_HORIZONTAL ↔ SCROLL_HORIZONTAL_SNIPE（水平スクロール用）

3. **Vertical/Horizontal トグル (パラメータ 2):**
   - スクロール方向を垂直と水平で切り替え
   - スクロールモード時のみ動作（MOVE/SNIPE では無効）
   - SCROLL ↔ SCROLL_HORIZONTAL
   - SCROLL_SNIPE ↔ SCROLL_HORIZONTAL_SNIPE

### モードの組み合わせ

これらのトグルを組み合わせることで、利用可能な 6 つのモード全てにアクセスできます：

- **MOVE:** デフォルトのカーソル移動
- **SNIPE:** 高精度カーソル移動
- **SCROLL:** 垂直スクロール
- **SCROLL_SNIPE:** 高精度垂直スクロール
- **SCROLL_HORIZONTAL:** 水平スクロール
- **SCROLL_HORIZONTAL_SNIPE:** 高精度水平スクロール

## 利用方法

- キーバインディングで各モード切替機能を呼び出し、パラメータで動作を指定します。
- モード変更やエラー時にログで状態を確認できます。
- デバイス参照は初期化時に設定されます。

## 初期化

デバイスツリーが正しく設定され、`CONFIG_PAW3222_BEHAVIOR`オプションが有効な場合、自動的に初期化されます。

```
CONFIG_PAW3222_BEHAVIOR=y
```

---

## 分割キーボード設定

分離した MCU を持つ分割キーボードでは、左右のトラックボールに異なる動作を設定できます。

### 独立したトラックボール設定

各サイドで完全に異なる設定を持つ PAW3222 インスタンスを設定できます：

#### 左側設定 (left.overlay)

```dts
&spi0 {
    trackball_left: trackball@0 {
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;

        // 左側：カーソル移動に最適化
        res-cpi = <800>;           // 低感度
        scroll-tick = <15>;        // スクロール感度低め
        snipe-divisor = <3>;       // 高精度モード
        switch-method = "toggle";
    };
};
```

#### 右側設定 (right.overlay)

```dts
&spi0 {
    trackball_right: trackball@0 {
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 15 GPIO_ACTIVE_LOW>;

        // 右側：スクロールに最適化
        res-cpi = <1200>;          // 高感度
        scroll-tick = <8>;         // スクロール感度高め
        snipe-divisor = <2>;       // 標準精度
        switch-method = "toggle";
    };
};
```

### 異なるビヘイビア割り当て

各サイドに異なるモード切替ビヘイビアを割り当てできます：

#### 左側キーマップ

```dts
/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp Q &kp W &kp E
                &kp A &paw_mode 0 &kp D  // 左：Move/Scrollトグルのみ
                &kp Z &kp X &kp C
            >;
        };
    };
};
```

#### 右側キーマップ

```dts
/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp R &kp T &kp Y
                &kp F &paw_mode 1 &kp H  // 右：Normal/Snipeトグルのみ
                &kp V &kp B &kp N
            >;
        };
    };
};
```

### 使用例

1. **専門的な役割分担:**

   - 左トラックボール：カーソル移動専用（低感度）
   - 右トラックボール：スクロール専用（高感度）

2. **補完的な機能:**

   - 左トラックボール：粗い移動と垂直スクロール
   - 右トラックボール：細かい移動と水平スクロール

3. **モード特化最適化:**
   - 左トラックボール：デザイン作業用（高精度）
   - 右トラックボール：ブラウジング用（高速スクロール）

### 設定のコツ

- 各サイドの用途に応じて異なる`res-cpi`値を使用
- 異なるスクロール動作のために`scroll-tick`を独立調整
- 各サイドの精度要件に基づいて`snipe-divisor`を設定
- 各 MCU が独自の状態を維持するため、完全に独立した動作が可能

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
