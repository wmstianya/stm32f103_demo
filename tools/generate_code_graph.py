#!/usr/bin/env python3
"""Generate .understand-anything/knowledge-graph.json for MCGS UART firmware."""

from __future__ import annotations

import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / ".understand-anything"
OUT_FILE = OUT_DIR / "knowledge-graph.json"

SCAN_DIRS = ("USER", "SYSTEM", "HARDWARE")
SKIP_PARTS = {"STM32F10x_FWLib", "CORE", "OBJ", "RTE", ".git"}

FUNC_RE = re.compile(
    r"^(?:static\s+)?(?:void|uint8|uint16|int|u8|u16|u32|float|double)\s+"
    r"(?:\*\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*\(",
    re.MULTILINE,
)
INCLUDE_RE = re.compile(r'^\s*#include\s+"([^"]+)"', re.MULTILINE)

FILE_SUMMARIES: dict[str, str] = {
    "USER/main.c": "固件入口：硬件初始化、上电自检、主循环按主/从地址分发通信与控制任务。",
    "USER/main.h": "全局类型定义、跨模块 extern 声明与宏常量聚合头文件。",
    "USER/stm32f10x_it.c": "Cortex 异常、EXTI、USART 与 TIM2/3/4 定时中断服务程序。",
    "USER/system_stm32f10x.c": "系统时钟初始化（HSE 72 MHz）。",
    "USER/systick/bsp_systick.c": "SysTick 延时与毫秒计数。",
    "USER/rcc/bsp_rccclkconfig.c": "外部晶振 HSE 配置与 PLL 倍频。",
    "SYSTEM/system_control/system_control.c": "锅炉控制核心（单体）：FSM 启停、点火、PID 风机、排污、补水、联控增减机、异常响应。",
    "SYSTEM/system_control/system_control.h": "系统控制 API、sys_flags/SYS_ADMIN 等核心数据结构定义。",
    "SYSTEM/usart/usart.c": "USART1 Modbus RTU 远程控制与传感器通信。",
    "SYSTEM/in_flash/Internal_flash.c": "内部 Flash 读写，持久化 Sys_Admin 参数。",
    "SYSTEM/activate_key/activate_key.c": "CPU ID 加密激活校验。",
    "SYSTEM/delay/delay.c": "延时工具函数。",
    "SYSTEM/iwdg/bsp_iwdg.c": "独立看门狗配置与喂狗。",
    "SYSTEM/rtc/bsp_rtc.c": "RTC 实时时钟驱动。",
    "SYSTEM/rtc/bsp_calendar.c": "日历转换工具。",
    "SYSTEM/rtc/bsp_date.c": "日期处理。",
    "SYSTEM/out_flash/bsp_spi_flash.c": "外部 SPI Flash 驱动。",
    "HARDWARE/USART2/usart2.c": "USART2：10.1 寸屏 / LCD4013 / 外置联控 Modbus RTU，含自适应波特率。",
    "HARDWARE/USART3/usart3.c": "USART3：多机联控发送、变频补水、加药泵 Modbus 通信。",
    "HARDWARE/USART4/usart4.c": "USART4：本地/联控 Modbus RTU，主机轮询从机状态。",
    "HARDWARE/USART5/usart5.c": "USART5 调试串口（PC12/PD2，可选）。",
    "HARDWARE/ADC/bsp_adc.c": "ADC1 压力/温度采样、滤波与炉温查表转换。",
    "HARDWARE/ADS1220/ADS1220.c": "ADS1220 SPI 高精度炉内温度采集。",
    "HARDWARE/PWM/pwm_output.c": "TIM3 PWM 风机功率输出（2 kHz）。",
    "HARDWARE/relays/BSP_RELAYS.c": "继电器过零驱动：燃气、风门、补水、排污、报警输出。",
    "HARDWARE/led/bsp_led.c": "LED/蜂鸣器、火检输入、SPI、拨码地址扫描。",
    "HARDWARE/timers/timer.c": "TIM2/TIM4 定时器初始化（1 ms 系统节拍）。",
    "HARDWARE/Parallel_Serial/bsp_parallel_serial.c": "并串转换 74HC165 数字输入读取。",
    "HARDWARE/USR_C210/usr_c210.c": "USR-C210 WiFi 模块 Modbus 透传解析。",
}

