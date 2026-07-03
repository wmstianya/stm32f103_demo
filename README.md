# MCGS 三拼锅炉控制器固件（UART V2.1.0）

基于 STM32F103VC 的 MCGS 三拼/五拼长板锅炉控制器固件，`Soft_Version = 210`。

## 工程结构

| 目录 | 说明 |
|------|------|
| `USER/` | 入口 `main.c`、中断、启动文件 |
| `SYSTEM/` | 系统控制、Flash、RTC、USART1 |
| `HARDWARE/` | ADC、继电器、USART2~5、PWM 等外设驱动 |
| `STM32F10x_FWLib/` | ST 标准外设库 |
| `CORE/` | Cortex-M3 内核文件 |
| `docs/` | 架构与链路说明文档 |
| `tools/` | code-graph 生成脚本 |

## 文档

- [小屏单机链路（拨码0 / 机组地址1）](docs/小屏单机链路_拨码0_机组1.md)
- [PD12 继电器过零刷新逻辑](docs/PD12_继电器过零刷新逻辑.md)

## 构建

使用 Keil uVision 打开 `USER/UART.uvprojx` 编译。

## Code Graph

```powershell
python tools/generate_code_graph.py
```

输出：`.understand-anything/knowledge-graph.json`
