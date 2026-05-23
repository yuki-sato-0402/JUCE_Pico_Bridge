import board
import busio
import digitalio
import pwmio
import rotaryio
from time import sleep
import usb_midi
import adafruit_midi
from adafruit_midi.system_exclusive import SystemExclusive
from adafruit_midi.control_change import ControlChange

# --- LCD設定 ---
i2c = busio.I2C(board.GP15, board.GP14) #[cite: 1]
LCD_addr = 0x27
LCD_EN, LCD_BL = 0x04, 0x08
CMD, CHR = 0x00, 0x01
LINE1, LINE2 = 0x80, 0xC0
buf = bytearray(2)

while not i2c.try_lock():
    pass

def LCD_write(bits, mode):
    data = (bits & 0xF0) | mode
    buf[0] = data | LCD_EN | LCD_BL
    buf[1] = data | LCD_BL
    i2c.writeto(LCD_addr, buf)
    sleep(0.0001)
    data = ((bits << 4) & 0xF0) | mode
    buf[0] = data | LCD_EN | LCD_BL
    buf[1] = data | LCD_BL
    i2c.writeto(LCD_addr, buf)
    sleep(0.0001)

def LCD_init():
    for cmd in [0x33, 0x32, 0x06, 0x0C, 0x28, 0x01]:
        LCD_write(cmd, CMD)
    sleep(0.002)

def LCD_clear():
    LCD_write(0x01, CMD)
    sleep(0.002)

def LCD_cursor(x, y):
    LCD_write((LINE1 if y == 0 else LINE2) + x, CMD)

def LCD_print(message):
    for c in message:
        LCD_write(ord(c), CHR)

# --- パラメータ管理 ---
Param_Config = {
    20: {"name": "harmonicSeriesMode", "val_str": "--", "cur_midi": 0, "max_val": 127},
    21: {"name": "harmonicRatio", "val_str": "--", "cur_midi": 0, "max_val": 127},
    22: {"name": "oscillator", "val_str": "--", "cur_midi": 0, "max_val": 127},
    23: {"name": "terms", "val_str": "--", "cur_midi": 0, "max_val": 127},
    24: {"name": "filterOnOff", "val_str": "--", "cur_midi": 0, "max_val": 127},
    25: {"name": "cutoffOvertone", "val_str": "--", "cur_midi": 0, "max_val": 127},
    26: {"name": "attenuation", "val_str": "--", "cur_midi": 0, "max_val": 127},
    27: {"name": "PosNegSync", "val_str": "--", "cur_midi": 0, "max_val": 127},
    28: {"name": "PosNeg", "val_str": "--", "cur_midi": 0, "max_val": 127},
    29: {"name": "cycleCountToAdd", "val_str": "--", "cur_midi": 0, "max_val": 127},
    30: {"name": "cycleCountToSubtract", "val_str": "--", "cur_midi": 0, "max_val": 127},
    70: {"name": "termsToAddPerCount", "val_str": "--", "cur_midi": 0, "max_val": 127},
    71: {"name": "amp", "val_str": "--", "cur_midi": 0, "max_val": 127},
    72: {"name": "attack", "val_str": "--", "cur_midi": 0, "max_val": 127},
    73: {"name": "decay", "val_str": "--", "cur_midi": 0, "max_val": 127},
    74: {"name": "sustain", "val_str": "--", "cur_midi": 0, "max_val": 127},
    75: {"name": "release", "val_str": "--", "cur_midi": 0, "max_val": 127}
}
cc_keys = sorted(Param_Config.keys())
current_idx = 0

def display_param_and_value(name, val_text):
    """名前の長さに応じて値の表示位置を変える"""
    LCD_clear()
    if len(name) <= 16:
        LCD_cursor(0, 0)
        LCD_print(name)
        LCD_cursor(0, 1)
        LCD_print(str(val_text))
    else:
        LCD_cursor(0, 0)
        LCD_print(name[:16])
        LCD_cursor(0, 1)
        remaining_name = name[16:]
        LCD_print(f"{remaining_name} {val_text}")

# --- LED・PWM設定 ---
led_tact1 = pwmio.PWMOut(board.GP22, frequency=5000, duty_cycle=0)
led_val = pwmio.PWMOut(board.GP26, frequency=5000, duty_cycle=0) # 以前のled_pot
led_tact2 = pwmio.PWMOut(board.GP17, frequency=5000, duty_cycle=0)

# --- 入出力ピン設定 ---
encoder = rotaryio.IncrementalEncoder(board.GP27, board.GP28)
last_encoder_pos = encoder.position