KEY_FUNCTIONS: dict[str, tuple[str, list[int], str]] = {
    "USER/main.c:main": ("main", [57, 267], "固件唯一入口：初始化后进入主循环。"),
    "SYSTEM/system_control/system_control.c:System_All_Control": (
        "System_All_Control",
        [1976, 2137],
        "锅炉主控调度：按 sys_staus 分发空闲/点火/运行/异常各子状态机。",
    ),
    "SYSTEM/system_control/system_control.c:One_Sec_Check": (
        "One_Sec_Check",
        [2390, 2458],
        "1 Hz 周期检查：工作时间累计、RTC 同步、安全检测。",
    ),
    "SYSTEM/system_control/system_control.c:Sys_Ignition_Fun": (
        "Sys_Ignition_Fun",
        [326, 857],
        "点火序列 FSM：预吹扫、开阀、点火继电器、火焰确认。",
    ),
    "SYSTEM/system_control/system_control.c:System_Pressure_Balance_Function": (
        "System_Pressure_Balance_Function",
        [1068, 1130],
        "压力平衡与 PID 风机调节（主机）。",
    ),
    "SYSTEM/system_control/system_control.c:D50L_SoloPressure_Union_MuxJiZu_Control_Function": (
        "D50L_SoloPressure_Union_MuxJiZu_Control_Function",
        [4851, 4860],
        "多拼联控增减机：按压力需求智能加机/减机。",
    ),
    "SYSTEM/system_control/system_control.c:Auto_Pai_Wu_Function": (
        "Auto_Pai_Wu_Function",
        [3073, 3319],
        "自动排污 FSM：定时/水位触发排污阀控制。",
    ),
    "SYSTEM/system_control/system_control.c:Abnormal_Events_Response": (
        "Abnormal_Events_Response",
        [1340, 1627],
        "异常工况响应：熄火吹扫、故障停机与自动重启。",
    ),
    "SYSTEM/system_control/system_control.c:Auto_Baudrate_check_Function": (
        "Auto_Baudrate_check_Function",
        [4306, 4418],
        "USART2 自适应波特率检测（9600/115200）。",
    ),
    "HARDWARE/USART4/usart4.c:Union_Modbus4_UnionTx_Communication": (
        "Union_Modbus4_UnionTx_Communication",
        [764, 881],
        "主机经 UART4 轮询从机 Modbus 状态。",
    ),
    "HARDWARE/USART2/usart2.c:Union_ModBus2_Communication": (
        "Union_ModBus2_Communication",
        [122, 1002],
        "主机 USART2 与 10.1 寸屏联控通信解析。",
    ),
    "HARDWARE/PWM/pwm_output.c:PWM_Adjust": (
        "PWM_Adjust",
        [102, 120],
        "设置风机 PWM 占空比（0–100%）。",
    ),
    "HARDWARE/ADC/bsp_adc.c:ADC_Process": (
        "ADC_Process",
        [758, 863],
        "ADC 采样主流程：压力/温度/烟气处理。",
    ),
}

CALL_HINTS: dict[str, list[str]] = {
    "USER/main.c:main": [
        "SYSTEM/system_control/system_control.c:System_All_Control",
        "SYSTEM/system_control/system_control.c:One_Sec_Check",
        "HARDWARE/ADC/bsp_adc.c:ADC_Process",
        "HARDWARE/USART4/usart4.c:Union_Modbus4_UnionTx_Communication",
    ],
    "SYSTEM/system_control/system_control.c:System_All_Control": [
        "SYSTEM/system_control/system_control.c:Sys_Ignition_Fun",
        "SYSTEM/system_control/system_control.c:System_Pressure_Balance_Function",
        "SYSTEM/system_control/system_control.c:Abnormal_Events_Response",
        "SYSTEM/system_control/system_control.c:Auto_Pai_Wu_Function",
    ],
}


