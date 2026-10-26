# PWMの分解能をtick単位にする改善案（未決定）

- 作成日: 2026-10-27
- 前提: `main`（v0.11.0）の`cores/arduino/r01lib/PwmOut.cpp`と`arduino_api/arduino_analog.cpp`を読んだ上での検討
- **位置づけ: 提案と調べた事実であり、決定事項ではない。** 採用したら`CLAUDE.md`の残りのPendingタスク11と`docs/DEVELOPMENT_LOG.md`へ移し、ここからは消す
- 下の数値はコードの計算をPythonで再現したもので、**実機では測っていない**（§5に確かめ方）

---

## 1. 何が問題か

`PwmOut`は周期とパルス幅を**整数のマイクロ秒**で持っている。

- FlexPWM（A153・A156・N947・N236）: `SubmoduleState::period_us`と`pulse_us[2]`（`uint32_t`）
- TPM（C444）: `TpmState::period_us`と`PwmOut::_pulse_us`（`uint32_t`）

このため、ハードウェアのカウンタがどれだけ細かくても、**デューティの段階数は「周期のµs数」が上限**になり、周波数も1 µs刻みでしか選べない。

経路（FlexPWM）:

1. `analogWriteFrequency(pin, f)` → `period_us( 1000000 / f )`: 周期を整数µsに切り捨て
2. `analogWrite(pin, v)` → `write( v / max )` → `pulsewidth( duty * period_us * 1e-6f )` → `pulse_us = (uint32_t)( seconds * 1e6f )`: パルス幅を整数µsに切り捨て（float32の往復も入る）
3. `apply()`: µsをtickに直す（`pulse_us * (src_clk >> prescale) / 1e6`）→ `duty16 = dutyTicks * 65535 / pulseCnt`
4. SDKの`PWM_UpdatePwmPeriodAndDutycycle()`: `pwmHighPulse = pulseCnt * duty16 / 65535`で、またtickに戻す（ここでも切り捨て）

## 2. 影響の大きさ（A156、メインクロック96MHz、8ビット指定）

| 周波数 | 周期 | 1周期のtick数 | 区別できるデューティの段階 |
|---|---|---|---|
| 1 kHz（既定） | 1000 µs | 48000（1/2分周） | 256 |
| 3906 Hz | 256 µs | 24576 | **256（ここが上限）** |
| 3910 Hz | 255 µs | 24480 | 253 |
| 5 kHz | 200 µs | 19200 | 201 |
| 10 kHz | 100 µs | 9600 | 101 |
| 20 kHz | 50 µs | 4800 | 51 |
| 100 kHz | 10 µs | 960 | 11 |

- 8ビットを保てる上限は**約3.9 kHz**。ハードウェアの上限は96MHz/256 = **375 kHz**で、2桁近く使い残している
- 50 Hz（サーボ）では周期20000 µsで、区別できるのは20001段階（約14.3ビット）。`analogWriteResolution(14)`までは全段階が別の値になり、15・16ビットは20001段階で頭打ち。ハードウェアは1/32分周で60000 tickなので、約15.9ビットまで出せる
- 周波数そのものも1 µs刻み: 30 kHzを指定すると周期33 µsで約30.3 kHzになる
- µsの切り捨てはクロックに依らないので、**A153・N947・N236も同じ約3.9 kHzが上限**（ハードウェアの上限はクロックで違う: A153 96MHz→375 kHz、N947/N236 150MHz→約586 kHz）。C444（TPM、48MHz）も同じくµsで持っているので同様と見込むが、**未確認**（実装が別なので計算し直す）

## 3. 直し方の案

### 3-1. 状態をtickで持つ

- `SubmoduleState`（C444は`TpmState`）に、周期を**ソースクロックのtick数**（分周前、`uint32_t`か`uint64_t`）で持たせる。パルス幅も同じtickで持つ
- 分周（`prescale`）は`apply()`で「周期のtickが16ビットに収まる最小の分周」を選ぶ今の方式のままでよい。比較レジスタの値は`pulse_ticks >> prescale`
- 周期を変えたときに相手チャンネルのパルス幅（絶対値）を保つ今の約束（`analogWriteFrequency()`の説明）は、tickで持っても同じに守れる

### 3-2. 入口ごとの計算