def setup_sw(pin):
    io = digitalio.DigitalInOut(pin)
    io.direction = digitalio.Direction.INPUT
    io.pull = digitalio.Pull.UP
    return io

sw1Up, sw1Down = setup_sw(board.GP5), setup_sw(board.GP7)
sw2Up, sw2Down = setup_sw(board.GP20), setup_sw(board.GP8)

# --- MIDI設定 ---
midi_in = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], in_buf_size=128)
midi_out = adafruit_midi.MIDI(midi_out=usb_midi.ports[1], out_channel=0)

LCD_init()
display_param_and_value(Param_Config[cc_keys[current_idx]]["name"], "Ready")

last_states = [True, True, True, True] 

while True:
    # --- 1. MIDI受信 (JUCEからのフィードバック) ---
    msg = midi_in.receive()
    if isinstance(msg, SystemExclusive) and msg.manufacturer_id == b'\x7d':
        try:
            val_str = "".join([chr(b) for b in msg.data])
            # カンマ区切りで解析: パラメータ名, 現在値, 最大値
            parts = val_str.split(",")
            if len(parts) == 3:
                p_name, p_val, p_max = parts[0], parts[1], int(parts[2])
                for cc, config in Param_Config.items():
                    print(f"Checking {config['name']} against {p_name}")
                    if config["name"] == p_name:
                        print(f"Matched {p_name} to CC {cc}")
                        config["val_str"] = p_val
                        config["max_val"] = p_max
                        if cc == cc_keys[current_idx]:
                            display_param_and_value(config["name"], config["val_str"])
                        break
            else:
                print(f"Unexpected SysEx format: {val_str}")
                config = Param_Config[cc_keys[current_idx]]
                config["val_str"] = val_str
                display_param_and_value(config["name"], val_str)
        except Exception as e:
            print(f"SysEx Error: {e}")

    # --- 2. パラメータ切り替え (sw1) + LED制御 ---
    if not sw1Up.value or not sw1Down.value:
        led_tact1.duty_cycle = 65535
    else:
        led_tact1.duty_cycle = 0

    if not sw1Up.value and last_states[0]:
        current_idx = (current_idx + 1) % len(cc_keys)
        config = Param_Config[cc_keys[current_idx]]
        display_param_and_value(config["name"], config["val_str"])
    
    if not sw1Down.value and last_states[1]:
        current_idx = (current_idx - 1) % len(cc_keys)
        config = Param_Config[cc_keys[current_idx]]
        display_param_and_value(config["name"], config["val_str"])

    # --- 3. Encoder操作 + LED調光 ---
    current_pos = encoder.position
    delta = current_pos - last_encoder_pos
    
    if delta != 0:
        config = Param_Config[cc_keys[current_idx]]
        # 20クリックで360度 = MIDI 0-127の全域
        # 1クリックあたりの変化量 = 127 / 20 = 6.35
        step_val = 127 / 20
        new_midi = config["cur_midi"] + (delta * step_val)
        config["cur_midi"] = max(0, min(127, int(new_midi)))
        print(f"Encoder: {delta} steps, MIDI Value: {config['cur_midi']}")
        
        midi_out.send(ControlChange(cc_keys[current_idx], config["cur_midi"]))
        last_encoder_pos = current_pos

    # LEDを現在のMIDI値に合わせて調光 (0-127 -> 0-65535)
    led_val.duty_cycle = int(Param_Config[cc_keys[current_idx]]["cur_midi"] * 516)

    # --- 4. パラメータ操作 (sw2) + LED制御 ---
    if not sw2Up.value or not sw2Down.value:
        led_tact2.duty_cycle = 65535
    else:
        led_tact2.duty_cycle = 0

    if not sw2Up.value and last_states[2]:
        config = Param_Config[cc_keys[current_idx]]
        step = max(1, 128 // config["max_val"]) if config["max_val"] > 0 else 1
        config["cur_midi"] = min(127, config["cur_midi"] + step)
        midi_out.send(ControlChange(cc_keys[current_idx], config["cur_midi"]))
    
    if not sw2Down.value and last_states[3]:
        config = Param_Config[cc_keys[current_idx]]
        step = max(1, 128 // config["max_val"]) if config["max_val"] > 0 else 1
        config["cur_midi"] = max(0, config["cur_midi"] - step)
        midi_out.send(ControlChange(cc_keys[current_idx], config["cur_midi"]))

    # 状態保存とウェイト
    last_states = [sw1Up.value, sw1Down.value, sw2Up.value, sw2Down.value]
    sleep(0.01)