def rel_path(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def should_scan(path: Path) -> bool:
    parts = set(path.parts)
    if parts & SKIP_PARTS:
        return False
    return path.suffix in {".c", ".h"} and any(p in parts for p in SCAN_DIRS)


def git_hash() -> str | None:
    try:
        out = subprocess.check_output(
            ["git", "rev-parse", "HEAD"],
            cwd=ROOT,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        return out.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def scan_functions(path: Path, text: str) -> list[tuple[str, int]]:
    found: list[tuple[str, int]] = []
    for match in FUNC_RE.finditer(text):
        name = match.group(1)
        if name in {"if", "for", "while", "switch", "return"}:
            continue
        line = text.count("\n", 0, match.start()) + 1
        found.append((name, line))
    return found


def resolve_include(src: Path, inc: str) -> str | None:
    candidates = [
        src.parent / inc,
        ROOT / inc,
        ROOT / "USER" / inc,
        ROOT / "SYSTEM" / inc,
        ROOT / "HARDWARE" / inc,
    ]
    for cand in candidates:
        if cand.is_file():
            return rel_path(cand)
    return None


def file_tags(path: str) -> list[str]:
    tags: list[str] = []
    lower = path.lower()
    if lower.endswith(".h"):
        tags.append("header")
    if "usart" in lower or "uart" in lower:
        tags.append("communication")
    if "system_control" in lower:
        tags.append("boiler-control")
    if "adc" in lower or "ads1220" in lower:
        tags.append("sensor")
    if "relay" in lower or "pwm" in lower:
        tags.append("actuator")
    if "main.c" in lower:
        tags.append("entry-point")
    return tags or ["module"]


def build_graph() -> dict:
    nodes: list[dict] = []
    edges: list[dict] = []
    node_ids: set[str] = set()
    edge_keys: set[tuple[str, str, str]] = set()

    def add_node(node: dict) -> None:
        if node["id"] in node_ids:
            return
        node_ids.add(node["id"])
        nodes.append(node)

    def add_edge(src: str, tgt: str, etype: str, weight: float = 1.0) -> None:
        key = (src, tgt, etype)
        if key in edge_keys:
            return
        edge_keys.add(key)
        edges.append(
            {
                "source": src,
                "target": tgt,
                "type": etype,
                "direction": "forward",
                "weight": weight,
            }
        )

    source_files: list[Path] = sorted(
        p for p in ROOT.rglob("*") if should_scan(p)
    )

    exported: dict[str, set[str]] = {}

    for path in source_files:
        rel = rel_path(path)
        text = path.read_text(encoding="utf-8", errors="ignore")
        summary = FILE_SUMMARIES.get(rel, f"{path.name} 模块源文件。")
        add_node(
            {
                "id": f"file:{rel}",
                "type": "file",
                "name": path.name,
                "filePath": rel,
                "summary": summary,
                "tags": file_tags(rel),
                "complexity": "complex" if rel.endswith("system_control.c") else "moderate",
            }
        )

        if path.suffix == ".h":
            exported[rel] = {m.group(1) for m in FUNC_RE.finditer(text)}

        for inc in INCLUDE_RE.findall(text):
            target = resolve_include(path, inc)
            if target:
                add_edge(f"file:{rel}", f"file:{target}", "includes", 0.9)

        for name, line in scan_functions(path, text):
            if path.suffix != ".c":
                continue
            fid = f"function:{rel}:{name}"
            is_exported = name in exported.get(rel.replace(".c", ".h"), set())
            tags = ["function"]
            if is_exported:
                tags.extend(["exported", "api"])
            add_node(
                {
                    "id": fid,
                    "type": "function",
                    "name": name,
                    "filePath": rel,
                    "lineRange": [line, line + 1],
                    "summary": f"{name}() 定义于 {path.name}。",
                    "tags": tags,
                    "complexity": "complex"
                    if name in {"System_All_Control", "Sys_Ignition_Fun", "main"}
                    else "moderate",
                }
            )
            add_edge(f"file:{rel}", fid, "contains", 1.0)
            if is_exported:
                add_edge(f"file:{rel}", fid, "exports", 0.8)

    for key, (name, line_range, summary) in KEY_FUNCTIONS.items():
        rel, _, _ = key.partition(":")
        fid = f"function:{rel}:{name}"
        if fid in node_ids:
            for node in nodes:
                if node["id"] == fid:
                    node["lineRange"] = line_range
                    node["summary"] = summary
                    node["tags"] = list(set(node.get("tags", []) + ["key-api"]))
                    node["complexity"] = "complex"
                    break

    for src_key, targets in CALL_HINTS.items():
        src_rel, _, src_name = src_key.partition(":")
        src_id = f"function:{src_rel}:{src_name}"
        for tgt_rel_name in targets:
            tgt_rel, _, tgt_name = tgt_rel_name.partition(":")
            tgt_id = f"function:{tgt_rel}:{tgt_name}"
            if src_id in node_ids and tgt_id in node_ids:
                add_edge(src_id, tgt_id, "calls", 0.7)

    tour = [
        {
            "order": 1,
            "title": "项目概览",
            "description": (
                "MCGS 三拼/五拼 V2.1.0 锅炉控制器固件，基于 STM32F103VC。"
                "单体 system_control.c 承载主 FSM，四路 UART 分别对接远程、大屏、补水/加药、联控。"
            ),
            "nodeIds": ["file:USER/main.c", "file:SYSTEM/system_control/system_control.h"],
        },
        {
            "order": 2,
            "title": "入口与主循环",
            "description": (
                "main.c 完成时钟、ADC、四路 UART、定时器初始化与上电自检。"
                "主循环按 Address_Number 区分主机(0)与从机(1–6)，分发通信与控制任务。"
            ),
            "nodeIds": ["file:USER/main.c", "function:USER/main.c:main"],
        },
        {
            "order": 3,
            "title": "锅炉状态机",
            "description": (
                "system_control.c 是控制核心：System_All_Control 调度点火、运行、空闲、异常等状态。"
                "One_Sec_Check 提供 1 Hz 安全与时间累计。"
            ),
            "nodeIds": [
                "file:SYSTEM/system_control/system_control.c",
                "function:SYSTEM/system_control/system_control.c:System_All_Control",
                "function:SYSTEM/system_control/system_control.c:Sys_Ignition_Fun",
            ],
            "languageLesson": "本版本采用 switch-case FSM 而非表驱动，逻辑集中在单文件，修改时需关注状态变量 sys_staus/Ignition_Index。",
        },
        {
            "order": 4,
            "title": "通信拓扑",
            "description": (
                "UART1 远程传感器；UART2 大屏/LCD4013；UART3 补水变频与加药；UART4 多机联控。"
                "主机轮询从机，从机执行 System_All_Control。"
            ),
            "nodeIds": [
                "file:HARDWARE/USART2/usart2.c",
                "file:HARDWARE/USART3/usart3.c",
                "file:HARDWARE/USART4/usart4.c",
                "file:SYSTEM/usart/usart.c",
            ],
        },
        {
            "order": 5,
            "title": "传感与执行",
            "description": "ADC/ADS1220 采集压力温度，PWM 驱动风机，继电器过零控制燃气/风门/补水/排污。",
            "nodeIds": [
                "file:HARDWARE/ADC/bsp_adc.c",
                "file:HARDWARE/PWM/pwm_output.c",
                "file:HARDWARE/relays/BSP_RELAYS.c",
            ],
        },
    ]

    return {
        "version": "1.0.0",
        "project": {
            "name": "MCGS 三拼锅炉控制器固件",
            "languages": ["c"],
            "frameworks": ["STM32 Standard Peripheral Library", "Keil uVision"],
            "description": (
                "基于 STM32F103VC 的 MCGS 三拼/五拼长板锅炉控制器固件（V2.1.0 / Soft_Version=210）。"
                "含主从联控、单体 FSM 锅炉控制、PID 风机、Modbus RTU 四路通信、"
                "自动排污/补水、加药泵、风速检测与 USART2 自适应波特率。"
            ),
            "analyzedAt": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.000Z"),
            "gitCommitHash": git_hash(),
        },
        "nodes": nodes,
        "edges": edges,
        "tour": tour,
    }


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    config = OUT_DIR / "config.json"
    if not config.exists():
        config.write_text(
            json.dumps({"language": "zh", "outputLanguage": "zh"}, indent=2) + "\n",
            encoding="utf-8",
        )
    graph = build_graph()
    OUT_FILE.write_text(
        json.dumps(graph, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"Generated {OUT_FILE}")
    print(f"  nodes: {len(graph['nodes'])}")
    print(f"  edges: {len(graph['edges'])}")


if __name__ == "__main__":
    main()