| 入口 | 今 | 案 |
|---|---|---|
| `analogWriteFrequency(pin, f)` | `period_us( 1000000 / f )` | `period_ticks = round( src_clk / f )`。µsを経由しない |
| `analogWrite(pin, v)` | `write( (float)v / max )` → µs | `pulse_ticks = period_ticks * v / max`（整数演算、64ビット）。0と`max`は端の値をそのまま（常にLOW／常にHIGH）にする |
| `PwmOut::period()`/`pulsewidth()`（秒、float） | µsに切り捨て | `seconds * src_clk`をtickに丸める |
| `PwmOut::period_us()`/`pulsewidth_us()` | そのまま | `us * src_clk / 1000000` |
| `PwmOut::read()` | `pulse_us / period_us` | `pulse_ticks / period_ticks` |

`analogWrite()`がfloatを経由しない専用の入口（例: `PwmOut::write_ratio( uint32_t num, uint32_t den )`）を`PwmOut`に足すと、float32の丸めも無くなる。

### 3-3. SDKの関数を通さず比較レジスタを書く

`PWM_UpdatePwmPeriodAndDutycycle()`はデューティを16ビットの割合（`duty16`）で受け取り、中で`pulseCnt * duty16 / 65535`でtickに戻すので、往復で切り捨てが2回入る。
SDKの`PWM_SetDutycycleRegister()`相当（エッジアラインドなら`VAL2`/`VAL3`か`VAL4`/`VAL5`と`VAL1`）を直接書けば、tickのまま渡せる。
今の作りが`PWM_UpdatePwmPeriodAndDutycycle()`を使う理由（`PWM_SetupPwm()`を1回だけ呼び、以後はLDOKまでバッファされる値だけを書く）は、レジスタを直接書いても保てる

## 4. 変える範囲と、変わる挙動

- 変えるファイル: `cores/arduino/r01lib/PwmOut.{h,cpp}`（FlexPWMとTPMの両方）、`cores/arduino/arduino_api/arduino_analog.cpp`
- **スケッチから見える変化**:
  - 3.9 kHzを超える周波数で、`analogWrite()`の段階が指定した分解能どおりに出る
  - 周波数が指定に近くなる（30 kHz → 30.3 kHzだったものが30.000 kHz近く）
  - 1 kHz・50 Hzなどの既定の使い方では、デューティが最大でも1 µs未満だけ変わる（今は切り捨てで短めに出ている）
- `API_COMPATIBILITY.md`の`analogWriteFrequency`の行（「pulse width」「absolute pulse width across the change」）は意味が変わらない。分解能の上限（周波数とビット数の関係）を書き足す
- `Servo`と`mcxRCServo`は`writeMicroseconds()`がµs単位のAPIなので、今と同じ値が出る（分解能の改善は`write()`の角度指定で少し効く）
- 1.0で`analogWriteFrequency()`の挙動を互換の対象にする前に入れるのがよい（`API_COMPATIBILITY.md`の「What stays compatible」）

## 5. 確かめ方の案

- 計算の確認: `apply()`相当をホストで動かすテスト（下の再現コードを元に）で、各ボードのクロック・周波数・分解能の組み合わせについて「区別できる段階 = 2^bits（ハードウェアの上限まで）」を確かめる
- 実機: `test_analogWrite_duty`・`test_analogWriteFrequency`・`test_analogWrite_pairs_and_pinMode`・`test_Servo`をA156・N947・C444で回す。加えて、20 kHz・8ビットで`analogWrite(pin, 1)`と`analogWrite(pin, 2)`のパルス幅をロジアナで測り、1/255周期ずつ違うことを見る（今は50 µs周期なので0 µsと0 µsで同じになる）
- `release_check/08`の`analogWrite`の項目

## 6. 再現に使った計算（FlexPWM、A156）

```python
import numpy as np
f32 = np.float32
CLK = 96_000_000          # A156のMainClk（BOARD_BootClockFRO96M）

def steps(freq, bits=8):
    P = 1_000_000 // freq                 # analogWriteFrequency(): period_us
    mx = (1 << bits) - 1
    for pre in range(8):                  # apply(): 16ビットに収まる最小の分周
        cnt = (P * (CLK >> pre)) // 1_000_000
        if cnt <= 0xFFFF or pre == 7:
            break
    cnt = max(1, min(cnt, 0xFFFF))
    seen = set()
    for v in range(mx + 1):
        duty = f32(v) / f32(mx)                       # analogWrite()
        sec = f32(duty * f32(P) * f32(1e-6))          # write()
        pu = f32(sec * f32(1e6))                      # pulsewidth()
        pu = min(int(pu), P)
        dt = min((pu * (CLK >> pre)) // 1_000_000, cnt)
        d16 = (dt * 65535) // cnt                     # apply(): duty16
        seen.add((cnt * d16) // 65535)                # SDK: pwmHighPulse
    return P, cnt, len(seen)
```
