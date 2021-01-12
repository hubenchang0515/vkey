# vkey
Tool of sending virtual key event on Linux - Linux发送虚拟的按键事件的工具

## Usage
```
  Usage: vkey <keycode> [event file]
Example: vkey 125
         vkey 125 /dev/input/event0
```

* 不带 `event file` 参数时，会自动推断键盘输入设备的文件路径

